/* nd_tex_set.c: the texture ops of the walker and the globals they keep (target only). Split from nd_tex.c (MOVE-5): the
 * descriptor table, nd_tex_get and the unit words are host-tested there (nd_tex.c), and these need the walker state (nd_ws, the former pm_gbi.c). */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nd.h"
#include "nd_int.h"
#include "pm_gpu.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"
#include "nd_op.h"
#include "nd_api.h"

/* GLOB: definitions moved here from the former pm_gbi.c (layout only). */
NdTexM nd_tm; /* D4a: the load side of the texture model (nd_int.h) */
M1Rec m1;
unsigned shc[16]; /* BOMBLIT: shade path counters of the shadecen switch (0 m1 set, 1 m1 skipped, 2 shaded0 ok, 3 shaded0 cond false, 4 palette NULL, 5 load fail, 6 native why 3) */
/* BOMBLIT2: 7 native ok, 8 native fallback, 9 native why 5, 10 and 11 unused (memo hit and miss come from nd_sprm_take, nd_model.c), 12 shaded native draw, 13 shade_mean call */
void nd_sprm_take(unsigned* h, unsigned* m); /* nd_model.c */
int shd_min[3] = { 255, 255, 255 }, shd_max[3] = { 0, 0, 0 }; /* BOMBLIT2: dual_f range of the shaded native draws, x255 */
int pm_gbi_bind_tile1(void) { /* MDL-1b v2: the walker's pre-draw tile step (gv_epoch_enter); rec_aux already ran the record's combine and tile words */
    if (!plan_active || !cur_plan.uses_tex1) return 0;
    ensure_tile(nd_ws.use_t1 ? 1 : 0);
    dual_bind();
    return nd_ws.td[1].valid && pm_tex_aux_off_idx() < 0;
}

/* the counters of PG0 (the former pm_gbi.c gp_note) and the pipeline flush by its line (the former pm_gbi.c macros) */
#define GP_SET(k, same) do { if (gp_on()) gp_note(k, (same)); } while (0)
#define pm_gpu_flush() pm_gpu_flush_at(__LINE__)

/* ---- T globals owned by nd_tex.c (plan_s10_path D10: set by the texture setters here) ---- */
unsigned st0_ep; int st0_ok; /* PG1: tile 0 wrap state of the last G_SETTILE */
struct TszLast tsz_last;     /* PG1: the last tile 0 size record (ensure_tile in the former pm_gbi.c reads and writes it until TX-2) */
/* owner: nd_tex.c. The bind of tile n for the next draw (ensure_tile, the ens_ state the cache skip reads) */
const void* ens_img[2];
const void* ens_src;
const void* ens_pal;
unsigned ens_fmt, ens_siz;
unsigned ens_wrap[4];
int ens_sz[4];
static int ens_slot = -1; static unsigned ens_gen; /* KMR10M: cache slot and generation the last walker bind of tile 0 left in pm_tex cur */

/* The bodies of the texture ops, one function each (decoded fields). HC4c: the typed setters nd_set_* (nd_op.h fields) call them; nd_op_apply
 * (nd_set.c) runs the typed ops by id. */
