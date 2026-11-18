/* nd_ds.c: the core 1 draw state and its apply (design_nstate.md section 4, row NST1). Each case copies the state writes of the core 0
 * handler named in its comment, in the same order. Left out: every pm_gpu_* call, flush, sync_combine, GP_SET, lit_freeze, log, and the
 * draw-time fields (td[], tile_x0/y0, tile_fx/fy, cur_tile, box_folded, leg_mode, comb_mode5, box_pat, the plan, fogmix_on which sync_render
 * writes, the tex_shaded0 record m1). The initial values are those of nd_ws (nd_state.c), the global initializers and pm_gbi_init
 * (geom G_SHADE | G_SHADING_SMOOTH). Pure C: the host test builds it with stubs for is_host, pm_wide and tex_win_s25. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "nd_ds.h"
#include "nd_gtab.h"
#include "nd_ops.h"
#include "nd_op.h"
#include "nd_dual.h"
#include "macros.h"
#include "nd_fs.h"
unsigned char nd_lsc_on; __attribute__((weak)) void nd_lsc_unit(uint32_t w0) { (void)w0; } /* NCONV1D1: the list units of a snap collector (nd_snap.c) */ __attribute__((weak)) void nd_lsc_tris(unsigned n) { (void)n; } /* NREC4 */ __attribute__((weak)) void nd_lsc_ep(void) {} /* NCONV2C */

extern int pm_wide;  /* nd_set.c set_scissor: the wide sign rule */
int is_host(u32 p);  /* nd_walk.c: the range test of ptr_res */
void tex_win_s25(int* p_uls, int* p_ult, int* p_lrs, int* p_lrt, unsigned fs, unsigned ms, unsigned ft, unsigned mt); /* nd_tex_set.c */

NdDs nd_ds = {
    .w = { .geom = G_SHADE | G_SHADING_SMOOTH, .omode_h = 0, .omode_l = 0x00000030u, .prim = 0xffffffffu, .envc = 0xffffffffu,
           .fill = 0xffffffffu, .tex_ss = 0xFFFF, .tex_ts = 0xFFFF },
    .comb_alpha_shade = 1, .sc_lost = 1,
    .sci_x1 = 320, .sci_y1 = 240,
    .vp_sx = 160.f, .vp_sy = 120.f, .vp_sz = 127.75f, .vp_tx = 160.f, .vp_ty = 120.f, .vp_tz = 127.75f,
};

/* ptr_res of nd_walk.c on the own segment table (ptr_of also notes the pointer for the core 2 record: core 1 must not). */
static void* ds_ptr(const NdDs* s, u32 w) {
    if (!w) return NULL;
    if (is_host(w)) return (void*)(uintptr_t)w;
    unsigned seg = (w >> 24) & 0xff;
    if (seg < 16 && s->w.segbase[seg]) return (void*)(uintptr_t)(s->w.segbase[seg] + (w & 0x00ffffffu));
    if ((w & 0xc0000000u) == 0x80000000u && is_host(w & 0x1fffffffu)) return (void*)(uintptr_t)(w & 0x1fffffffu);
    return (void*)(uintptr_t)w;
}

#define SET(f, val) do { __typeof__(f) v_ = (val); if ((f) != v_) { (f) = v_; ch = 1; } } while (0)

/* nd_set.c set_om (HC4a): the state writes of the othermode setter. */
static unsigned ds_om(NdDs* s, int h, uint32_t mask, u32 v) {
    unsigned ch = 0;
    if (h) {
        SET(s->w.omode_h, (s->w.omode_h & ~mask) | (v & mask));
        SET(s->w.om_kh, s->w.om_kh | mask);
    } else {
        SET(s->w.omode_l, (s->w.omode_l & ~mask) | (v & mask));
        SET(s->w.om_kl, s->w.om_kl | mask);
    }
    return ch;
}

