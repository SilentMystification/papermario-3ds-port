/* nd_direct.c: the shared direct-draw helpers of the native draw API (NATIVEAPI A1, nd_api.h). Core 0. A direct executor states a draw as a pso (an N64
 * state description from nd_pso_n64), a texture descriptor and the rectangle words, and skips the op stream of the walker: no typed ops, no per-op
 * state sync. The combine decision is the walker's own function (nd_comb_decide) made once per distinct inputs; the texture bind is nd_tex_bind_desc
 * (the walker's bind); the render state is sync_render. The walker state the stream path would have left (other mode, combine words, primitive colour)
 * is written too, so a later walker draw sees what it saw before. */
#include <stdint.h>
#include <string.h>
#include "ultra64.h"
#include "nd.h"
#include "nd_int.h"
#include "nd_api.h"
#include "pm_gpu.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "PR/gbi.h"
#include "gbi_custom.h"
#include "nd_gbi.h"

#ifndef pm_gpu_flush
#define pm_gpu_flush() pm_gpu_flush_at(__LINE__)
#endif
int nd_dq_on; /* S5CHUDA: a direct quad is drawing (nd_direct_quad .. nd_direct_end); read by gv_epoch_enter */
void comb_in_walker(NdCombIn* in);                 /* nd_comb.c: the walker's combine inputs */
void ens_clear(void);                              /* nd_tex_set.c */
void nd_sr_forget(void);                           /* nd_set.c */
extern int ls_use, ls_why, ls_cyc, ls_dual, ls_frame; /* nd_comb.c: the decision the last sync_combine made (diagnostics, the memo) */
void pm_tex_set_filter(int linear);                  /* pm_tex.c */
extern PmPlan cur_plan;                            /* nd_comb.c */
extern int pm_comb_vprim;                         /* pm_comb.c */

/* One decision: the inputs it was made under (key) and the outputs. Direct mapped by pso id; an entry of another id or other inputs is made again. */
struct NdDec {
    NdCombOut out;
    uint32_t key[8];
    NdPso id;
    int ok;       /* 1: the direct path can state it (no tile 1 read, no noise, no box pattern) */
    uint8_t used;
};
#define DEC_N 16u
static NdDec dec_tab[DEC_N];

static __attribute__((cold, noinline)) NdDec* dec_make(NdDec* e, NdPso id, const NdPsoN64* p, const uint32_t* key) {
    NdCombIn in;
    if (nd_ws.comb_w0 != p->cw0 || nd_ws.comb_w1 != p->cw1 || !comb_set) op_setcombine(p->cw0, p->cw1); /* the tile fields of the pso words */
    comb_in_walker(&in);
    memset(e, 0, sizeof *e);
    nd_comb_decide(&in, &e->out);
    e->ok = !(e->out.plan_on && (e->out.plan.uses_tex1 || e->out.plan.uses_noise)) && !e->out.pat;
    memcpy(e->key, key, sizeof e->key);
    e->id = id;
    e->used = 1;
    return e;
}

/* The inputs of the decision that the pso id does not name: the other mode words it runs under, the walker flags and combiner constants the combine
 * work reads (nd_comb.c decide and sync_legacy). The primitive and environment colours are not inputs: they only colour the result (nd_direct_bind). */
static void dec_key(uint32_t* k, const NdPsoN64* p, uint32_t mh, uint32_t ml) {
    k[0] = mh; k[1] = ml;
    k[2] = (uint32_t)(nd_ws.tex_on | nd_ws.use_t1 << 1 | nd_ws.td[1].valid << 2 | dual_on << 3 | box_prod << 4 | box_pat << 5 | box_folded << 6 | comb_set << 7 |
                      comb_alpha_shade << 8);
    k[3] = keyc; k[4] = keys; k[5] = conv_k4;
    k[6] = (uint32_t)(uint8_t)nd_ws.prim_lod | (uint32_t)pm_comb_vprim << 8;
    k[7] = p->cw0 ^ (p->cw1 * 2654435761u);
}