/* CORE-2b batch D: texture image setter (decoded fields). */
static void set_timg(u32 addr, unsigned fmt, unsigned siz, int stride) {
    nd_ws.timg = ptr_of(addr);
    if (!addr && zslot_on && siz == 2) nd_ws.timg = zb_fake; /* B5: the underwater mesh reads the z-buffer slot */
    /* D03: a texture read from a frame buffer is the last frame. Fetch it now. */
    if (nd_ws.timg && pm_is_cfb(nd_ws.timg) && !pm_is_cur_cfb(nd_ws.timg)) pm_cfb_capture_prev(); /* GAMEOVER: the t = 0.5 pass too (it is a drawn frame with its own old frame) */ /* B2: the frame buffer being drawn is a grab (T2, grab_cur) */
    nd_ws.tst1.timg_fmt = fmt;
    nd_ws.tst1.timg_siz = siz;
    nd_ws.tst1.timg_stride = stride;
    nd_tm.ld_valid = 0;
}
void nd_set_timg(u32 a, u32 b) { set_timg(b, (a >> 21) & 7, (a >> 19) & 3, (int)(a & 0xfff) + 1); }
/* CORE-2b batch D: tile setter (decoded fields; wr = the four wrap words, sh0 and sh1 the shifts). */
static void set_tile(unsigned tile, unsigned fmt, unsigned siz, unsigned tmem, unsigned pal, const unsigned* wr, unsigned sh0, unsigned sh1) {
    int same_tile = tile == 0 && st0_ok && pm_tex_wrap_gen() == st0_ep && nd_ws.tst1.tile_fmt[0] == fmt && nd_ws.tst1.tile_siz[0] == siz && nd_ws.tst1.tile_tmem[0] == tmem && nd_ws.tst1.tile_pal[0] == pal
                  && nd_ws.tst1.tile_wrap[0][0] == wr[0] && nd_ws.tst1.tile_wrap[0][1] == wr[1] && nd_ws.tst1.tile_wrap[0][2] == wr[2] && nd_ws.tst1.tile_wrap[0][3] == wr[3]
                  && nd_ws.tst1.tile_shift[0][0] == sh0 && nd_ws.tst1.tile_shift[0][1] == sh1;
    if (tile == 0) GP_SET(3, same_tile);
    nd_ws.tst1.tile_fmt[tile] = fmt;
    nd_ws.tst1.tile_siz[tile] = siz;
    nd_ws.tst1.tile_tmem[tile] = tmem;
    nd_ws.tst1.tile_pal[tile] = pal;
    nd_ws.tst1.tile_wrap[tile][0] = wr[0];
    nd_ws.tst1.tile_wrap[tile][1] = wr[1];
    nd_ws.tst1.tile_wrap[tile][2] = wr[2];
    nd_ws.tst1.tile_wrap[tile][3] = wr[3];
    nd_ws.tst1.tile_shift[tile][0] = sh0;
    nd_ws.tst1.tile_shift[tile][1] = sh1;
    /* The render tile's wrap belongs to the triangles already queued.
     * Draw them before this tile replaces it. */
    if (tile == 0 && !same_tile) { /* PG1: an identical G_SETTILE changes nothing: no flush, no wrap call, no new texture epoch */
        int f_on = pm_ph_cen && sp0_cur == 1; unsigned f0 = f_on ? pm_ticks() : 0; /* SPLIT0C2: flush time in the tex kind */
        pm_gpu_flush();
        if (f_on) sp0_F += (unsigned)(pm_ticks() - f0);
        pm_tex_set_wrap(wr[0], wr[1], wr[2], wr[3]);
        st0_ep = pm_tex_wrap_gen(); st0_ok = 1; /* the wrap state pm_tex has now is the one of tile 0: an identical G_SETTILE may skip while nothing else touched pm_tex */
    }
}
void nd_set_tile(u32 a, u32 b) {
    unsigned wr[4] = { (b >> 8) & 3, (b >> 18) & 3, (b >> 4) & 15, (b >> 14) & 15 };
    set_tile((b >> 24) & 7, (a >> 21) & 7, (a >> 19) & 3, a & 0x1ff, (b >> 20) & 15, wr, b & 15, (b >> 10) & 15);
}
/* CORE-2b batch D2: block load setter (decoded fields: tile and texel count). */
static void set_loadblock(unsigned tile, unsigned texels) {
            unsigned tm = nd_ws.tst1.tile_tmem[tile];
            {
                /* V13: the block also covers the TMEM above it (a half-height aux tile reads there) */
                unsigned words = ((texels + 1u) << nd_ws.tst1.tile_siz[tile]) >> 4;
                for (unsigned k = 0; k < 4; k++) if (nd_ws.tst1.ldrec[k].tmem >= tm && (nd_ws.tst1.ldrec[k].tmem == tm || nd_ws.tst1.ldrec[k].tmem < tm + words)) nd_ws.tst1.ldrec[k].img = NULL;
            }
            {
                unsigned k;
                for (k = 0; k < 4 && !(nd_ws.tst1.blk[k].img && nd_ws.tst1.blk[k].tmem == tm); k++) {}
                if (k == 4) k = nd_tm.blk_n++ & 3;
                nd_ws.tst1.blk[k].tmem = tm; nd_ws.tst1.blk[k].img = nd_ws.timg;
                nd_ws.tst1.blk[k].bytes = ((texels + 1u) << nd_ws.tst1.tile_siz[tile]) >> 1;
            }
}
void nd_set_loadblock(u32 b) { set_loadblock((b >> 24) & 7, (b >> 12) & 0xfff); }
/* D4a-2: tile load setter (decoded fields: tile and the 12 bit window of G_LOADTILE): the LoadTile window and the load record of its TMEM address. */
static void set_loadtile(unsigned tile, int uls, int ult, int lrs, int lrt) {
    nd_tm.ld_valid = 1;
    nd_tm.ld_x = uls >> 2;
    {
        unsigned tm = nd_ws.tst1.tile_tmem[tile], k;
        for (k = 0; k < 4 && !(nd_ws.tst1.ldrec[k].img && nd_ws.tst1.ldrec[k].tmem == tm); k++) {}
        if (k == 4) k = nd_tm.ldrec_n++ & 3;
        nd_ws.tst1.ldrec[k].tmem = tm; nd_ws.tst1.ldrec[k].img = nd_ws.timg; nd_ws.tst1.ldrec[k].stride = nd_ws.tst1.timg_stride;
        nd_ws.tst1.ldrec[k].x = uls >> 2; nd_ws.tst1.ldrec[k].y = ult >> 2;
    }
    nd_tm.ld_y = ult >> 2;
    nd_tm.ld_h = ((lrt - ult) >> 2) + 1;
    nd_tm.ld_w = ((lrs - uls) >> 2) + 1;
}
/* CORE-2b batch D2: palette load setter (decoded fields: tile and entry count). */
static void set_loadtlut(unsigned tile, unsigned count) {
            nd_ws.tlut = nd_ws.timg;
            nd_ws.tlut_n = (int)(count) + 1;
            {
                /* The palette slot is the 16-entry block at the load tile's TMEM address. */
                unsigned slot = (nd_ws.tst1.tile_tmem[tile] - 256u) >> 4;
                if (slot < 16) {
                    nd_tm.tlut_pal[slot] = nd_ws.timg;
                    /* V20: gDPLoadTextureBlock_4b sets the tile size (which binds the texture) BEFORE the palette load. A bound CI tile that
                     * uses this palette bank must take the new palette, else it keeps the one of the previous model (the brick block was
                     * drawn with a dark grey palette). */
                    tlut_v20(slot);
                }
            }
}
void nd_set_loadtlut(u32 b) { set_loadtlut((b >> 24) & 7, (b >> 14) & 0x3ff); }
void nd_set_loadtile(u32 a, u32 b) { set_loadtile((b >> 24) & 7, (int)((a >> 12) & 0xfff), (int)(a & 0xfff), (int)((b >> 12) & 0xfff), (int)(b & 0xfff)); }
/* CORE-2b batch D: tile size setter (decoded fields). */
static __attribute__((noinline)) void set_tilesize(unsigned tile, int uls, int ult, int lrs, int lrt) {
    if (tile == 0 && PMDL("negpanf") && pm_frame_count() >= 200u && (nd_ws.tst1.tile_wrap[0][2] || nd_ws.tst1.tile_wrap[0][3])) { /* NEGPAN step 1 debug: force the mushroom panner offset (-0x8000 = -128 in 10.2) on 8 tile-0 sizes */
        static unsigned nf_;
        if (nf_ < 8u) { nf_++; ult = (ult - 128) & 0xfff; lrt = (lrt - 128) & 0xfff; } /* raw 12 bit fields, as the record carries them */
    }
            if (((lrs - uls) >> 2) + 1 <= 0 || ((lrt - ult) >> 2) + 1 <= 0) tex_win_s25(&uls, &ult, &lrs, &lrt, nd_ws.tst1.tile_wrap[tile][0], nd_ws.tst1.tile_wrap[tile][2], nd_ws.tst1.tile_wrap[tile][1], nd_ws.tst1.tile_wrap[tile][3]);
            GP_SET(4, nd_ws.tst1.tile_sz[tile][0] == uls && nd_ws.tst1.tile_sz[tile][1] == ult && nd_ws.tst1.tile_sz[tile][2] == lrs && nd_ws.tst1.tile_sz[tile][3] == lrt);
            nd_ws.tst1.tile_sz[tile][0] = uls;
            nd_ws.tst1.tile_sz[tile][1] = ult;
            nd_ws.tst1.tile_sz[tile][2] = lrs;
            nd_ws.tst1.tile_sz[tile][3] = lrt;
            if (tile == 1) { tex_tile1(tile, uls, ult, lrs, lrt); sync_combine(); } /* S25: a plan that reads TEXEL1 needs tile 1 */
            if (tile != 0) return;
            nd_ws.td[0].valid = 0;
            nd_ws.cur_tile = -1; /* GP3: defer pm_tex_load to ensure_tile(0) at first draw */
            if (dual_on) { dual_on = 0; sync_combine(); } /* S25: the plan path is off while the shaded sprite is on */
            if (m1.valid && tex_shaded0(uls, ult)) return;
            m1.valid = 0;
            int x0 = uls >> 2;
            int y0 = ult >> 2;
            NdWin wn;
            tex_win0(&nd_ws.tst1, &nd_tm, nd_ws.timg, uls, ult, lrs, lrt, &wn);
            const u8* src = wn.src;
            int w = wn.w, h = wn.h, stride = wn.stride;
            nd_ws.tile_x0 = x0;
            nd_ws.tile_y0 = y0;
            nd_ws.tile_fx = (float)(uls & 3) * 0.25f; /* PT38 */
            nd_ws.tile_fy = (float)(ult & 3) * 0.25f;
            /* PG1: the same texture again (same source, format, size, palette, stride) and nothing touched the texture state since the last load (pm_tex_epoch):
             * the load would find the same cache slot. Skip the flush and the load, the batch in flight samples the same image. A frame buffer image is
             * excluded (its content changes under the same pointer, grab_cur). */
            {
                struct TszLast* lp = &tsz_last;
                const void* palc = nd_tm.tlut_pal[nd_ws.tst1.tile_pal[0] & 15] ? nd_tm.tlut_pal[nd_ws.tst1.tile_pal[0] & 15] : nd_ws.tlut;
                int ggx = 0, ggy = 0;
                if (src && w > 0 && h > 0 && nd_ws.tst1.tile_fmt[0] == 2 && nd_ws.tst1.tile_siz[0] == 0 &&
                    pm_tex_glyph_same(src, nd_ws.tst1.tile_fmt[0], nd_ws.tst1.tile_siz[0], w, h, palc, nd_ws.tlut_n, &ggx, &ggy)) {
                    nd_ws.tile_x0 = x0 - ggx; /* same sign as ensure_tile's new-bind path (x0 - gox) */
                    nd_ws.tile_y0 = y0 - ggy;
                    nd_ws.tile_fx = (float)(uls & 3) * 0.25f;
                    nd_ws.tile_fy = (float)(ult & 3) * 0.25f;
                    nd_ws.td[0].valid = 1; nd_ws.td[0].src = src; nd_ws.td[0].fmt = nd_ws.tst1.tile_fmt[0]; nd_ws.td[0].siz = nd_ws.tst1.tile_siz[0]; nd_ws.td[0].w = w; nd_ws.td[0].h = h; nd_ws.td[0].stride = stride;
                    nd_ws.td[0].x0 = x0; nd_ws.td[0].y0 = y0; nd_ws.td[0].pal = palc; nd_ws.td[0].pal_n = nd_ws.tlut_n;
                    gp3_t0_set(); GP_SET(7, 1);
                    return;
                }
                if (lp->ok && src == lp->src && nd_ws.tst1.tile_fmt[0] == lp->fmt && nd_ws.tst1.tile_siz[0] == lp->siz && w == lp->w && h == lp->h && stride == lp->stride &&
                    palc == lp->pal && nd_ws.tlut_n == lp->pal_n && pm_tex_epoch() == lp->ep && !pm_is_cfb(src) && src && w > 0 && h > 1) {
                    nd_ws.tile_x0 = x0 - lp->ox;
                    nd_ws.tile_y0 = y0 - lp->oy;
                    nd_ws.td[0].valid = 1; nd_ws.td[0].src = src; nd_ws.td[0].fmt = nd_ws.tst1.tile_fmt[0]; nd_ws.td[0].siz = nd_ws.tst1.tile_siz[0]; nd_ws.td[0].w = w; nd_ws.td[0].h = h; nd_ws.td[0].stride = stride;
                    nd_ws.td[0].x0 = x0; nd_ws.td[0].y0 = y0; nd_ws.td[0].pal = palc; nd_ws.td[0].pal_n = nd_ws.tlut_n;
                    gp3_t0_set(); GP_SET(7, 1);
                    return;
                }
                GP_SET(7, 0);
                lp->ok = 0;
                lp->src = src; lp->fmt = nd_ws.tst1.tile_fmt[0]; lp->siz = nd_ws.tst1.tile_siz[0]; lp->w = w; lp->h = h; lp->stride = stride; lp->pal = palc; lp->pal_n = nd_ws.tlut_n;
            }
            /* GP3: pm_tex_load runs in ensure_tile(0) at the first surviving draw. */
            /* F03: a 1 row tile of the N64 z-buffer is a depth query source (pm_depth_query), not a texture (the pause background loads tall z-buffer tiles, those are textures) */
            if (src && w > 0 && h > 0 && (h > 1 || (unsigned)nd_ws.timg - (unsigned)nuGfxZBuffer >= 320u * 240u * 2u)) {
                const void* pal0 = nd_tm.tlut_pal[nd_ws.tst1.tile_pal[0] & 15] ? nd_tm.tlut_pal[nd_ws.tst1.tile_pal[0] & 15] : nd_ws.tlut;
                nd_ws.td[0].valid = 1;
                nd_ws.td[0].src = src;
                nd_ws.td[0].fmt = nd_ws.tst1.tile_fmt[0];
                nd_ws.td[0].siz = nd_ws.tst1.tile_siz[0];
                nd_ws.td[0].w = w;
                nd_ws.td[0].h = h;
                nd_ws.td[0].stride = stride;
                nd_ws.td[0].x0 = x0;
                nd_ws.td[0].y0 = y0;
                nd_ws.td[0].pal = pal0;
                nd_ws.td[0].pal_n = nd_ws.tlut_n;
                gp3_t0_set();
            }
            return;
}
__attribute__((noinline)) void nd_set_tilesize(u32 a, u32 b) {
    set_tilesize((b >> 24) & 7, (int)((a >> 12) & 0xfff), (int)(a & 0xfff), (int)((b >> 12) & 0xfff), (int)(b & 0xfff));
}
/* CORE-2b batch D2: texture enable and scale setter (decoded fields). */
static void set_texture(unsigned on, unsigned ss, unsigned ts) {
    nd_ws.tex_on = on;
    nd_ws.tex_ss = ss;
    nd_ws.tex_ts = ts;
}
void nd_set_texture(u32 a, u32 b) { set_texture(a, b >> 16, b & 0xffffu); }