/* nd_comb.c op_setcombine: the combine words, the derived fields, the TEXEL1 rename, dual_on, box_prod, comb_alpha_shade. */
static unsigned ds_comb(NdDs* s, u32 w0, u32 w1) {
    unsigned ch = 0;
    NdTst* t = &s->w.tst1;
    SET(s->w.comb_w0, w0);
    SET(s->w.comb_w1, w1);
    SET(t->comb_a, (w0 >> 20) & 0xf);
    SET(t->comb_c, (w0 >> 15) & 0x1f);
    SET(t->comb_b, (w1 >> 28) & 0xf);
    SET(t->comb_d, (w1 >> 15) & 7);
    SET(s->comb_set, 1);
    SET(t->comb_aa, (w0 >> 12) & 7);
    SET(t->comb_ac, (w0 >> 9) & 7);
    SET(t->comb_ab, (w1 >> 12) & 7);
    SET(t->comb_ad, (w1 >> 9) & 7);
    SET(t->comb2_a, (w0 >> 5) & 0xf);
    SET(t->comb2_c, w0 & 0x1f);
    SET(t->comb2_b, (w1 >> 24) & 0xf);
    SET(t->comb2_d, (w1 >> 6) & 7);
    SET(t->comb2_aa, (w1 >> 21) & 7);
    SET(t->comb2_ac, (w1 >> 18) & 7);
    SET(t->comb2_ab, (w1 >> 3) & 7);
    SET(t->comb2_ad, w1 & 7);
    {
        int t0 = t->comb_a == 1 || t->comb_b == 1 || t->comb_d == 1 || t->comb_c == 1 || t->comb_c == 8 ||
                 t->comb_aa == 1 || t->comb_ab == 1 || t->comb_ac == 1 || t->comb_ad == 1;
        int t1 = t->comb_a == 2 || t->comb_b == 2 || t->comb_d == 2 || t->comb_c == 2 || t->comb_c == 9 ||
                 t->comb_aa == 2 || t->comb_ab == 2 || t->comb_ac == 2 || t->comb_ad == 2;
        int c2 = t->comb2_a == 1 || t->comb2_b == 1 || t->comb2_d == 1 || t->comb2_c == 1 || t->comb2_c == 8 ||
                 t->comb2_aa == 1 || t->comb2_ab == 1 || t->comb2_ac == 1 || t->comb2_ad == 1 ||
                 t->comb2_a == 2 || t->comb2_b == 2 || t->comb2_d == 2 || t->comb2_c == 2 || t->comb2_c == 9 ||
                 t->comb2_aa == 2 || t->comb2_ab == 2 || t->comb2_ac == 2 || t->comb2_ad == 2;
        SET(s->w.use_t1, 0);
        if (t1 && !t0 && !c2) {
            SET(s->w.use_t1, 1);
            if (t->comb_a == 2) SET(t->comb_a, 1);
            if (t->comb_b == 2) SET(t->comb_b, 1);
            if (t->comb_d == 2) SET(t->comb_d, 1);
            if (t->comb_c == 2) SET(t->comb_c, 1);
            if (t->comb_c == 9) SET(t->comb_c, 8);
            if (t->comb_aa == 2) SET(t->comb_aa, 1);
            if (t->comb_ab == 2) SET(t->comb_ab, 1);
            if (t->comb_ac == 2) SET(t->comb_ac, 1);
            if (t->comb_ad == 2) SET(t->comb_ad, 1);
        }
    }
    if (s->dual_on && !nd_dual_keep(t->comb_a, t->comb_c)) SET(s->dual_on, 0);
    SET(s->box_prod, t->comb_a == 1 && t->comb_b >= 7 && t->comb_c == 2 && t->comb_d == 7 &&
                     t->comb_aa == 7 && t->comb_ab == 7 && t->comb_ac == 7 && t->comb_ad == 2 &&
                     t->comb2_a == 3 && t->comb2_b == 5 && t->comb2_c == 0 && t->comb2_d == 5 &&
                     t->comb2_aa == 7 && t->comb2_ab == 7 && t->comb2_ac == 7 && t->comb2_ad == 0);
    SET(s->comb_alpha_shade, ((w0 >> 12) & 7) == 4 || ((w0 >> 9) & 7) == 4 || ((w1 >> 12) & 7) == 4 || ((w1 >> 9) & 7) == 4);
    return ch;
}

/* nd_set.c set_rare: G_SETCONVERT, G_SETKEYR, G_SETKEYGB. */
static unsigned ds_rare(NdDs* s, unsigned k, u32 w1) {
    unsigned ch = 0;
    if (k == ND_FS_RARE_CONV) SET(s->conv_k4, (w1 >> 9) & 0xffu);
    else if (k == ND_FS_RARE_KEYR) {
        SET(s->keyc, (s->keyc & 0x00ffffffu) | ((w1 >> 8 & 0xffu) << 24));
        SET(s->keys, (s->keys & 0x00ffffffu) | ((w1 & 0xffu) << 24));
    } else {
        SET(s->keyc, (s->keyc & 0xff000000u) | ((w1 >> 24 & 0xffu) << 16) | ((w1 >> 8 & 0xffu) << 8));
        SET(s->keys, (s->keys & 0xff000000u) | ((w1 >> 16 & 0xffu) << 16) | ((w1 & 0xffu) << 8));
    }
    return ch;
}

/* nd_set.c set_mw and nd_mtx_set.c nd_light_count. */
static unsigned ds_mw(NdDs* s, unsigned kind, unsigned arg, u32 w1) {
    unsigned ch = 0;
    switch (kind) {
    case ND_FS_MW_NUMLIGHT: { unsigned n = w1 / 24; if (n > 7) n = 7; SET(s->lt_n, n); break; }
    case ND_FS_MW_LIGHTCOL:
        if (arg >= 9) break; /* core 0 does not test it; the game writes slots 0-8 */
        SET(s->lt[arg].col[0], (uint8_t)(w1 >> 24));
        SET(s->lt[arg].col[1], (uint8_t)(w1 >> 16));
        SET(s->lt[arg].col[2], (uint8_t)(w1 >> 8));
        break;
    case ND_FS_MW_FOG: SET(s->fog_fm, (int16_t)(w1 >> 16)); SET(s->fog_fo, (int16_t)w1); break;
    case ND_FS_MW_SEG: {
        if (arg >= 16) break;
        u32 b = (u32)(uintptr_t)ds_ptr(s, w1);
        if (!b) b = w1;
        SET(s->w.segbase[arg], b);
        break;
    }
    default: break;
    }
    return ch;
}

/* nd_set.c set_scissor (pm_wide sign rule). */
static unsigned ds_scissor(NdDs* s, u32 w0, u32 w1) {
    unsigned ch = 0;
    int ulx = (int)((w0 >> 12) & 0xfff), uly = (int)(w0 & 0xfff), lrx = (int)((w1 >> 12) & 0xfff), lry = (int)(w1 & 0xfff);
    if (pm_wide) {
        if (ulx & 0x800) ulx |= ~0xfff;
        if (lrx & 0x800) lrx |= ~0xfff;
    }
    SET(s->sci_x0, ulx >> 2);
    SET(s->sci_y0, uly >> 2);
    SET(s->sci_x1, lrx >> 2);
    SET(s->sci_y1, lry >> 2);
    return ch;
}