/* The decision of pso id under the current walker state (it sets the pso combine words in the walker state: the decision reads the tile fields they decode to). NULL: not statable. */
const NdDec* nd_pso_decide(NdPso id) {
    const NdPsoN64* p = nd_pso_get(id);
    const uint32_t mh = (nd_ws.omode_h & ~p->omh_m) | (p->omh & p->omh_m);
    const uint32_t ml = (nd_ws.omode_l & 7u) | (p->oml & ~7u);
    uint32_t key[8];
    NdDec* e = &dec_tab[id & (DEC_N - 1u)];
    dec_key(key, p, mh, ml);
    if (!e->used || e->id != id || memcmp(e->key, key, sizeof key)) {
        uint32_t sh = nd_ws.omode_h;
        nd_ws.omode_h = mh; /* the decision reads the cycle type of the pso's mode */
        e = dec_make(e, id, p, key);
        nd_ws.omode_h = sh;
    }
    return e->ok ? e : NULL;
}

int nd_direct_ready(void) {
    if (img_role(0) != ROLE_MAIN || cimg_zero) return -1;  /* the main colour image: no off-screen target, no depth role, no z-buffer copy */
    if (nd_ws.cimg && !pm_is_cfb(nd_ws.cimg)) return -2;    /* rect_to_image draws into a non-frame-buffer image */
    if (m1.valid) return -3;                                /* a shaded sprite tile 1 raster: set_tilesize takes tex_shaded0 */
    if (pm_tex_aux_off_idx() >= 0) return -4;                /* t1_fold would move the tile 1 map */
    return 1;
}

uint32_t nd_direct_prim(void) { return nd_ws.prim; }
uint32_t nd_direct_env(void) { return nd_ws.envc; }

int nd_direct_bind(NdPso id, uint32_t prim, uint32_t env) {
    const NdPsoN64* p = nd_pso_get(id);
    const uint32_t mh = (nd_ws.omode_h & ~p->omh_m) | (p->omh & p->omh_m);
    const uint32_t ml = (nd_ws.omode_l & 7u) | (p->oml & ~7u);
    const NdDec* d;
    if (((mh >> G_MDSFT_CYCLETYPE) & 3u) >= 2u) return 0; /* COPY and FILL rectangles have their own span rules */
    if ((ml & (1u << G_MDSFT_ZSRCSEL)) && (ml & 0x10u) && (nd_ws.geom & G_ZBUFFER)) return 0; /* G_ZS_PRIM rectangle: its own depth test */
    {   /* the other mode and combine words the stream would have set, in its order (filter, render mode, combine, primitive colour) */
        const unsigned old_tf = nd_ws.omode_h & (3u << 12);
        nd_ws.omode_h = mh; nd_ws.om_kh |= p->omh_m;
        nd_ws.omode_l = ml; nd_ws.om_kl |= ~7u;
        if ((nd_ws.omode_h & (3u << 12)) != old_tf) { /* V15: G_MDSFT_TEXTFILT */
            pm_gpu_flush();
            pm_tex_set_filter((nd_ws.omode_h & (3u << 12)) != 0);
        }
    }
    if (nd_ws.comb_w0 != p->cw0 || nd_ws.comb_w1 != p->cw1 || !comb_set) op_setcombine(p->cw0, p->cw1);
    dual_on = 0; /* set_tilesize of tile 0 ends the shaded sprite mode */
    pm_gpu_set_prim(prim, 0u);
    nd_ws.prim = prim; nd_ws.prim_lod = 0u;
    nd_ws.envc = env;
    d = nd_pso_decide(id);
    if (!d) return 0; /* the stream path sets the same words again: nothing it reads is changed */
    {   /* what sync_combine does with the decision (scm_replay) */
        const NdCombOut* o = &d->out;
        unsigned cc = 0;
        leg_mode = o->leg; comb_mode5 = o->mode5; box_pat = o->pat; plan_active = o->plan_on;
        ls_use = o->use; ls_why = o->why; ls_cyc = o->cyc; ls_dual = o->dual; ls_frame = rm_frame;
        if (o->cc_on) {
            if (!const_rgb_tex_alpha(&cc)) cc = o->cc;
            pm_gpu_set_const(cc);
        }
        if (o->env_on) pm_gpu_set_env(nd_ws.envc);
        if (o->plan_on) {
            cur_plan = o->plan;
            pm_comb_colours(&cur_plan, nd_ws.prim, nd_ws.envc, (u8)nd_ws.prim_lod, keyc, keys);
            pm_gpu_plan_key(nd_ws.comb_w0, nd_ws.comb_w1, o->cyc, nd_ws.use_t1);
            pm_gpu_set_plan(&cur_plan);
        } else {
            pm_gpu_set_combine(leg_mode);
        }
    }
    nd_sr_forget(); /* sync_combine of the stream path clears it when the inputs change: the render state is made again */
    sync_render();
    return 1;
}