/* DIRECT D4b-1, D5: NDC_TEXRECT (nd_cmd.h), recorded by the nd_gbi.h redirect: the walker's op prologue of G_TEXRECT (gv_epoch,
 * pm_gbi_cur_op), then the rectangle. */
#define TX_OP(op) do { gv_epoch++; pm_gbi_cur_op = (op); } while (0)
void nd_texrect_run(const uint32_t* p) {
    TX_OP(G_TEXRECT);
    nd_texrect(p[2], p[3], p[4], p[5]);
}
#undef TX_OP

void unpack8(unsigned c, float o[4]) {
    o[0] = pm_f255[(c >> 24) & 255u];
    o[1] = pm_f255[(c >> 16) & 255u];
    o[2] = pm_f255[(c >> 8) & 255u];
    o[3] = pm_f255[c & 255u];
}

void pal_rgba(const void* base, unsigned i, float o[4]) {
    const u8* p = (const u8*)base;
    unsigned c = 0;
    if (p) {
        p += (i & 15u) * 2u;
        c = ((unsigned)p[0] << 8) | p[1];
    }
    float r = (float)((c >> 11) & 31u) / 31.f, g = (float)((c >> 6) & 31u) / 31.f,
          b = (float)((c >> 1) & 31u) / 31.f;
    o[0] = r; o[1] = g; o[2] = b; o[3] = (c & 1u) ? 1.f : 0.f;
}

const u8* fb_prev_base(int n) {
    const u8* base;
    unsigned i;
    if (pm_wide || !nd_ws.td[n].valid || nd_ws.td[n].fmt != 0 || nd_ws.td[n].siz != 2 || nd_ws.td[n].stride != 320) return NULL;
    if (nd_ws.tst1.tile_wrap[n][2] || nd_ws.tst1.tile_wrap[n][3]) return NULL;
    base = (const u8*)nd_ws.td[n].src - ((size_t)nd_ws.td[n].y0 * 320u + (size_t)nd_ws.td[n].x0) * 2u;
    for (i = 0; i < nuGfxCfbNum; i++)
        if ((const u8*)nuGfxCfb[i] == base) break;
    if (i == nuGfxCfbNum || pm_is_cur_cfb(base)) return NULL;
    return base;
}

void ens_clear(void) { ens_src = NULL; ens_pal = NULL; ens_slot = -1; }