/* nd_tex_set.c set_tile, set_tilesize (the stored size and the dual_on reset only). */
static unsigned ds_tile(NdDs* s, u32 a, u32 b) {
    unsigned ch = 0, tile = (b >> 24) & 7;
    NdTst* t = &s->w.tst1;
    unsigned wr[4] = { (b >> 8) & 3, (b >> 18) & 3, (b >> 4) & 15, (b >> 14) & 15 };
    SET(t->tile_fmt[tile], (a >> 21) & 7);
    SET(t->tile_siz[tile], (a >> 19) & 3);
    SET(t->tile_tmem[tile], a & 0x1ff);
    SET(t->tile_pal[tile], (b >> 20) & 15);
    for (unsigned i = 0; i < 4; i++) SET(t->tile_wrap[tile][i], wr[i]);
    SET(t->tile_shift[tile][0], b & 15);
    SET(t->tile_shift[tile][1], (b >> 10) & 15);
    return ch;
}
static void ds_td0(NdDs* s, int uls, int ult, int lrs, int lrt) {   /* set_tilesize tile 0 (nd_tex_set.c:158-213) */
    NdTst* t = &s->w.tst1; NdTd* d = &s->w.td[0]; NdWin wn;
    nd_texw0(t, &s->tm, s->w.timg, uls, ult, lrs, lrt, &wn);
    if (!wn.src || wn.w <= 0 || wn.h <= 0) return;
    if (wn.h == 1 && (unsigned)(uintptr_t)s->w.timg - (unsigned)(uintptr_t)nuGfxZBuffer < 320u * 240u * 2u) return;
    d->valid = 1; d->src = wn.src; d->fmt = t->tile_fmt[0]; d->siz = t->tile_siz[0]; d->w = wn.w; d->h = wn.h; d->stride = wn.stride;
    d->x0 = uls >> 2; d->y0 = ult >> 2; d->pal_n = s->w.tlut_n;
    d->pal = s->tm.tlut_pal[t->tile_pal[0] & 15] ? s->tm.tlut_pal[t->tile_pal[0] & 15] : s->w.tlut; }
static void ds_td1(NdDs* s, int uls, int ult, int lrs, int lrt) {   /* tex_tile1 (nd_tex_set.c:464-482) */
    NdTst* t = &s->w.tst1; NdTd* d = &s->w.td[1]; NdWin wn;
    nd_texw1(t, &s->tm, s->w.timg, uls, ult, lrs, lrt, &wn);
    d->valid = 0;
    if (!wn.src || wn.w <= 0 || wn.h <= 0) return;
    d->valid = 1; d->src = wn.src; d->fmt = t->tile_fmt[1]; d->siz = t->tile_siz[1]; d->w = wn.w; d->h = wn.h; d->stride = wn.stride;
    d->x0 = wn.x0; d->y0 = ult >> 2; d->pal_n = s->w.tlut_n;
    d->pal = s->tm.tlut_pal[t->tile_pal[1] & 15] ? s->tm.tlut_pal[t->tile_pal[1] & 15] : s->w.tlut; }
static unsigned ds_tilesize(NdDs* s, u32 a, u32 b) {
    unsigned ch = 0, tile = (b >> 24) & 7;
    NdTst* t = &s->w.tst1;
    int uls = (int)((a >> 12) & 0xfff), ult = (int)(a & 0xfff), lrs = (int)((b >> 12) & 0xfff), lrt = (int)(b & 0xfff);
    if (((lrs - uls) >> 2) + 1 <= 0 || ((lrt - ult) >> 2) + 1 <= 0)
        tex_win_s25(&uls, &ult, &lrs, &lrt, t->tile_wrap[tile][0], t->tile_wrap[tile][2], t->tile_wrap[tile][1], t->tile_wrap[tile][3]);
    SET(t->tile_sz[tile][0], uls);
    SET(t->tile_sz[tile][1], ult);
    SET(t->tile_sz[tile][2], lrs);
    SET(t->tile_sz[tile][3], lrt);
    if (tile == 0 && s->dual_on) SET(s->dual_on, 0);
    if (tile == 1 && t->tile_fmt[1] == 2 && t->tile_siz[1] == 0 && s->tm.ld_valid && s->w.timg) {        /* tex_tile1 :453 */
        s->m1v = 1; s->m1w = ((lrs - uls) >> 2) + 1; s->m1h = ((lrt - ult) >> 2) + 1; }
    if (tile == 1) ds_td1(s, uls, ult, lrs, lrt);                                                       /* tex_tile1 :468-481 */
    if (tile == 0) {                                                                                   /* tex_shaded0 :497 */
        s->w.td[0].valid = 0;                                                                          /* set_tilesize :153 */
        const int sh = s->m1v && t->tile_fmt[0] == 2 && t->tile_siz[0] == 0 && t->tile_pal[0] != t->tile_pal[1] && t->comb_a == 1 && t->comb_c == 2 &&
            s->m1w > 0 && s->m1h > 0; /* GUESS: load_shaded_sprite succeeds */
        if (sh) SET(s->dual_on, 1);
        s->m1v = 0;
        if (!sh) ds_td0(s, uls, ult, lrs, lrt); }                                                       /* set_tilesize :158-213 */
    return ch;
}