/* Tile 0 = d: the state set_tilesize leaves for the bind (origin, quarter texel offsets, the repeated-load record) and the bind. */
int nd_direct_tex(const NdBindDesc* d) {
    nd_ws.tile_x0 = d->x0;
    nd_ws.tile_y0 = d->y0;
    nd_ws.tile_fx = (float)(d->sz[0] & 3) * 0.25f; /* PT38 */
    nd_ws.tile_fy = (float)(d->sz[1] & 3) * 0.25f;
    tsz_last.ok = 0;
    tsz_last.src = (const u8*)d->src; tsz_last.fmt = d->fmt; tsz_last.siz = d->siz; tsz_last.w = d->w; tsz_last.h = d->h;
    tsz_last.stride = d->stride; tsz_last.pal = d->pal; tsz_last.pal_n = d->pal_n;
    return nd_tex_bind_desc(d, 0);
}

/* One texture rectangle of tile 0: nd_texrect (nd_rect.c) for a 1 or 2 cycle rectangle with no tile shift, no tile 1 and a main colour image. */
void nd_direct_rect(uint32_t w0, uint32_t w1, uint32_t hs, uint32_t hd) {
    int tw = 1, th = 1;
    int xh = (int)((w0 >> 12) & 0xfff), yh = (int)(w0 & 0xfff), xl = (int)((w1 >> 12) & 0xfff), yl = (int)(w1 & 0xfff);
    if (xh & 0x800) xh |= ~0xfff;
    if (yh & 0x800) yh |= ~0xfff;
    if (xl & 0x800) xl |= ~0xfff;
    if (yl & 0x800) yl |= ~0xfff;
    gp3_t0_surv();
    pm_tex_size(&tw, &th);
    {
        float s0 = ((float)(s16)(hs >> 16) / 32.f - (float)nd_ws.tile_x0 - nd_ws.tile_fx) / (float)tw;
        float t0 = ((float)(s16)(hs & 0xffff) / 32.f - (float)nd_ws.tile_y0 - nd_ws.tile_fy) / (float)th;
        float dsdx = (float)(s16)(hd >> 16) / 1024.f;
        float dtdy = (float)(s16)(hd & 0xffff) / 1024.f;
        float width = (float)(xh - xl) / 4.f;
        float height = (float)(yh - yl) / 4.f;
        int rx0 = xl / 4, ry0 = yl / 4, ex = xh / 4 - 1, ey = yh / 4 - 1; /* the lower right edge is exclusive in 1 and 2 cycle mode */
        float s1 = s0 + dsdx * width / (float)tw;
        float t1 = t0 + dtdy * height / (float)th;
        if (ex >= rx0 && ey >= ry0) pm_gpu_tex_rect(rx0, ry0, ex, ey, s0, t0, s1, t1);
    }
}

void nd_direct_end(void) {
    ens_clear();
    nd_ws.cur_tile = -1;
    sc_ok = 0;
    nd_dq_on = 0;
    nd_sr_forget();
    gv_epoch++; /* the ops of the stream path each raise it: the first triangle after this draw takes its state again */
}

/* Census of the records a direct executor hands back to the old path: the first 16 and every 64th are logged (debug level) with the frame and the reason. */
void pm_log_d(const char* fmt, ...);
unsigned pm_frame_count(void);
__attribute__((cold, noinline)) void nd_direct_fallback(unsigned why) {
    static unsigned n;
    n++;
    if (n <= 16u || !(n & 63u)) pm_log_d("napihud fallback f%u n %u why %u", pm_frame_count(), n, why);
}

/* NAPI3B: one message glyph (msg_draw.c msg_glyph_run). The walker state before it (combine, render mode, primitive colour) comes from the typed ops
 * the printer setup recorded; the glyph states its texture (nd_tex_bind_glyph) and its rectangle. nd_glyph_can: 1 when the direct draw takes the
 * walker's own path (nd_texrect for a 1 cycle rectangle on the main colour image, tile 0, no shift, no tile 1 product); checked before any state is
 * touched, 0 runs the old list. No nd_direct_end: the walker leaves the bind (ens_*, tsz_last) as the glyph arms keep it, and so does this. */