/* KMR10M debug: refusals of a matching bind; the first and every 8th is logged with its frame and tile (cold: off the bind path) */
static __attribute__((cold, noinline)) void tile_stale_log(int n) {
    static unsigned stale_n;
    stale_n++;
    if (stale_n == 1 || !(stale_n & 7)) pm_log_d("ensstale f%u n %u tile %d ens_slot %d cur %d", pm_frame_count(), stale_n, n, ens_slot, pm_tex_cur_idx());
}

/* NAPI2B: the bind test on the facts of a bind (a walker tile or a direct descriptor): the texture layer holds this image, palette, format, wrap and window */
static inline __attribute__((always_inline)) int bound_cmp(int valid, const void* src, const void* pal, unsigned fmt, unsigned siz, const unsigned* wrap, const int* sz, int n) {
    if (!valid || pm_tex_cur_idx() < 0 || !ens_src) return 0;
    if (ens_src != src || ens_pal != pal || ens_fmt != fmt || ens_siz != siz) return 0;
    if (memcmp(wrap, ens_wrap, sizeof ens_wrap) || memcmp(sz, ens_sz, sizeof ens_sz)) return 0;
    /* KMR10M: a native load since the bind (nd_tex_get, spr_tex_unit) moves cur; the bind holds only while its slot keeps its generation */
    if (pm_tex_cur_idx() != ens_slot || pm_tex_slot_gen(ens_slot) != ens_gen) {
        tile_stale_log(n);
        return 0;
    }
    return 1;
}

int tile_bound(int n) {
    return bound_cmp(nd_ws.td[n].valid, nd_ws.td[n].src, nd_ws.td[n].pal, nd_ws.td[n].fmt, nd_ws.td[n].siz, nd_ws.tst1.tile_wrap[n], nd_ws.tst1.tile_sz[n], n);
}

int tile_texel(int n, int s, int t, unsigned char o[4]) {
    const u8* p = nd_ws.td[n].src;
    int w = nd_ws.td[n].w, h = nd_ws.td[n].h;
    if (n == 0) {
        s %= w; if (s < 0) s += w;
        t %= h; if (t < 0) t += h;
    } else {
        if (s < 0) s = 0; else if (s >= w) s = w - 1;
        if (t < 0) t = 0; else if (t >= h) t = h - 1;
    }
    size_t i = (size_t)t * (size_t)nd_ws.td[n].stride + (size_t)s;
    unsigned c;
    switch (nd_ws.td[n].fmt * 4 + nd_ws.td[n].siz) {
    case 4 * 4 + 0: { u8 b = p[i >> 1]; unsigned v = ((i & 1) ? (b & 15u) : (b >> 4)) * 17u; o[0] = o[1] = o[2] = o[3] = (u8)v; return 1; }
    case 4 * 4 + 1: o[0] = o[1] = o[2] = o[3] = p[i]; return 1;
    case 3 * 4 + 0: { u8 b = p[i >> 1]; unsigned v = (i & 1) ? (b & 15u) : (b >> 4); o[0] = o[1] = o[2] = (u8)((v >> 1) * 36u); o[3] = (v & 1u) ? 255 : 0; return 1; }
    case 3 * 4 + 1: o[0] = o[1] = o[2] = (u8)((p[i] >> 4) * 17u); o[3] = (u8)((p[i] & 15u) * 17u); return 1;
    case 3 * 4 + 2: o[0] = o[1] = o[2] = p[i * 2]; o[3] = p[i * 2 + 1]; return 1;
    case 0 * 4 + 2: c = ((unsigned)p[i * 2] << 8) | p[i * 2 + 1]; break;
    case 2 * 4 + 0: case 2 * 4 + 1: {
        unsigned idx;
        if (!nd_ws.td[n].pal) return 0;
        if (nd_ws.td[n].siz == 0) { u8 b = p[i >> 1]; idx = (i & 1) ? (b & 15u) : (b >> 4); } else idx = p[i];
        c = ((unsigned)((const u8*)nd_ws.td[n].pal)[idx * 2] << 8) | ((const u8*)nd_ws.td[n].pal)[idx * 2 + 1];
        break;
    }
    default: return 0;
    }
    o[0] = (u8)(((c >> 11) & 31u) * 255u / 31u);
    o[1] = (u8)(((c >> 6) & 31u) * 255u / 31u);
    o[2] = (u8)(((c >> 1) & 31u) * 255u / 31u);
    o[3] = (c & 1u) ? 255 : 0;
    return 1;
}

/* HC7: the frame buffer candidate of fb_prev_base (a 16 bit RGBA 320 wide window): it takes the cold path, the bind is decided there */
static inline int fb_cand(int n) {
    return nd_ws.td[n].fmt == 0 && nd_ws.td[n].siz == 2; /* HC7: every 16 bit RGBA bind takes the slow path (frame buffer candidate, fbbind log) */
}

static __attribute__((cold, noinline)) void ensure_tile_slow(int n) {
    if (!nd_ws.td[n].valid) return;
    nd_ws.tile_fx = (float)(nd_ws.tst1.tile_sz[n][0] & 3) * 0.25f; /* PT38 */
    nd_ws.tile_fy = (float)(nd_ws.tst1.tile_sz[n][1] & 3) * 0.25f;
    { /* FB1 debug switch fbbind: one line per 16 bit RGBA bind (source, window, size); pm_tex.c load_impl adds hit or miss */
        static int fbon = -1;
        if (fbon < 0) fbon = pm_debug_has("fbbind");
        if (fbon && nd_ws.td[n].fmt == 0 && nd_ws.td[n].siz == 2) pm_log_d("fbbind n%d src %p xy %d,%d wh %dx%d st %d", n, (const void*)nd_ws.td[n].src, nd_ws.td[n].x0, nd_ws.td[n].y0, nd_ws.td[n].w, nd_ws.td[n].h, nd_ws.td[n].stride);
    }
    {   /* FB1: a clamp window of the previous colour buffer (the transition strips, 6 rows each) samples one texture of the whole 320 x 240
         * buffer. The key is the buffer, so every strip after the first is a bind hit (no flush, no load). Texels are absolute: tile_x0 = tile_y0 = 0. */
        const u8* fb = fb_prev_base(n);
        if (fb) {
            static const int fb_sz[4] = { -1, -1, -1, -1 }; /* a window size no SetTileSize writes: tile_bound never matches it */
            if (ens_src == fb && ens_pal == nd_ws.td[n].pal && ens_fmt == 0 && ens_siz == 2 && pm_tex_cur_idx() >= 0 &&
                !memcmp(ens_sz, fb_sz, sizeof ens_sz) && !memcmp(nd_ws.tst1.tile_wrap[n], ens_wrap, sizeof ens_wrap)) {
                nd_ws.tile_x0 = nd_ws.tile_y0 = 0;
                nd_ws.cur_tile = n;
                return;
            }
            pm_gpu_flush();
            pm_tex_set_wrap(nd_ws.tst1.tile_wrap[n][0], nd_ws.tst1.tile_wrap[n][1], nd_ws.tst1.tile_wrap[n][2], nd_ws.tst1.tile_wrap[n][3]);
            pm_tex_load(fb, 0, 2, 320, 240, nd_ws.td[n].pal, nd_ws.td[n].pal_n, 320);
            { /* FB1 debug switch fbbind: the bind as the texture layer gives it: size (pm_tex_size), source, slot (off) or CPU texture (cur) */
                static int fbon = -1;
                int tw1 = 0, th1 = 0;
                if (fbon < 0) fbon = pm_debug_has("fbbind");
                if (fbon) { pm_tex_size(&tw1, &th1); pm_log_d("fbbind bind n%d fb %p fmt RGBA16 size %dx%d off %d cur %d", n, (const void*)fb, tw1, th1, pm_tex_off_idx(), pm_tex_cur_idx()); }
            }
            nd_ws.tile_x0 = nd_ws.tile_y0 = 0;
            ens_src = fb; ens_pal = nd_ws.td[n].pal; ens_fmt = 0; ens_siz = 2; ens_slot = -1;
            memcpy(ens_wrap, nd_ws.tst1.tile_wrap[n], sizeof ens_wrap);
            memcpy(ens_sz, fb_sz, sizeof ens_sz);
            ens_img[n] = fb;
            nd_ws.cur_tile = n;
            return;
        }
    }
    if (n == 0 && !tile_bound(0) && glyph_atlas_bound()) {
        nd_ws.cur_tile = 0;
        return;
    }
    {
        NdBindDesc d;
        d.src = nd_ws.td[n].src; d.pal = nd_ws.td[n].pal; d.pal_n = nd_ws.td[n].pal_n; d.fmt = nd_ws.td[n].fmt; d.siz = nd_ws.td[n].siz;
        d.stride = nd_ws.td[n].stride; d.x0 = nd_ws.td[n].x0; d.y0 = nd_ws.td[n].y0; d.w = nd_ws.td[n].w; d.h = nd_ws.td[n].h;
        memcpy(d.wrap, nd_ws.tst1.tile_wrap[n], sizeof d.wrap);
        memcpy(d.sz, nd_ws.tst1.tile_sz[n], sizeof d.sz);
        nd_tex_bind_desc(&d, n);
    }
}