/* nd_tex_set.c set_timg, set_loadblock, set_loadtile, set_loadtlut. */
static unsigned ds_timg(NdDs* s, u32 a, u32 addr) {
    unsigned ch = 0;
    SET(s->w.timg, (const void*)ds_ptr(s, addr));
    SET(s->w.tst1.timg_fmt, (a >> 21) & 7);
    SET(s->w.tst1.timg_siz, (a >> 19) & 3);
    SET(s->w.tst1.timg_stride, (int)(a & 0xfff) + 1);
    SET(s->tm.ld_valid, 0);
    return ch;
}
static unsigned ds_loadblock(NdDs* s, u32 b) {
    unsigned ch = 0, tile = (b >> 24) & 7, texels = (b >> 12) & 0xfff;
    NdTst* t = &s->w.tst1;
    unsigned tm = t->tile_tmem[tile];
    unsigned words = ((texels + 1u) << t->tile_siz[tile]) >> 4;
    for (unsigned k = 0; k < 4; k++)
        if (t->ldrec[k].tmem >= tm && (t->ldrec[k].tmem == tm || t->ldrec[k].tmem < tm + words)) SET(t->ldrec[k].img, NULL);
    unsigned k;
    for (k = 0; k < 4 && !(t->blk[k].img && t->blk[k].tmem == tm); k++) {}
    if (k == 4) { k = s->tm.blk_n & 3; SET(s->tm.blk_n, s->tm.blk_n + 1); }
    SET(t->blk[k].tmem, tm);
    SET(t->blk[k].img, s->w.timg);
    SET(t->blk[k].bytes, ((texels + 1u) << t->tile_siz[tile]) >> 1);
    return ch;
}
static unsigned ds_loadtile(NdDs* s, u32 a, u32 b) {
    unsigned ch = 0, tile = (b >> 24) & 7;
    NdTst* t = &s->w.tst1;
    int uls = (int)((a >> 12) & 0xfff), ult = (int)(a & 0xfff), lrs = (int)((b >> 12) & 0xfff), lrt = (int)(b & 0xfff);
    SET(s->tm.ld_valid, 1);
    SET(s->tm.ld_x, uls >> 2);
    unsigned tm = t->tile_tmem[tile], k;
    for (k = 0; k < 4 && !(t->ldrec[k].img && t->ldrec[k].tmem == tm); k++) {}
    if (k == 4) { k = s->tm.ldrec_n & 3; SET(s->tm.ldrec_n, s->tm.ldrec_n + 1); }
    SET(t->ldrec[k].tmem, tm);
    SET(t->ldrec[k].img, s->w.timg);
    SET(t->ldrec[k].stride, t->timg_stride);
    SET(t->ldrec[k].x, uls >> 2);
    SET(t->ldrec[k].y, ult >> 2);
    SET(s->tm.ld_y, ult >> 2);
    SET(s->tm.ld_h, ((lrt - ult) >> 2) + 1);
    SET(s->tm.ld_w, ((lrs - uls) >> 2) + 1);
    return ch;
}
static unsigned ds_loadtlut(NdDs* s, u32 b) {
    unsigned ch = 0, tile = (b >> 24) & 7, count = (b >> 14) & 0x3ff;
    SET(s->w.tlut, s->w.timg);
    SET(s->w.tlut_n, (int)count + 1);
    unsigned slot = (s->w.tst1.tile_tmem[tile] - 256u) >> 4;
    if (slot < 16) {
        SET(s->tm.tlut_pal[slot], s->w.timg);
        for (int n = 0; n < 2; n++)                                                                    /* tlut_v20 :487-493 */
            if (s->w.td[n].valid && s->w.td[n].fmt == 2 && (s->w.tst1.tile_pal[n] & 15) == slot && s->w.td[n].pal != s->w.timg) {
                s->w.td[n].pal = s->w.timg;
                s->w.td[n].pal_n = s->w.tlut_n; }
    }
    return ch;
}

/* nd_mtx_set.c gbi_movemem and gbi_viewport (no lit_freeze). */
static unsigned ds_movemem(NdDs* s, u32 w0, u32 w1) {
    unsigned ch = 0;
    if ((w0 & 0xff) == G_MV_LIGHT) {
        int slot = (int)(((w0 >> 8) & 0xff) * 8 / 24) - 2;
        const uint8_t* l = (const uint8_t*)ds_ptr(s, w1);
        if (l && (slot == -2 || slot == -1)) {
            SET(s->lk[slot + 2][0], (int8_t)l[8]);
            SET(s->lk[slot + 2][1], (int8_t)l[9]);
            SET(s->lk[slot + 2][2], (int8_t)l[10]);
        }
        if (l && slot >= 0 && slot < 9) {
            SET(s->lt[slot].col[0], l[0]); SET(s->lt[slot].col[1], l[1]); SET(s->lt[slot].col[2], l[2]);
            SET(s->lt[slot].dir[0], (int8_t)l[8]); SET(s->lt[slot].dir[1], (int8_t)l[9]); SET(s->lt[slot].dir[2], (int8_t)l[10]);
        }
    }
    if ((w0 & 0xff) == G_MV_VIEWPORT) {
        const Vp* vp = (const Vp*)ds_ptr(s, w1);
        if (vp) {
            SET(s->vp_sx, (float)vp->vp.vscale[0] / 4.f);
            SET(s->vp_sy, (float)vp->vp.vscale[1] / 4.f);
            SET(s->vp_sz, (float)vp->vp.vscale[2] / 4.f);
            SET(s->vp_tx, (float)vp->vp.vtrans[0] / 4.f);
            SET(s->vp_ty, (float)vp->vp.vtrans[1] / 4.f);
            SET(s->vp_tz, (float)vp->vp.vtrans[2] / 4.f);
        }
    }
    return ch;
}