int nd_glyph_can(int texW) {
    if (texW < 16 || (texW & 15)) return 0;
    if (nd_direct_ready() != 1) return 0;
    if (((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3u) >= 2u) return 0;
    if ((nd_ws.omode_l & (1u << G_MDSFT_ZSRCSEL)) && (nd_ws.omode_l & 0x10u) && (nd_ws.geom & G_ZBUFFER)) return 0;
    if (nd_ws.use_t1 || dual_on || box_prod || box_pat) return 0;
    return 1;
}

static inline int sx12(int v) { v &= 0xfff; return (v & 0x800) ? v | ~0xfff : v; }

void nd_glyph_draw(uint32_t raster, uint32_t pal, int tlut, int w, int h, int ulx, int uly, int lrx, int lry, int s, int t, int dsdx_, int dtdy_) {
    int tw = 1, th = 1;
    const int xl = sx12(ulx * 4), yl = sx12(uly * 4), xh = sx12(lrx * 4), yh = sx12(lry * 4);
    nd_tex_bind_glyph(raster, pal, tlut, w, h);
    pm_gbi_cur_op = G_TEXRECT;
    gv_epoch++;
    off_screen_check();
    box_fold_sync();
    sync_render();
    gp3_t0_surv();
    ensure_tile(0);
    dual_bind();
    pm_tex_size(&tw, &th);
    {
        const float s0 = ((float)(s16)s / 32.f - (float)nd_ws.tile_x0 - nd_ws.tile_fx) / (float)tw;
        const float t0 = ((float)(s16)t / 32.f - (float)nd_ws.tile_y0 - nd_ws.tile_fy) / (float)th;
        const float dsdx = (float)(s16)dsdx_ / 1024.f;
        const float dtdy = (float)(s16)dtdy_ / 1024.f;
        const float width = (float)(xh - xl) / 4.f;
        const float height = (float)(yh - yl) / 4.f;
        /* nd_texrect names the rectangle by xh, yh of the first word: the lower right corner; xl, yl the upper left */
        const int rx0 = xl / 4, ry0 = yl / 4, ex = xh / 4 - 1, ey = yh / 4 - 1;
        const float s1 = s0 + dsdx * width / (float)tw;
        const float t1 = t0 + dtdy * height / (float)th;
        if (ex >= rx0 && ey >= ry0) pm_gpu_tex_rect(rx0, ry0, ex, ey, s0, t0, s1, t1);
    }
}

/* NATIVEAPI E5b (S5PQUAD): the combine and other mode low bits of each NdFillRec kind (the walker's words for the same macros). */
static const struct { Gfx cc; uint32_t rm0, rm1; } kFillKind[4] = {
    { gsDPSetCombineMode(G_CC_PRIMITIVE, G_CC_PRIMITIVE), G_RM_CLD_SURF, G_RM_CLD_SURF2 },
    { gsDPSetCombineMode(G_CC_PRIMITIVE, G_CC_PRIMITIVE), CVG_DST_SAVE | G_RM_OPA_SURF, CVG_DST_SAVE | G_RM_OPA_SURF2 },
    /* PM_CC_PRIM_FULL_ALPHA (include/macros.h:328) written out: macros.h is not included here */
    { gsDPSetCombineLERP(0, 0, 0, PRIMITIVE, 0, 0, 0, 1, 0, 0, 0, PRIMITIVE, 0, 0, 0, 1), G_RM_OPA_SURF, G_RM_OPA_SURF2 },
    { gsDPSetCombineMode(G_CC_PRIMITIVE, G_CC_PRIMITIVE), G_RM_XLU_SURF, G_RM_XLU_SURF2 },
};

void nd_walk_state(const uint32_t* q, unsigned n); /* nd_walk.c, core 0: the op executor */

/* the typed units of the fallback ops: the record-side gbi redirect (nd_gx_w takes a local packet, so the ops are typed as the stream has them) */
#include "gbi_custom.h"
#include "nd_gbi.h"

/* Off-screen or depth colour image (row S5PQUAD 2b): the walker's fill takes its own path (off_screen_check). The state of the kind and the
 * primitive colour as ops, then one fill rectangle (lower right exclusive, as passed). n = 4 ops (nd_walk_state counts ops). */
static void nd_fill_walk(const NdFillRec* f, unsigned k) {
    static Gfx buf[4];
    static unsigned nfb;
    if (nfb++ < 16u) pm_log_d("fill fallback f%u", pm_frame_count());
    nd_gx_w(&buf[0], ND_X0(kFillKind[k].cc.words.w0, kFillKind[k].cc.words.w1), ND_X1(kFillKind[k].cc.words.w0, kFillKind[k].cc.words.w1));
    gDPSetRenderMode(&buf[1], kFillKind[k].rm0, kFillKind[k].rm1);
    gDPSetPrimColor(&buf[2], 0, 0, (f->rgba >> 24) & 0xffu, (f->rgba >> 16) & 0xffu, (f->rgba >> 8) & 0xffu, f->rgba & 0xffu);
    gDPFillRectangle(&buf[3], f->x0, f->y0, f->x1, f->y1);
    nd_walk_state((const uint32_t*)buf, 4u);
}

/* core 0: the 1-cycle fill of a rectangle, as the walker's fill (nd_rect.c nd_fillrect): primitive colour, lower right exclusive. */
void nd_fill_run(const uint32_t* u) {
    const NdFillRec* f = (const NdFillRec*)u;
    const unsigned k = f->kind & 3u;
    const int x1 = f->x1 - 1, y1 = f->y1 - 1;
    NdPsoN64 d;
    if (nd_direct_ready() != 1) {
        nd_fill_walk(f, k);
        return;
    }
    d.cw0 = kFillKind[k].cc.words.w0;
    d.cw1 = kFillKind[k].cc.words.w1;
    d.oml = kFillKind[k].rm0 | kFillKind[k].rm1;
    d.omh = G_CYC_1CYCLE;
    d.omh_m = 3u << G_MDSFT_CYCLETYPE;
    nd_direct_bind(nd_pso_n64(&d), f->rgba, nd_direct_env());
    if (x1 >= f->x0 && y1 >= f->y0) {
        pm_gpu_set_replace(0);
        pm_gpu_fill_rect(f->x0, f->y0, x1, y1, f->rgba);
    }
    nd_direct_end();
}

/* S5TRECT: the NDC_NAPI runner of one NdTRect (nd_api.h): a texture window and one texture rectangle. Direct when the window is valid and the walker
 * state lets a direct rectangle draw (nd_direct_ready == 1); else the same load and rectangle ops into a local list, walked with nd_walk_state. */
void nd_trect_run(const uint32_t* u) {
    static Gfx buf[16];
    static unsigned nfb;
    const NdTRect* r = (const NdTRect*)u;
    const int is4 = r->siz == G_IM_SIZ_4b;
    const unsigned sh = is4 ? 1u : 2u;
    const int stride = is4 ? (int)(r->width >> 1) : (int)r->width;
    const int32_t lu = r->uls << sh, ll = r->lrs << sh;
    const unsigned wrap[4] = { r->cms, r->cmt, r->masks, r->maskt };
    NdBindDesc d;
    Gfx* p = buf;
    const int ok = nd_tex_win_desc(&d, r->img, stride, r->fmt, r->siz, wrap, lu >> 2, r->ult, ((ll - lu) >> 2) + 1, r->lrt - r->ult + 1,
                                   r->uls << 2, r->ult << 2, r->lrs << 2, r->lrt << 2, r->pal, (int)r->pal_n);
    if (ok && nd_direct_ready() == 1) {
        const NdPsoN64 pd = { .cw0 = r->cw0, .cw1 = r->cw1, .oml = r->oml, .omh = r->omh, .omh_m = (3u << G_MDSFT_TEXTLUT) | (3u << G_MDSFT_TEXTFILT) };
        const NdPso id = nd_pso_n64(&pd);
        if (nd_direct_bind(id, (r->pad & 1u) ? nd_direct_prim() : r->prim, nd_direct_env())) {
            nd_direct_tex(&d);
            nd_direct_rect(r->w0, r->w1, r->hs, r->hd);
            nd_direct_end();
            return;
        }
    }
    if (nfb++ < 16u) pm_log_d("trect fallback f%u", pm_frame_count());
    switch (r->siz) {
        case G_IM_SIZ_4b:
            gDPLoadTextureTile_4b(p++, r->img, r->fmt, r->width, r->height, r->uls, r->ult, r->lrs, r->lrt, 0, r->cms, r->cmt, r->masks, r->maskt, G_TX_NOLOD, G_TX_NOLOD);
            break;
        case G_IM_SIZ_8b:
            gDPLoadTextureTile(p++, r->img, r->fmt, G_IM_SIZ_8b, r->width, r->height, r->uls, r->ult, r->lrs, r->lrt, 0, r->cms, r->cmt, r->masks, r->maskt, G_TX_NOLOD, G_TX_NOLOD);
            break;
        case G_IM_SIZ_16b:
            gDPLoadTextureTile(p++, r->img, r->fmt, G_IM_SIZ_16b, r->width, r->height, r->uls, r->ult, r->lrs, r->lrt, 0, r->cms, r->cmt, r->masks, r->maskt, G_TX_NOLOD, G_TX_NOLOD);
            break;
        default:
            gDPLoadTextureTile(p++, r->img, r->fmt, G_IM_SIZ_32b, r->width, r->height, r->uls, r->ult, r->lrs, r->lrt, 0, r->cms, r->cmt, r->masks, r->maskt, G_TX_NOLOD, G_TX_NOLOD);
            break;
    }
    ND_GX(p++, r->w0, r->w1);
    gImmp1(p++, G_RDPHALF_1, r->hs);
    gImmp1(p++, G_RDPHALF_2, r->hd);
    nd_walk_state((const uint32_t*)buf, (unsigned)(p - buf));
}

/* S5CHUDA (NATIVEAPI E1): the geometry of one transformed HUD quad on the bound state. nd_dq_on is set for the length of the quad and cleared by
 * nd_direct_end: gv_epoch_enter (nd_geom.c) then skips ensure_tile and dual_bind, which would rebind tile 0 from the walker's stale td[0] over
 * the descriptor nd_direct_tex bound. The walker state a direct bind changed raises gv_epoch once, as the first state op of the stream would. */
void nd_direct_quad(uint32_t mtx, const void* vtx4) {
    const u32 wm = ND_W0_D2(G_MTX, sizeof(Mtx), (G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW) ^ G_MTX_PUSH, 0) & 0xffffffu; /* gSPMatrix */
    const u32 wv = ND_W0_VTX(4, 0) & 0xffffffu;                                                                       /* gSPVertex(v, 4, 0) */
    const u32 t0 = (_SHIFTL(G_TRI1, 24, 8) | __gsSP1Triangle_w1f(0, 1, 2, 0)) & 0xffffffu;                            /* gSP1Triangle(0, 1, 2) */
    const u32 t1 = (_SHIFTL(G_TRI1, 24, 8) | __gsSP1Triangle_w1f(0, 2, 3, 0)) & 0xffffffu;                            /* gSP1Triangle(0, 2, 3) */
    nd_dq_on = 1;
    gv_epoch++;
    pm_gbi_cur_op = G_MTX;
    load_mtx(wm, mtx, 0);
    pm_gbi_cur_op = G_VTX;
    nd_geom_vtx(wv, (u32)(uintptr_t)vtx4, NULL);
    pm_gbi_cur_op = G_TRI1;
    tri_prep_ok = 0;
    draw_tri(t0);
    tri_prep_ok = 0;
    draw_tri(t1);
    pm_gbi_cur_op = G_POPMTX;
    gbi_popmtx(64u); /* gSPPopMatrix: w1 = 1 * 64 */
}

/* S5CHUDA: tile 0 after gDPLoadTextureBlock(timg, fmt, siz, w, h, 0, cms, cmt, masks, maskt, 0, 0). The block load leaves no LoadTile window (set_timg
 * clears ld_valid, the block load sets none, and it clears the load record of its TMEM address), the image stride is the width (the macro loads
 * through a width 1 image, so set_tilesize takes the tile width) and the tile size is (0, 0, (w - 1) << 2, (h - 1) << 2). That is the window of
 * nd_tex_win_desc with the origin (0, 0), the full size and stride w, so the rule is that call; with a mask the ld_valid rules of tex_win0 would
 * differ (period widening), so a mask or a block over 2048 texels is not valid (0): the caller runs the old list. */
int nd_tex_blk_desc(NdBindDesc* o, const void* timg, unsigned fmt, unsigned siz, int w, int h, const unsigned wrap[4], const void* pal, int pal_n) {
    if (w <= 0 || h <= 0 || siz == 0 || (((unsigned)(w * h) << siz) >> 1) > 4096u || wrap[2] || wrap[3]) return 0; /* S5CHUDB: byte cap (4 KB TMEM), not the 16-bit word limit */
    return nd_tex_win_desc(o, timg, w, fmt, siz, wrap, 0, 0, w, h, 0, 0, (w - 1) << 2, (h - 1) << 2, pal, pal_n);
}

/* S5CHUDB try 3: the walker tile 0 TILESIZE of a non-CI4 tile clears the shaded sprite raster (nd_tex_set.c:205-206); a direct
 * IA8 bind does the same before its draw, so nd_direct_ready no longer refuses on m1.valid (-3). */
void nd_direct_m1_clear(void) { m1.valid = 0; }