/* NAPI2B: the bind of one texture window to tile n from a descriptor (ensure_tile_slow builds it from nd_ws.td[n] and tst1; a direct executor states it).
 * The tile_bound arm, the GAMEOVER re-window arm and the load arm of the former ensure_tile_slow, in the same order with the same calls.
 * Returns 1 when the texture layer already held the image (nothing loaded), 0 when it was re-windowed or loaded. */
int nd_tex_bind_desc(const NdBindDesc* d, int n) {
    if (bound_cmp(1, d->src, d->pal, d->fmt, d->siz, d->wrap, d->sz, n)) {
        nd_ws.cur_tile = n;
        return 1;
    }
    /* GAMEOVER: a window of the off-screen image bound now (the 40 six-row strips of the previous-frame blur, one load each) samples the same
     * texture: only the origin moves, so the queued rectangles stay in one batch (it was one flush and one draw per strip). Not the frame buffer
     * being drawn: grab_cur decides a new copy for it. */
    if (pm_tex_off_idx() >= 0 && ens_src && ens_pal == d->pal && ens_fmt == d->fmt && ens_siz == d->siz &&
        !memcmp(d->wrap, ens_wrap, sizeof ens_wrap) && !pm_is_cur_cfb(d->src) &&
        pm_tex_off_rewin(d->src, d->fmt, d->siz)) {
        int ox, oy, gox, goy;
        pm_tex_origin(&ox, &oy);
        pm_tex_glyph_ofs(&gox, &goy);
        nd_ws.tile_x0 = d->x0 - ox - (n == 0 ? gox : 0);
        nd_ws.tile_y0 = d->y0 - oy - (n == 0 ? goy : 0);
        ens_src = d->src; ens_slot = -1;
        memcpy(ens_sz, d->sz, sizeof ens_sz);
        ens_img[n] = d->src;
        nd_ws.cur_tile = n;
        return 0;
    }
    pm_gpu_flush();
    pm_tex_set_wrap(d->wrap[0], d->wrap[1], d->wrap[2], d->wrap[3]);
    if (d->fmt == 0 && d->siz == 2) grab_cur(d->src, d->w, d->h);
    pm_tex_load(d->src, d->fmt, d->siz, d->w, d->h, d->pal, d->pal_n, d->stride);
    {   /* V05: a window of an off-screen texture starts at (ox, oy) in it */
        int ox, oy, gox, goy;
        pm_tex_origin(&ox, &oy);
        pm_tex_glyph_ofs(&gox, &goy);
        nd_ws.tile_x0 = d->x0 - ox - (n == 0 ? gox : 0);
        nd_ws.tile_y0 = d->y0 - oy - (n == 0 ? goy : 0);
        if (n == 0) {
            tsz_last.ox = ox - gox;
            tsz_last.oy = oy - goy;
            tsz_last.ep = pm_tex_epoch();
            tsz_last.ok = pm_tex_off_idx() < 0;
        }
    }
    ens_src = d->src;
    ens_pal = d->pal;
    ens_fmt = d->fmt;
    ens_siz = d->siz;
    ens_slot = pm_tex_cur_idx(); ens_gen = pm_tex_slot_gen(ens_slot); /* KMR10M */
    memcpy(ens_wrap, d->wrap, sizeof ens_wrap);
    memcpy(ens_sz, d->sz, sizeof ens_sz);
    ens_img[n] = d->src;
    nd_ws.cur_tile = n;
    return 0;
}

void ensure_tile(int n) {
    if (nd_ws.td[n].valid && tile_bound(n) && !fb_cand(n)) {
        nd_ws.tile_fx = (float)(nd_ws.tst1.tile_sz[n][0] & 3) * 0.25f; /* PT38 */
        nd_ws.tile_fy = (float)(nd_ws.tst1.tile_sz[n][1] & 3) * 0.25f;
        nd_ws.cur_tile = n;
        return;
    }
    ensure_tile_slow(n);
}