/* NST2D: the unit log. Core 1 units applied to nd_ds while a range is open (lg_dep) are logged; a range replay (nd_ds_range) applies the
 * logged units between two marks. A chunk = 4096 units (32 KB). */
static uint32_t* lg_ch[8];
static unsigned lg_n, lg_dep, lg_nmk, sv_n;
static NdDs sv[4];
unsigned nd_ds_lg_over, nd_ds_lg_hw;
unsigned nd_ds_rpl; /* NST2D: depth of range replays (nd_rec.c stream_run, nd_walk.c nd_range_run); core 1 skips its dschk copy inside one */
static struct { const uint32_t* pos; unsigned i; } lg_mk[1024];
static void lg_put(uint32_t k, uint32_t v) {
    const unsigned c = lg_n >> 12;
    if (c >= 8u || (!lg_ch[c] && !(lg_ch[c] = (uint32_t*)malloc(4096u * 8u)))) { nd_ds_lg_over++; return; }
    lg_ch[c][2u * (lg_n & 4095u)] = k;
    lg_ch[c][2u * (lg_n & 4095u) + 1u] = v;
    if (++lg_n > nd_ds_lg_hw) nd_ds_lg_hw = lg_n;
}
void nd_ds_lg_reset(void) { lg_n = lg_dep = lg_nmk = sv_n = 0; }
void nd_ds_lg_open(void) { lg_dep++; }
void nd_ds_lg_close(void) { if (lg_dep) lg_dep--; }
void nd_ds_mark(const uint32_t* pos) {
    if (lg_nmk < 1024u) { lg_mk[lg_nmk].pos = pos; lg_mk[lg_nmk++].i = lg_n; }
    else nd_ds_lg_over++;
}
void nd_ds_sub_save(void) { if (sv_n < 4u) sv[sv_n] = nd_ds; sv_n++; }
void nd_ds_sub_restore(void) { if (sv_n && --sv_n < 4u) nd_ds = sv[sv_n]; }
/* replay from mark a to mark b (b NULL: the next mark) */
void nd_ds_range(NdDs* s, const uint32_t* a, const uint32_t* b) {
    int ia = -1, ib = -1;
    if (a == b) return;
    for (int j = (int)lg_nmk - 1; j >= 0; j--) if (lg_mk[j].pos == a) { ia = j; break; }
    if (ia >= 0 && !b) ib = ia + 1 < (int)lg_nmk ? ia + 1 : -1;
    else if (ia >= 0) for (int j = (int)lg_nmk - 1; j > ia; j--) if (lg_mk[j].pos == b) { ib = j; break; }
    if (ib < 0) { nd_ds_lg_over++; return; }
    for (unsigned i = lg_mk[ia].i, e = lg_mk[ib].i; i < e; i++)
        nd_ds_apply(s, lg_ch[i >> 12][2u * (i & 4095u)], lg_ch[i >> 12][2u * (i & 4095u) + 1u]);
}

static unsigned ds_apply(NdDs* s, uint32_t k, uint32_t v) {
    if (lg_dep && s == &nd_ds) lg_put(k, v);
    const u32 a = k & 0xffffffu, b = v;
    unsigned ch = 0;
    switch (k >> 24) {
    case ND_FS_OMH: return ds_om(s, 1, nd_op_om_mask(a), b);
    case ND_FS_OML: return ds_om(s, 0, nd_op_om_mask(a), b);
    case ND_FS_GEOM: SET(s->w.geom, (s->w.geom & a) | b); break; /* nd_set.c set_geom */
    case ND_FS_PRIM: SET(s->w.prim, b); SET(s->w.prim_lod, a & 0xffffu); break; /* nd_set_prim */
    case ND_FS_ENV: SET(s->w.envc, b); break; /* gbi_setenv */
    case ND_FS_FOG: SET(s->fogc, b); break;
    case ND_FS_BLEND: SET(s->blendc, b); break;
    case ND_FS_FILL: SET(s->w.fill, b); break;
    case ND_FS_PRIMZ: SET(s->w.prim_z, (b >> 16) & 0x7fffu); break;
    case ND_FS_SCISSOR: return ds_scissor(s, a, b);
    case ND_FS_TEXTURE: SET(s->w.tex_on, (int)a); SET(s->w.tex_ss, b >> 16); SET(s->w.tex_ts, b & 0xffffu); break; /* set_texture */
    case ND_FS_MW: return ds_mw(s, a >> 16, a & 0xffffu, b);
    case ND_FS_RARE: return ds_rare(s, a, b);
    case ND_FS_COMB: return ds_comb(s, ND_FS_COMB_W0(a), b);
    case ND_FS_TIMG: return ds_timg(s, a, b);
    case ND_FS_TILE: return ds_tile(s, a, b);
    case ND_FS_TILESIZE: return ds_tilesize(s, a, b);
    case ND_FS_LOADBLOCK: return ds_loadblock(s, b);
    case ND_FS_LOADTILE: return ds_loadtile(s, a, b);
    case ND_FS_LOADTLUT: return ds_loadtlut(s, b);
    case ND_FS_MOVEMEM: return ds_movemem(s, a, b);
    default: break;
    }
    return ch;
}

static void ds_sync(NdDs* s, uint32_t k, uint32_t v, unsigned ch, uint32_t h0, int du0) {   /* NST2J: h0, du0 = omode_h, dual_on before the unit */
    int sync = 0;
    switch (k >> 24) {
    case ND_FS_PRIM: case ND_FS_ENV: sync = (ch & 1u) || s->sc_lost; break;                     /* nd_set.c:132, :135 */
    case ND_FS_TEXTURE: case ND_FS_COMB: sync = (ch & 1u) || s->sc_lost || s->sc_dirty; break; /* nd_set.c:138-141 */
    case ND_FS_RARE: sync = 1; break;                                                          /* nd_set.c:157 */
    case ND_FS_OMH:
        if (s->w.omode_h == h0 && !s->sc_lost) break;                                          /* skipped (nd_set.c:125) */
        if ((k & 0xffu) <= G_MDSFT_CYCLETYPE && (k & 0xffu) + ((k >> 8) & 0xffu) > G_MDSFT_CYCLETYPE) sync = 1; /* nd_set.c:95 */
        else if (s->w.omode_h != h0) s->sc_dirty = 1;                                          /* omode_h is a sc_sig input */
        break;
    case ND_FS_TILESIZE: sync = ((v >> 24) & 7u) == 1u || (((v >> 24) & 7u) == 0u && (du0 || s->dual_on)); break; /* nd_tex_set.c:151, :155, :509 */
    default: break;
    }
    if (sync) { s->sc_lost = 0; s->sc_dirty = 0; }
}
/* NST2J: the typed unit. ds_apply is the old body; the core 1 sync state follows it. */
unsigned nd_ds_apply(NdDs* s, uint32_t k, uint32_t v) {
    const uint32_t h0 = s->w.omode_h; const int du0 = s->dual_on;
    const unsigned ch = ds_apply(s, k, v);
    ds_sync(s, k, v, ch, h0, du0);
    return ch;
}

/* NST2A nd_ds_list (design_nstate.md section 5, NST2a). Applies the state units of a static list to nd_ds in the order core 0 runs them:
 * a v2 table its units nt->u (nu pairs); a v1 table its ops in gt_ops order (NDG_DEF d.u, NDG_UNIT, NDG_CALL); any other list its typed
 * pairs to ND_FS_END (at most DS_LIST_MAX). A call unit recurses at depth + 1 (ND_LIST_DEPTH 12, nd_walk.c); a call with w0 bit 0 (the
 * branch form) ends the list that holds it. */
/* NCONV2C: the ep rule of one applied unit (moved from nd_rec.c nd_ds_unit): the first rule that fires, 0 none. ch: nd_ds_apply's result; lost, dirty: before the unit. */
unsigned nd_ds_ep_rule(uint32_t w0, unsigned ch, int lost, int dirty) { const unsigned id = w0 >> 24;
    if ((ch & 1u) && id != ND_FS_VTX && id != ND_FS_TRI1 && id != ND_FS_TRI2 && id != ND_FS_MTX && id != ND_FS_POPMTX) return 1u;
    if ((lost && (id == ND_FS_OMH || id == ND_FS_PRIM || id == ND_FS_ENV)) || ((lost || dirty) && (id == ND_FS_TEXTURE || id == ND_FS_COMB))) return 2u;
    if (id == ND_FS_MOVEMEM && ((ch & 1u) || (w0 & 0xffu) != G_MV_LIGHT)) return 3u;
    if ((id >= ND_FS_TIMG && id <= ND_FS_LOADTLUT) || id == ND_FS_FILL || id == ND_FS_PRIMZ || id == ND_FS_SCISSOR || id == ND_FS_RARE) return 6u;
    if (id == ND_FS_MW && ((w0 & 0xffffffu) >> 16) != ND_FS_MW_LIGHTCOL) return 7u;
    return (id == ND_FS_RECT || (id == ND_FS_MISC && w0 != ND_NOP_A && ((w0 & 0xffffffu) >> 16) != ND_FS_MISC_MODVTX)) ? 8u : 0u; }
#define DS_LIST_MAX 200000u
#define DS_LIST_DEPTH 12u
static void ds_list_at(NdDs* s, uint32_t w, unsigned depth);
static void ds_gtab(NdDs* s, const NdGTab* t, unsigned depth);
/* n pairs of u; raw: stop at ND_FS_END (the list end of a typed list). */
static void ds_pairs(NdDs* s, const uint32_t* u, unsigned n, unsigned depth, int raw) {
    for (unsigned i = 0; i < n; i++) {
        const uint32_t w0 = u[2u * i], w1 = u[2u * i + 1u];
        if (raw && (w0 >> 24) == ND_FS_END) return;
        if ((w0 >> 24) == ND_FS_CALL) {
            ds_list_at(s, w1, depth + 1u);
            if (w0 & 1u) return;
        } else {
            const int lost = s->sc_lost, dirty = s->sc_dirty; const unsigned ch = nd_ds_apply(s, w0, w1);
            if (nd_lsc_on) { if (nd_ds_ep_rule(w0, ch, lost, dirty)) nd_lsc_ep(); nd_lsc_unit(w0); } /* NCONV2C */
        }
    }
}
static void ds_list_at(NdDs* s, uint32_t w, unsigned depth) {
    if (depth >= DS_LIST_DEPTH) return;
    const void* l = ds_ptr(s, w);
    if (!l) return;
    const NdNDev* nt = nd_ntab_find(l);
    if (nt) { ds_pairs(s, nt->u, nt->nu, depth, 0); return; }
    const NdGTab* g = nd_gtab_find(l);
    if (g) { ds_gtab(s, g, depth); return; }
    ds_pairs(s, (const uint32_t*)l, DS_LIST_MAX, depth, 1);
}
static void ds_gtab(NdDs* s, const NdGTab* t, unsigned depth) {
    if (depth >= DS_LIST_DEPTH) return;
    for (const NdGOp* o = t->op;; o++) {
        switch (o->k) {
        case NDG_DEF: { const NdGDef* g = (const NdGDef*)o->p; ds_pairs(s, (const uint32_t*)g->d.u, g->d.n, depth, 0); break; }
        case NDG_UNIT: ds_pairs(s, (const uint32_t*)o->p, 1u, depth, 0); break;
        case NDG_CALL:
            ds_gtab(s, (const NdGTab*)o->p, depth + 1u);
            if (o->a & 1u) return; /* a branch ends the list that holds it */
            break;
        case NDG_VTX: break; /* no state */
        case NDG_TRI: if (nd_lsc_on) nd_lsc_unit(ND_FS_W0(ND_FS_TRI1, 0)); break;
        default: return; /* NDG_END */
        }
    }
}
/* nd_rec_dl (nd_gtab.c) and nd_rec_nr (nd_rec.c, an ND_FS_CALL unit with depth 1). l: a host address or a segment address. */
void nd_ds_list(NdDs* s, const void* l, int depth) { ds_list_at(s, (uint32_t)(uintptr_t)l, (unsigned)depth); }