/* D4a-1: the tile 1 window of a SetTileSize (source, size, stride, origin column): pure, as tex_win0. */
static void tex_win1(const NdTst* t, const NdTexM* m, const void* timg, int uls, int ult, int lrs, int lrt, NdWin* o) {
    int w = ((lrs - uls) >> 2) + 1, h = ((lrt - ult) >> 2) + 1;
    if (t->tile_wrap[1][2] && !(t->tile_wrap[1][0] & 2) && m->ld_valid) { /* VIS3: the same as PT09 for tile 1. The RDP reads (s - sl) through the mask, a mirrored or wrapped read of a window to the right of the image start (walking dust frame i + 1) needs the sheet from its start. */
        int per = 1 << t->tile_wrap[1][2];
        int ldw = t->tile_siz[1] == 0 ? m->ld_w * 2 : m->ld_w;
        if (per > w && per <= 256 && ldw > w) w = ldw < per ? ldw : per;
    }
    if (t->tile_wrap[1][2] && !(t->tile_wrap[1][0] & 2) && m->ld_valid) { const int per = 1 << t->tile_wrap[1][2]; const int ldw = t->tile_siz[1] == 0 ? m->ld_w * 2 : m->ld_w; if (per <= 256 && ldw >= per) w = per; }
    if (t->tile_wrap[1][3] && !(t->tile_wrap[1][1] & 2) && m->ld_valid) { const int per = 1 << t->tile_wrap[1][3]; if (per <= 256 && m->ld_h >= per) h = per; }
    /* TILEWIN2: a wrapping axis reads (s - sl) & mask from TMEM: one period is bound, whatever the window (effect_63, whirlwind) */
    int stride = t->timg_stride > w ? t->timg_stride : w;
    int bpp = t->tile_siz[1] >= 3 ? 4 : t->tile_siz[1] == 2 ? 2 : t->tile_siz[1] == 0 ? 0 : 1;
    const u8* src = (const u8*)timg;
    int l1x = m->ld_x, l1y = m->ld_y, l1stride = t->timg_stride, l1v = m->ld_valid;
    for (unsigned k = 0; k < 4; k++) {
        if (t->ldrec[k].img && t->ldrec[k].tmem == t->tile_tmem[1]) {
            src = (const u8*)t->ldrec[k].img; l1x = t->ldrec[k].x; l1y = t->ldrec[k].y; l1stride = t->ldrec[k].stride; l1v = 1;
            stride = l1stride > w ? l1stride : w;
        }
    }
    int t1x = uls >> 2; /* texel column of the window origin; the V21 branch lowers it to the loaded origin */
    int boff = 0; /* V13: tile 1 inside an earlier block load: the image start moves by the TMEM difference */
    for (unsigned k = 0; k < 4; k++)
        if (t->blk[k].img && t->blk[k].tmem < t->tile_tmem[1] && (t->tile_tmem[1] - t->blk[k].tmem) * 8u < t->blk[k].bytes && t->blk[k].tmem == t->tile_tmem[0] && src == (const u8*)t->blk[k].img)
            boff = (int)((t->tile_tmem[1] - t->blk[k].tmem) * 8u);
    /* The image offset is the LoadTile origin. SetTileSize uls and ult only place
     * it: gDPLoadMultiTile (draw_box corners) loads at 0,0 and moves tiles 1 to 3
     * to the far side of the window by SetTileSize. */
    if (src && bpp && l1v) src += (size_t)(l1y * stride + l1x) * (size_t)bpp;
    else if (src && bpp) src += (size_t)boff; /* V13: a block load puts the image at TMEM start, SetTileSize only moves the coordinates */
    else if (src && t->tile_siz[1] == 0 && !l1v) src += (size_t)boff; /* PT82: the same for a 4 bit image (gDPScrollTextureBlockHalfHeight_4b: tile 1 is the lower half of one block load, mim_darkgreen2tif) */
    else if (src && l1v && t->tile_wrap[1][2] && t->tile_wrap[1][3] &&
             w == (1 << t->tile_wrap[1][2]) && h == (1 << t->tile_wrap[1][3])) {
        /* WALLT1: the window is the whole mask period of the loaded image (aux pan: hos_01 walls; model aux offset: kmr_10 cliffs).
         * The RDP reads (s - sl) & mask from what LoadTile put in TMEM: the source is the LoadTile origin, and SetTileSize
         * only moves the coordinates (o->x0 and y0 in tex_tile1, as V13). Moving the source by the window origin read past the raster. */
        src += (size_t)(l1y * l1stride + l1x);
        stride = l1stride * 2;
    }
    else if (src && l1v && ((uls >> 2) >= m->ld_w * 2 || (ult >> 2) >= m->ld_h)) {
        /* COINSTAR: a window origin outside the loaded 4-bit image is a coordinate offset only (item icons in a shaded map: tile 1 at 256,256
         * over a 24 x 24 load, item_entity.c:990). The source is the LoadTile origin, as tile 0 (V13); x0 keeps uls >> 2. */
        src += (size_t)(l1y * l1stride + l1x);
        stride = l1stride * 2;
    }
    else if (src && l1v && t->tile_wrap[1][2] && t->tile_wrap[1][3] && (1 << t->tile_wrap[1][2]) <= m->ld_w * 2 && (1 << t->tile_wrap[1][3]) <= m->ld_h) {
        /* TILEWIN: a window inside one wrap period of a 4 bit image loaded whole (effect_63, fire_breath, whirlwind tile 1 scroll): bind the
         * whole period at the LoadTile origin; SetTileSize only moves the coordinates (x0 = uls >> 2, td[1].y0 = ult >> 2), as WALLT1.
         * Before, the source moved with the window: a new texture for every scroll step. */
        w = 1 << t->tile_wrap[1][2]; h = 1 << t->tile_wrap[1][3];
        src += (size_t)(l1y * l1stride + l1x); stride = l1stride * 2;
    }
    else if (src && l1v) {
        /* V21: SetTileSize uls and ult are the window origin in the image (fire_breath scrolls tile 1 by imgFrame*32+32).
         * This branch is 4 bit only (bpp 0). uls >> 2 counts texels, two per byte, so the byte offset is half of it.
         * A byte pointer cannot start on an odd texel: the loaded origin is the even texel at or before the window
         * origin. td[1].x0 uses the same origin, so the UV map and the loaded data agree. */
        t1x = (uls >> 2) & ~1;
        src += (size_t)(ult >> 2) * (size_t)l1stride + (size_t)(t1x >> 1);
        stride = l1stride * 2;
    }
    o->src = src; o->w = w; o->h = h; o->stride = stride; o->x0 = t1x;
}
__attribute__((noinline)) void tex_tile1(unsigned tile, int uls, int ult, int lrs, int lrt) {
            if (__builtin_expect(PMDL("shadecen") && tile == 1 && nd_ws.tst1.tile_fmt[1] == 2 && nd_ws.tst1.tile_siz[1] == 0 && !nd_tm.ld_valid, 0)) shc[1]++;
            if (tile == 1 && nd_ws.tst1.tile_fmt[1] == 2 && nd_ws.tst1.tile_siz[1] == 0 && nd_tm.ld_valid && nd_ws.timg) {
                /* Second tile on the same 4-bit image: remember the raster for the shaded sprite. */
                m1.valid = 1;
                if (PMDL("shadecen")) shc[0]++;
                m1.src = (const u8*)nd_ws.timg + (size_t)nd_tm.ld_y * (size_t)nd_ws.tst1.timg_stride + (size_t)nd_tm.ld_x;
                m1.stride = nd_ws.tst1.timg_stride * 2;
                m1.w = ((lrs - uls) >> 2) + 1;
                m1.h = ((lrt - ult) >> 2) + 1;
                m1.x0 = uls >> 2;
                m1.y0 = ult >> 2;
            }
            if (tile == 1) {
                NdWin wn;
                tex_win1(&nd_ws.tst1, &nd_tm, nd_ws.timg, uls, ult, lrs, lrt, &wn);
                if (__builtin_expect(PMDL("t1win") && (uls >= 1024 || ult >= 1024), 0)) { static unsigned n_; if (n_ < 8u) { n_++; pm_log("t1win f%u uls %d ult %d w %d h %d ldw %d ldh %d off %d", pm_frame_count(), uls, ult, wn.w, wn.h, nd_tm.ld_w, nd_tm.ld_h, wn.src && nd_ws.timg ? (int)(wn.src - (const u8*)nd_ws.timg) : -1); } }
                nd_ws.td[1].valid = 0;
                if (nd_ws.cur_tile == 1) nd_ws.cur_tile = -1; /* a new tile 1 image: bind it again */
                if (wn.src && wn.w > 0 && wn.h > 0) {
                    nd_ws.td[1].valid = 1;
                    nd_ws.td[1].src = wn.src;
                    nd_ws.td[1].fmt = nd_ws.tst1.tile_fmt[1];
                    nd_ws.td[1].siz = nd_ws.tst1.tile_siz[1];
                    nd_ws.td[1].w = wn.w;
                    nd_ws.td[1].h = wn.h;
                    nd_ws.td[1].stride = wn.stride;
                    nd_ws.td[1].x0 = wn.x0;
                    nd_ws.td[1].y0 = ult >> 2;
                    nd_ws.td[1].pal = nd_tm.tlut_pal[nd_ws.tst1.tile_pal[1] & 15] ? nd_tm.tlut_pal[nd_ws.tst1.tile_pal[1] & 15] : nd_ws.tlut;
                    nd_ws.td[1].pal_n = nd_ws.tlut_n;
                }
            }
}