/* NST2b nd_ds_model (design_nstate.md section 5, row NST2b): the state units of one model record, in the order of rec_apply (nd_recs.c:72-103)
 * and rec_lights (nd_mtx_set.c:202). q: the record payload (pair Rk = q[2k], q[2k+1]). The R13 units (kept count q[26], from q[28]) and the
 * render mode pairs rm (nrm) go through unit. Left out: the matrix loads, the ranges (R11) and the billboard matrix (R8). */
#define T(id, a, b) nd_ds_apply(s, ND_FS_W0(id, a), (b))
#define FOGP(x) T(ND_FS_MW, ND_FS_MW_FOG << 16, x)
void nd_ds_model(NdDs* s, const uint32_t* q, const uint32_t* rm, unsigned nrm, NdDsUnitFn unit) {
    const uint32_t f = q[6];
    if (f & 4u) T(ND_FS_PRIM, 0, q[12]);
    if (f & 8u) T(ND_FS_ENV, 0, q[13]);
    if (f & 64u) { T(ND_FS_FOG, 0, q[8]); FOGP(q[11]); }
    if (q[7] && q[4] <= 7u) {
        const unsigned n = q[4] ? q[4] : 1u;
        const Lightsn* L = (const Lightsn*)(uintptr_t)q[7];
        T(ND_FS_MW, ND_FS_MW_NUMLIGHT << 16, NUML(n));
        for (unsigned i = 1; i <= n + 1u; i++)
            T(ND_FS_MOVEMEM, (((uint32_t)sizeof(Light) - 1u) / 8u) << 19 | (3u * i + 3u) << 8 | G_MV_LIGHT,
              (uint32_t)(uintptr_t)(i <= n ? (const void*)&L->l[i - 1u] : (const void*)&L->a));
    }
    if (q[26]) { for (unsigned k = 0; k < q[26]; k++) unit(s, q[28 + 2 * k], q[29 + 2 * k]); }
    else if (q[1]) for (const uint32_t* u = (const uint32_t*)(uintptr_t)q[1]; (u[0] >> 24) != ND_FS_END; u += 2) nd_ds_apply(s, u[0], u[1]);
    if (q[9]) { const uint32_t* c = (const uint32_t*)(uintptr_t)q[9]; nd_ds_apply(s, c[0], c[1]); }
    for (unsigned k = 0; k < nrm; k++) unit(s, rm[2 * k], rm[2 * k + 1]);
    if (f & 32u) { T(ND_FS_OML, ND_FS_A_RENDERMODE, (uint32_t)(PM_RM_SHROUD) | q[15]); T(ND_FS_FOG, 0, q[8]); if (f & 16u) T(ND_FS_BLEND, 0, q[14]); FOGP(q[11]); }
    else if (f & 2u) { T(ND_FS_FOG, 0, q[8]); FOGP(q[11]); }
    if (q[22]) nd_ds_range(s, (const uint32_t*)(uintptr_t)q[22], NULL); /* NST2D: custom gfx pre (R11, nd_range_run) */
    if (q[18]) nd_ds_apply(s, q[18], q[19]);
    if (q[20]) nd_ds_apply(s, q[20], q[21]);
    s->sc_lost = 1; /* NST2J: the model runner leaves sc_ok 0 before the post range NREC1A: the caller applies the post range */
}
#undef T
#undef FOGP

/* NST3 dschk compare (design_nstate.md section 3). The draw-time fields are zeroed in both copies first: core 0 writes them at draw
 * time (td[], tile_x0/y0, tile_fx/fy, cur_tile) and sync_render writes fogmix_on. The table names every other field of NdDs (the
 * NdWs fields one by one, tst1 and tm as blocks). Returns the count of differing fields; the first max names go to bad. */