__attribute__((noinline)) void tlut_v20(unsigned slot) {
                    for (int n = 0; n < 2; n++)
                        if (nd_ws.td[n].valid && nd_ws.td[n].fmt == 2 && (nd_ws.tst1.tile_pal[n] & 15) == slot && nd_ws.td[n].pal != nd_ws.timg) {
                            if (!pm_tex_cell_bound()) pm_gpu_flush(); /* MSGBOX 3: the queued glyphs sample only their own cells on unit 0, which a new palette does not change (flushlog: a stale tile 1 record split every rainbow glyph) */
                            nd_ws.td[n].pal = nd_ws.timg;
                            nd_ws.td[n].pal_n = nd_ws.tlut_n;
                            if (n == nd_ws.cur_tile) nd_ws.cur_tile = -1;
                        }
}

__attribute__((cold, noinline)) int tex_shaded0(int uls, int ult) {
            if (m1.valid && nd_ws.tst1.tile_fmt[0] == 2 && nd_ws.tst1.tile_siz[0] == 0 && nd_ws.tst1.tile_pal[0] != nd_ws.tst1.tile_pal[1] &&
                nd_ws.tst1.comb_a == G_CCMUX_TEXEL0 && nd_ws.tst1.comb_c == G_CCMUX_TEXEL1 &&
                m1.w > 0 && m1.h > 0) {
                /* Tile 0 starts 256 texels in (the vertices carry that); the light offset moves only the shade index, which is one colour now (KZN04). */
                (void)uls; (void)ult;
                m1.valid = 0;
                pm_gpu_flush();
                nd_ws.tile_x0 = m1.x0;
                nd_ws.tile_y0 = m1.y0;
                nd_ws.tile_fx = nd_ws.tile_fy = 0.f;
                if (load_shaded_sprite()) {
                    dual_on = 1;
                    sync_combine();
                    if (PMDL("shadecen")) shc[2]++;
                    return 1;
                }
            } else if (PMDL("shadecen")) shc[3]++;
    return 0;
}

int nd_tex_neg_start(int s, unsigned flags, unsigned mask); /* nd_tex.c: the period mapping of a negative start (host tested) */
__attribute__((cold, noinline)) void tex_win_s25(int* p_uls, int* p_ult, int* p_lrs, int* p_lrt, unsigned fs, unsigned ms, unsigned ft, unsigned mt) {
    int uls = *p_uls, ult = *p_ult, lrs = *p_lrs, lrt = *p_lrt;
                /* S25: a window with a negative start (a scrolled texture) is 12 bit two's complement. A clamp tile (or a tile
                 * with no mask) starts at texel 0 as before. A wrap or mirror tile with a mask maps the start into its mask
                 * period (NEGPAN: -32 texels, mask 64 -> 32 texels), so the RDP reads the same texels as the N64. The window
                 * size is kept. Applied here only: set_tilesize stores these values as the tile size (tile_x0, tile_y0, uvso). */
                int su = uls & 0x800 ? uls - 0x1000 : uls, st = ult & 0x800 ? ult - 0x1000 : ult;
                int sr = lrs & 0x800 ? lrs - 0x1000 : lrs, sl = lrt & 0x800 ? lrt - 0x1000 : lrt;
                if (sr >= su && sl >= st) {
                    int ou = su < 0 ? nd_tex_neg_start(su, fs, ms) : su;
                    int ot = st < 0 ? nd_tex_neg_start(st, ft, mt) : st;
                    lrs = ou + (sr - su);
                    lrt = ot + (sl - st);
                    uls = ou;
                    ult = ot;
                }
    if (PMDL("negpan") && (su < 0 || st < 0)) { /* NEGPAN step 1: a negative start is clamped to 0 (uls/ult out) */
        static unsigned nneg_; /* first 8 hits */
        if (nneg_++ < 8u) pm_log("negpan in uls %d ult %d lrs %d lrt %d out uls %d ult %d lrs %d lrt %d f%u wrap s %u/%u t %u/%u", su, st, sr, sl, uls, ult, lrs, lrt, pm_frame_count(), fs, ms, ft, mt);
    }
    *p_uls = uls; *p_ult = ult; *p_lrs = lrs; *p_lrt = lrt;
}

/* S3-2 measurement (phase census only, line sp0x): the own time and the draw close time of one typed op by its F3DEX2 code. */
__attribute__((cold, noinline)) void nd_tex_timed(const uint32_t* u) {
    const unsigned op = nd_op_n64[(u[0] >> 24) - NDO_BASE], t0 = pm_ticks();
    unsigned long long f0 = pm_gpu_fl_acc();
    nd_op_apply(u);
    unsigned dt = (unsigned)(pm_ticks() - t0), df = (unsigned)(pm_gpu_fl_acc() - f0);
    tx_t[op] += dt - (df < dt ? df : dt); tx_f[op] += df; tx_n[op]++;
}

/* HC4c (W4): the typed bind list of a texture handle (model.c make_texture_gfx, typed at the texture load) to its NDO_END, as the walker ran
 * the word list: every op runs (no skip test), a combine directly before a G_TEXTURE leaves the combine sync to the G_TEXTURE. */
__attribute__((noinline)) void nd_bind_run(const uint32_t* u) {
    for (;; u += 2) {
        const unsigned id = u[0] >> 24;
        if (id == NDO_END) break;
        pm_gbi_cur_op = nd_op_n64[id - NDO_BASE];
        if (id == NDO_COMB) {
            op_setcombine(NDO_COMB_W0(u[0]), u[1]);
            if ((u[2] >> 24) != NDO_TEXTURE) sync_combine();
        } else if (__builtin_expect(pm_ph_cen, 0)) nd_tex_timed(u);
        else nd_op_apply(u);
    }
}

int glyph_atlas_bound(void) {
    int gx, gy;
    if (!nd_ws.td[0].valid || !nd_ws.td[0].src || nd_ws.td[0].fmt != 2 || nd_ws.td[0].siz != 0 || !ens_src) { return 0; }
    if (ens_fmt != 2 || ens_siz != 0) { return 0; } /* MSGBOX 3: the palette may differ; the cell texture holds a cell per palette */
    if (memcmp(nd_ws.tst1.tile_wrap[0], ens_wrap, sizeof ens_wrap) || memcmp(nd_ws.tst1.tile_sz[0], ens_sz, sizeof ens_sz)) { return 0; }
    if (!pm_tex_glyph_same(nd_ws.td[0].src, nd_ws.td[0].fmt, nd_ws.td[0].siz, nd_ws.td[0].w, nd_ws.td[0].h, nd_ws.td[0].pal, nd_ws.td[0].pal_n, &gx, &gy)) { return 0; }
    return 1;
}

/* KZN04: the shading of a sprite as one colour. The N64 reads the 16 shade colours per texel through a second tile (the shade index is the
 * sprite's own index at the light offset), which needs a palette lookup after a texture read: the PICA has no dependent read. The sprite is drawn
 * with its own palette (cached as any sprite) and the shade is the mean of the 16 shade colours weighted by the sprite's use of each palette index
 * (opaque entries only), put through the first cycle with TEXEL1 = 1. pm_gbi_gv_at multiplies the vertex colour by it. */
float dual_f[4];
static void shade_mean(const void* sp, const void* sh) {
    unsigned cnt[256], cn16[16];
    if (PMDL("shadecen")) shc[13]++;
    memset(cnt, 0, sizeof cnt);
    const u8* src = (const u8*)m1.src;
    const int bw = m1.w >> 1; /* whole bytes of a row; the row starts on a byte (the stride is even) */
    for (int y = 0; y < m1.h; y++) {
        const u8* row = src + (size_t)y * (size_t)m1.stride / 2u;
        for (int x = 0; x < bw; x++) cnt[row[x]]++;
        if (m1.w & 1) cnt[row[bw] & 0xf0u]++; /* the odd texel: high nibble, low nibble 0 (counted as index 0 of the pair, removed below) */
    }
    for (unsigned k = 0; k < 16; k++) cn16[k] = 0;
    for (unsigned b = 0; b < 256; b++) { cn16[b >> 4] += cnt[b]; cn16[b & 15u] += cnt[b]; }
    if (m1.w & 1) cn16[0] -= (unsigned)m1.h; /* the padding low nibble of the odd texel byte */
    float s[3] = { 0.f, 0.f, 0.f }, wsum = 0.f, s_all[3] = { 0.f, 0.f, 0.f };
    for (unsigned k = 0; k < 16; k++) {
        float t0[4], t1[4];
        pal_rgba(sh, k, t0); pal_rgba(sp, k, t1);
        const float wt = t1[3] > 0.f ? (float)cn16[k] : 0.f;
        for (int q = 0; q < 3; q++) { s[q] += wt * t0[q]; s_all[q] += t0[q] * (1.f / 16.f); }
        wsum += wt;
    }
    float t0[4] = { s_all[0], s_all[1], s_all[2], 1.f }, t1[4] = { 1.f, 1.f, 1.f, 1.f }, o[4];
    if (wsum > 0.f) { const float iw = 1.f / wsum; for (int q = 0; q < 3; q++) t0[q] = s[q] * iw; }
    cc_cycle1(t0, t1, o);
    for (int q = 0; q < 4; q++) dual_f[q] = o[q];
}

int load_shaded_sprite(void) {
    const void* sp = nd_tm.tlut_pal[nd_ws.tst1.tile_pal[1] & 15];
    const void* sh = nd_tm.tlut_pal[nd_ws.tst1.tile_pal[0] & 15];
    if (!sp || !sh) { if (PMDL("shadecen")) shc[4]++; return 0; }
    shade_mean(sp, sh);
    { const int ok = pm_tex_load_shaded(m1.src, m1.w, m1.h, m1.stride, sp); if (!ok && PMDL("shadecen")) shc[5]++; return ok; }
}

/* BOMBLIT: the shade path census of the shadecen switch (measurement only, no draw change). Prints one line for a frame with a count and
 * zeroes the counters. Called from pm_gpu.c pm_gpu_end_w. */
__attribute__((cold, noinline)) void shc_frame(unsigned f) {
    int any = 0;
    unsigned sh_, sm_;
    nd_sprm_take(&sh_, &sm_); /* BOMBLIT2: the memo hits and misses of this frame, taken every frame */
    for (unsigned k = 0; k < 16; k++) any |= shc[k] != 0;
    if (!any && !sh_ && !sm_) return;
    pm_log_hw("shadecen f%u m1 %u m1skip %u ok %u cond %u nopal %u loadfail %u why3 %u nat %u fb %u why5 %u sprmh %u sprmm %u shnat %u dmin %d,%d,%d dmax %d,%d,%d smean %u img %u",
        f, shc[0], shc[1], shc[2], shc[3], shc[4], shc[5], shc[6], shc[7], shc[8], shc[9], sh_, sm_, shc[12],
        shd_min[0], shd_min[1], shd_min[2], shd_max[0], shd_max[1], shd_max[2], shc[13], shc[14]);
    memset(shc, 0, sizeof shc);
    for (int q = 0; q < 3; q++) { shd_min[q] = 255; shd_max[q] = 0; }
}

/* ---- MV-10: RDP shift helper, cut from the former pm_gbi.c (no change) ---- */
/* RDP shift field: 1..10 divide by 2^n, 11..15 multiply by 2^(16-n). */
float shift_mul(unsigned n) { return nd_shift_mul(n); } /* S3N-1: nd_model.c */

/* NAPI3B: the texture state of one message glyph (the palette load when tlut, then the 4 bit block load of the raster and the tile size of
 * gDPLoadTLUT_pal16 and gDPLoadTextureBlock_4b), by the same setters in the same order as the typed ops the walker ran for them (set_timg,
 * set_tile, set_loadtlut, set_loadblock, set_tilesize: the glyph atlas arm, the PG1 memo arm and the load of set_tilesize and ensure_tile). The ops
 * are not built, decoded or skipped by nd_op_same (none of these ops has a skip test). raster and pal are the N64 addresses of the record. w is a
 * multiple of 16. */
void nd_tex_bind_glyph(uint32_t raster, uint32_t pal, int tlut, int w, int h) {
    static const unsigned wr[4] = { 0, 0, 0, 0 };
    unsigned texels = (unsigned)(((w * h + 3) >> 2) - 1);
    gv_epoch++;
    if (tlut) {
        pm_gbi_cur_op = G_SETTIMG;
        set_timg(pal, 0, 2, 1);
        pm_gbi_cur_op = G_SETTILE;
        set_tile(7, 0, 0, 256, 0, wr, 0, 0);
        pm_gbi_cur_op = G_LOADTLUT;
        set_loadtlut(7, 15);
    }
    pm_gbi_cur_op = G_SETTIMG;
    set_timg(raster, 2, 2, 1);
    pm_gbi_cur_op = G_SETTILE;
    set_tile(7, 2, 2, 0, 0, wr, 0, 0);
    pm_gbi_cur_op = G_LOADBLOCK;
    set_loadblock(7, texels > G_TX_LDBLK_MAX_TXL ? G_TX_LDBLK_MAX_TXL : texels);
    pm_gbi_cur_op = G_SETTILE;
    set_tile(0, 2, 0, 0, 0, wr, 0, 0);
    pm_gbi_cur_op = G_SETTILESIZE;
    set_tilesize(0, 0, 0, (w - 1) << 2, (h - 1) << 2);
}