typedef struct { const char* name; unsigned off, size; } NdDsField;
#define DSF(nm, f) { nm, offsetof(NdDs, f), sizeof(((NdDs*)0)->f) }
#define DSW(f) { #f, offsetof(NdDs, w.f), sizeof(((NdDs*)0)->w.f) }
#define DST(f) { "tst1." #f, offsetof(NdDs, w.tst1.f), sizeof(((NdDs*)0)->w.tst1.f) } /* NST2A: one entry per NdTst member */
static const NdDsField ds_fields[] = {
    DSW(geom), DSW(omode_h), DSW(omode_l), DSW(prim), DSW(envc), DSW(fill), DSW(prim_z), DSW(om_kh), DSW(om_kl), DSW(prim_lod),
    DSW(td), DSW(tex_on), DSW(timg), DSW(tlut), DSW(tlut_n), DSW(tex_ss), DSW(tex_ts), DSW(segbase), DSW(comb_w0), DSW(comb_w1), DSW(use_t1),
    DST(timg_fmt),
    DST(timg_siz),
    DST(timg_stride),
    DST(ldrec),
    DST(blk),
    DST(tile_tmem),
    DST(tile_pal),
    DST(tile_sz),
    DST(comb2_a),
    DST(comb2_b),
    DST(comb2_c),
    DST(comb2_d),
    DST(comb2_aa),
    DST(comb2_ab),
    DST(comb2_ac),
    DST(comb2_ad),
    DST(comb_aa),
    DST(comb_ab),
    DST(comb_ac),
    DST(comb_ad),
    DST(tile_wrap),
    DST(tile_shift),
    DST(tile_fmt),
    DST(tile_siz),
    DST(comb_a),
    DST(comb_b),
    DST(comb_c),
    DST(comb_d),
    DSF("conv_k4", conv_k4), DSF("keyc", keyc), DSF("keys", keys), DSF("dual_on", dual_on), DSF("box_prod", box_prod),
    DSF("comb_set", comb_set), DSF("comb_alpha_shade", comb_alpha_shade), DSF("sci_x0", sci_x0), DSF("sci_y0", sci_y0),
    DSF("sci_x1", sci_x1), DSF("sci_y1", sci_y1), DSF("fog_fm", fog_fm), DSF("fog_fo", fog_fo), DSF("fogc", fogc),
    DSF("blendc", blendc), DSF("lt", lt), DSF("lt_n", lt_n), DSF("lk", lk), DSF("vp_sx", vp_sx), DSF("vp_sy", vp_sy),
    DSF("vp_sz", vp_sz), DSF("vp_tx", vp_tx), DSF("vp_ty", vp_ty), DSF("vp_tz", vp_tz), DSF("tm", tm),
};

static void ds_zero_draw(NdDs* s) {
    for (int n = 0; n < 2; n++) if (!s->w.td[n].valid) memset(&s->w.td[n], 0, sizeof s->w.td[n]);
    s->m1v = 0; s->m1w = 0; s->m1h = 0;
    s->w.tile_x0 = 0; s->w.tile_y0 = 0; s->w.tile_fx = 0.f; s->w.tile_fy = 0.f; s->w.cur_tile = 0;
    s->fogmix_on = 0;
}

int nd_ds_cmp(const NdDs* a, const NdDs* b, const char** bad, int max) {
    NdDs ca, cb;
    int n = 0;
    memcpy(&ca, a, sizeof ca);
    memcpy(&cb, b, sizeof cb);
    ds_zero_draw(&ca);
    ds_zero_draw(&cb);
    for (unsigned i = 0; i < sizeof ds_fields / sizeof ds_fields[0]; i++) {
        const NdDsField* f = &ds_fields[i];
        if (memcmp((const uint8_t*)&ca + f->off, (const uint8_t*)&cb + f->off, f->size) == 0) continue;
        if (n < max && bad) bad[n] = f->name;
        n++;
    }
    return n;
}

/* NST2C: a parts record (nd_parts_end layout). The list call at index pre (the table pointer) has no state; the MTX, PRIM, ENV, TILESIZE
 * and POPMTX units of each instance run in record order (nd_parts_run order: pre units, list, post units). */
void nd_ds_parts(NdDs* s, const uint32_t* u, NdDsUnitFn unit) {
    const unsigned n = u[0] & 0xffu, pre = (u[0] >> 8) & 0xffu, m = (u[0] >> 16) & 0xffu;
    const uint32_t* p = u + 2;
    for (unsigned k = 0; k < n; k++, p += 2u * m)
        for (unsigned j = 0; j < m; j++) {
            if (j == pre) { if (nd_lsc_on) { unsigned t = 0; const NdNDev* e = (const NdNDev*)(const void*)((const NdGTab*)(uintptr_t)p[2u * j + 1u])->list; for (unsigned q = 0; q < e->nu; q++) { const unsigned id = e->u[2u * q] >> 24; t += id == ND_FS_TRI1 ? 1u : id == ND_FS_TRI2 ? 2u : 0u; } /* DEL5B1 */ nd_lsc_tris(t); } continue; } /* NREC4: the list triangles of the instance */
            unit(s, p[2u * j], p[2u * j + 1u]);
            if (nd_lsc_on) nd_lsc_unit(p[2u * j]); /* NREC4 */
        }
}
/* NST2C: a compiled def (nd_scomp.c rec_ops order): value unit j takes v[2j], v[2j+1]; a sync (ND_NOP_A) is skipped. */
void nd_ds_scomp(NdDs* s, const uint32_t* du, unsigned n, uint32_t vmask, const uint32_t* v, unsigned nv, NdDsUnitFn unit) {
    unsigned j = 0;
    for (unsigned k = 0; k < n; k++) {
        uint32_t w0 = du[2u * k], w1 = du[2u * k + 1u];
        if (vmask >> k & 1u) {
            if (j >= nv) continue;
            w0 = v[2u * j]; w1 = v[2u * j + 1u]; j++;
        }
        if (w0 != ND_NOP_A) unit(s, w0, w1);
    }
}
