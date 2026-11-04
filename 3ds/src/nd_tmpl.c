/* nd_tmpl.c: NDTMPL render state and TEV template per key (plan_native_slices S1-3).
 * The words are the register writes the citro3d context makes from the flush_draw setters and from flush_tev_rebuild
 * (C3Di_EffectBind and C3Di_TexEnvBind, libcitro3d.a). Table: 256 entries, open hash with a 512 slot index.
 * The TEV stages come from nd_tev_build, which pm_gpu.c flush_tev_rebuild also calls. The state words are not used by a draw yet (S3). */
#include <stdint.h>
#include <string.h>
#include "nd.h"

uint32_t f32tof24(float f); /* libctru gpu.h */

/* register numbers (libctru gpu/registers.h) */
#define R_CULL 0x040
#define R_EARLY_FUNC 0x061
#define R_EARLY_EN1 0x062
#define R_EARLY_REF 0x06A
#define R_DM_SCALE 0x04D
#define R_DM_OFFSET 0x04E
#define R_DM_ENABLE 0x06D
#define R_COLOR_OP 0x100
#define R_BLEND_FUNC 0x101
#define R_LOGIC_OP 0x102
#define R_BLEND_COLOR 0x103
#define R_ALPHA_TEST 0x104
#define R_STENCIL_TEST 0x105
#define R_STENCIL_OP 0x106
#define R_DEPTH_COLOR_MASK 0x107
#define R_EARLY_EN2 0x118
#define R_DEPTH_BUF_RD 0x126
#define R_FRAGOP_130 0x130
#define R_TEXENV_UPDATE 0x0E0
#define R_TEXENV_BUFCOL 0x0FD

/* GPU enums (libctru gpu/enums.h) */
#define G_ALWAYS 1u
#define G_EQUAL 2u
#define G_LEQUAL 5u
#define G_GREATER 6u
#define G_GEQUAL 7u
#define G_WRITE_DEPTH 0x10u
#define G_WRITE_COLOR 0x0Fu
#define G_WRITE_ALL 0x1Fu
#define G_ZERO 0u
#define G_ONE 1u
#define G_SRC_ALPHA 6u
#define G_ONE_MINUS_SRC_ALPHA 7u
#define G_KEEP 0u
#define G_ST_ZERO 1u
#define G_REPLACE 2u

/* ---- TEV: the stage words of flush_tev_rebuild (moved from pm_gpu.c) ---- */
/* GPU_TEVSRC */
#define TS_PRIMARY 0u
#define TS_TEX0 3u
#define TS_TEX1 4u
#define TS_TEX2 5u
#define TS_PREV_BUF 0xDu
#define TS_CONST 0xEu
#define TS_PREV 0xFu
/* GPU_TEVOP_RGB and GPU_TEVOP_A */
#define OR_COLOR 0u
#define OR_1MC 1u
#define OR_ALPHA 2u
#define OR_1MA 3u
#define OR_R 4u
#define OR_1MR 5u
#define OA_ALPHA 0u
#define OA_1MA 1u
#define OA_R 2u
#define OA_1MR 3u
/* GPU_COMBINEFUNC */
#define F_REPLACE 0u
#define F_MODULATE 1u
#define F_ADD 2u
#define F_ADD_SIGNED 3u
#define F_INTERP 4u
#define F_SUBTRACT 5u
#define F_MULADD 8u
#define SC_2 1u
#define SC_4 2u
#define M_RGB 1
#define M_A 2
#define M_BOTH 3

static void ev_init(uint32_t* e) { e[0] = TS_PREV | TS_PREV << 16; e[1] = 0; e[2] = 0; e[3] = 0xFFFFFFFFu; e[4] = 0; }
static void ev_src(uint32_t* e, int m, unsigned a, unsigned b, unsigned c) {
    uint32_t p = a | b << 4 | c << 8;
    if (m & M_RGB) e[0] = (e[0] & 0xFFFF0000u) | p;
    if (m & M_A) e[0] = (e[0] & 0xFFFFu) | p << 16;
}
static void ev_op_rgb(uint32_t* e, unsigned a, unsigned b, unsigned c) { e[1] = (e[1] & ~0xFFFu) | (a | b << 4 | c << 8); }
static void ev_op_a(uint32_t* e, unsigned a, unsigned b, unsigned c) { e[1] = (e[1] & ~0xFFF000u) | (a | b << 4 | c << 8) << 12; }
static void ev_func(uint32_t* e, int m, unsigned f) {
    if (m & M_RGB) e[2] = (e[2] & 0xFFFF0000u) | f;
    if (m & M_A) e[2] = (e[2] & 0xFFFFu) | f << 16;
}
static void ev_scale(uint32_t* e, int m, unsigned s) {
    if (m & M_RGB) e[4] = (e[4] & 0xFFFF0000u) | s;
    if (m & M_A) e[4] = (e[4] & 0xFFFFu) | s << 16;
}

static unsigned tev_src(int s) {
    switch (s) {
    case PM_S_TEX0: return TS_TEX0;
    case PM_S_TEX1: return TS_TEX1;
    case PM_S_TEX2: return TS_TEX2;
    case PM_S_VTX: return TS_PRIMARY;
    case PM_S_CONST: return TS_CONST;
    case PM_S_PREV: return TS_PREV;
    default: return TS_PREV_BUF;
    }
}
static unsigned tev_op_rgb(int o) { return o == PM_O_C ? OR_COLOR : o == PM_O_1MC ? OR_1MC : o == PM_O_A ? OR_ALPHA : OR_1MA; }
static unsigned tev_op_a(int o) { return (o == PM_O_1MA || o == PM_O_1MC) ? OA_1MA : OA_ALPHA; }
/* FX2a: an 8 bit slot is one channel, I, in R. A blended write leaves the alpha channel as the blend weight, so a read takes I from R, never from A. */
static int tev_i8(int src, int i8_0, int i8_1) { return (src == PM_S_TEX0 && i8_0) || (src == PM_S_TEX1 && i8_1); }
static unsigned tev_op_rgb_i(int o, int i8) {
    if (i8 && (o == PM_O_A || o == PM_O_1MA)) return o == PM_O_A ? OR_R : OR_1MR;
    return tev_op_rgb(o);
}
static unsigned tev_op_a_i(int o, int i8) {
    if (i8) return (o == PM_O_1MA || o == PM_O_1MC) ? OA_1MR : OA_R;
    return tev_op_a(o);
}
static unsigned tev_func(int f) {
    switch (f) {
    case PM_F_REPLACE: return F_REPLACE;
    case PM_F_MODULATE: return F_MODULATE;
    case PM_F_ADD: return F_ADD;
    case PM_F_SUBTRACT: return F_SUBTRACT;
    case PM_F_INTERP: return F_INTERP;
    case PM_F_ADDSIGNED: return F_ADD_SIGNED;
    default: return F_MULADD;
    }
}

void nd_tev_build(const NdTevIn* in, NdTevOut* o) {
    const NdTmplKey* k = in->k;
    const PmPlan* plan = in->plan;
    int combine = k->combine, tex = k->tex, tex1 = k->tex1, i8_0 = k->i8_0, i8_1 = k->i8_1, rgb_scale = k->rgb_scale;
    int off_active = k->off_active, off_i8 = k->off_i8, r_blend = k->r_blend, r_replace = k->r_replace, env_hi = 1;
    uint32_t* env = o->st[0];
    for (int i = 0; i < 6; i++) ev_init(o->st[i]);
    o->memb = 0; o->buf_set = 0; o->buf_color = 0;
    if (combine == 8 && plan && (!plan->uses_tex || tex) && (!plan->uses_tex1 || tex1) && (!plan->uses_noise || k->noise)) {
        /* S25: general combiner. A stage reads the texture, the vertex colour, its constant, the previous stage or the buffer colour. */
        o->buf_set = 1; o->buf_color = nd_abgr(plan->buf);
        for (int i = 0; i < plan->n && i < 6; i++) {
            uint32_t* e = o->st[i];
            const PmTevStage* st = &plan->st[i];
            ev_src(e, M_RGB, tev_src(st->g[0].src[0]), tev_src(st->g[0].src[1]), tev_src(st->g[0].src[2]));
            ev_op_rgb(e, tev_op_rgb_i(st->g[0].op[0], tev_i8(st->g[0].src[0], i8_0, i8_1)), tev_op_rgb_i(st->g[0].op[1], tev_i8(st->g[0].src[1], i8_0, i8_1)),
                      tev_op_rgb_i(st->g[0].op[2], tev_i8(st->g[0].src[2], i8_0, i8_1)));
            ev_func(e, M_RGB, tev_func(st->g[0].func));
            ev_src(e, M_A, tev_src(st->g[1].src[0]), tev_src(st->g[1].src[1]), tev_src(st->g[1].src[2]));
            ev_op_a(e, tev_op_a_i(st->g[1].op[0], tev_i8(st->g[1].src[0], i8_0, i8_1)), tev_op_a_i(st->g[1].op[1], tev_i8(st->g[1].src[1], i8_0, i8_1)),
                    tev_op_a_i(st->g[1].op[2], tev_i8(st->g[1].src[2], i8_0, i8_1)));
            ev_func(e, M_A, tev_func(st->g[1].func));
            e[3] = nd_abgr(st->constant);
            if (st->g[0].scale > 1) ev_scale(e, M_RGB, st->g[0].scale == 4 ? SC_4 : SC_2);
            if (st->g[1].scale > 1) ev_scale(e, M_A, st->g[1].scale == 4 ? SC_4 : SC_2);
        }
        env_hi = plan->n;
    } else if (combine == 8) {
        ev_src(env, M_BOTH, TS_PRIMARY, TS_PRIMARY, TS_PRIMARY);
        ev_func(env, M_BOTH, F_REPLACE);
    } else if (tex && combine == 6) {
        /* (TEXEL - PRIM) * PRIM_ALPHA + PRIM: the texture fades to the primitive colour (pause screen background). Alpha = texel alpha. */
        ev_src(env, M_RGB, TS_CONST, TS_TEX0, TS_CONST);
        ev_op_rgb(env, OR_COLOR, OR_COLOR, OR_ALPHA);
        ev_func(env, M_RGB, F_INTERP);
        ev_src(env, M_A, TS_TEX0, TS_TEX0, TS_TEX0);
        if (i8_0) ev_op_a(env, OA_R, OA_R, OA_R);
        ev_func(env, M_A, F_REPLACE);
        env[3] = nd_abgr(in->prim);
    } else if (tex && (combine == 5 || combine == 7)) {
        /* (PRIM - ENV) * TEXEL + ENV, alpha = texel alpha. ENV rides in the vertex colour. */
        ev_src(env, M_RGB, TS_CONST, TS_PRIMARY, TS_TEX0);
        ev_func(env, M_RGB, F_INTERP);
        if (combine == 7) {
            ev_src(env, M_A, TS_CONST, TS_CONST, TS_CONST);
            ev_func(env, M_A, F_REPLACE);
            env[3] = nd_abgr(in->prim | 0xffu);
        } else {
            ev_src(env, M_A, TS_TEX0, TS_TEX0, TS_TEX0);
            if (i8_0) ev_op_a(env, OA_R, OA_R, OA_R);
            ev_func(env, M_A, F_REPLACE);
            env[3] = nd_abgr(in->prim);
        }
    } else if (tex && combine == 4) {
        /* Flat colour, alpha = texture alpha times the constant alpha. */
        ev_src(env, M_RGB, TS_CONST, TS_CONST, TS_CONST);
        ev_func(env, M_RGB, F_REPLACE);
        ev_src(env, M_A, TS_TEX0, TS_CONST, TS_CONST);
        if (i8_0) ev_op_a(env, OA_R, OA_ALPHA, OA_ALPHA);
        ev_func(env, M_A, F_MODULATE);
        env[3] = nd_abgr(in->cconst);
    } else if (tex) {
        ev_src(env, M_RGB, TS_TEX0, TS_PRIMARY, TS_PRIMARY);
        ev_func(env, M_RGB, F_MODULATE);
        if (combine == 3) {
            ev_src(env, M_A, TS_PRIMARY, TS_PRIMARY, TS_PRIMARY);
            ev_func(env, M_A, F_REPLACE);
        } else {
            ev_src(env, M_A, TS_TEX0, TS_PRIMARY, TS_PRIMARY);
            if (i8_0) ev_op_a(env, OA_R, OA_ALPHA, OA_ALPHA);
            ev_func(env, M_A, F_MODULATE);
        }
        if (rgb_scale == 2) ev_scale(env, M_RGB, SC_2);
    } else if (combine == 1) {
        ev_src(env, M_BOTH, TS_CONST, TS_PRIMARY, TS_PRIMARY);
        ev_func(env, M_BOTH, F_REPLACE);
        env[3] = nd_abgr(in->prim);
    } else {
        ev_src(env, M_BOTH, TS_PRIMARY, TS_PRIMARY, TS_PRIMARY);
        ev_func(env, M_BOTH, F_REPLACE);
    }
    if (k->r_mix && k->draw_3d && !k->rect_2d && env_hi < 6 && !k->dview) {
        /* V06: colour = mix * alpha + previous * (1 - alpha), alpha = the constant alpha. The alpha channel passes. F08: r_mix 2 = weight is the vertex alpha */
        uint32_t* e = o->st[env_hi];
        ev_src(e, M_RGB, TS_CONST, TS_PREV, k->r_mix == 2 ? TS_PRIMARY : TS_CONST);
        ev_op_rgb(e, OR_COLOR, OR_COLOR, OR_ALPHA);
        ev_func(e, M_RGB, F_INTERP);
        ev_src(e, M_A, TS_PREV, TS_PREV, TS_PREV);
        ev_func(e, M_A, F_REPLACE);
        e[3] = nd_abgr(in->mixcol);
        env_hi++;
    }
    /* FX2a: an 8 bit image has memory alpha 0xE0 (7/8). The RDP blender then gives (P*a + M*0.88) / (a + 0.88): the weight of P is
     * a / (a + 0.88). The PICA blends by src alpha, so the fragment alpha becomes 0.532 * (1 - (1 - a)^2), a fit of that weight. */
    int memb = off_active && off_i8 && k->r_memblend && !r_replace;
    if (memb && env_hi <= 4) {
        uint32_t* e = o->st[env_hi++];
        ev_src(e, M_RGB, TS_PREV, TS_PREV, TS_PREV);
        ev_func(e, M_RGB, F_REPLACE);
        ev_src(e, M_A, TS_PREV, TS_PREV, TS_PREV);
        ev_op_a(e, OA_1MA, OA_1MA, OA_1MA);
        ev_func(e, M_A, F_MODULATE);
        e = o->st[env_hi++];
        ev_src(e, M_RGB, TS_PREV, TS_PREV, TS_PREV);
        ev_func(e, M_RGB, F_REPLACE);
        ev_src(e, M_A, TS_PREV, TS_CONST, TS_CONST);
        ev_op_a(e, OA_1MA, OA_ALPHA, OA_ALPHA);
        ev_func(e, M_A, F_MODULATE);
        e[3] = 0x88000000u;
    } else memb = 0;
    if (off_active && off_i8 && !(r_blend && !r_replace) && !memb && env_hi < 6) { /* FX2a: a blended 8 bit write keeps the blend weight in A */
        /* X02: an 8 bit intensity image reads as I in all four channels. The pixel written is (R, R, R, R). */
        uint32_t* e = o->st[env_hi];
        ev_src(e, M_BOTH, TS_PREV, TS_PREV, TS_PREV);
        ev_op_a(e, OA_R, OA_R, OA_R);
        ev_func(e, M_BOTH, F_REPLACE);
        env_hi++;
    }
    /* V02 depth view: vertex colour only, alpha from the texture */
    if (k->dview && k->draw_3d && !k->rect_2d) {
        for (int i = 0; i < 6; i++) ev_init(o->st[i]);
        env_hi = 6;
        ev_src(env, M_RGB, TS_PRIMARY, TS_PRIMARY, TS_PRIMARY);
        ev_func(env, M_RGB, F_REPLACE);
        if (tex) {
            ev_src(env, M_A, TS_TEX0, TS_PRIMARY, TS_PRIMARY);
            ev_func(env, M_A, F_MODULATE);
        } else {
            ev_src(env, M_A, TS_PRIMARY, TS_PRIMARY, TS_PRIMARY);
            ev_func(env, M_A, F_REPLACE);
        }
    }
    o->n = env_hi;
    o->memb = memb;
}

/* ---- render state rules: moved from pm_gbi.c sync_render (RDP other mode to draw flags) ---- */
#define OM_CYCLETYPE 20 /* G_MDSFT_CYCLETYPE */
void nd_render_state(uint32_t omode_h, uint32_t m, int depth_img, NdRender* o) {
    memset(o, 0, sizeof *o);
    o->atest = -1;
    if (depth_img) { /* B3: a model drawn into the z-buffer image is a depth write: no colour, always pass, constant depth */
        o->nocol = 1; o->zupd = 1; o->depth = 1;
        return;
    }
    o->blend = ((m & (3u << 20)) == (1u << 20) && (m & (3u << 16)) == 0) || ((m & (3u << 22)) == (1u << 22) && (m & (3u << 18)) == 0);
    if (m & 0x1000u) o->atest = o->blend ? 0x08 : 0x30; /* CVG_X_ALPHA, texture edge */
    else if ((m & 3u) == 1u) o->atest = 0x08;           /* G_AC_THRESHOLD */
    /* S25: a blender whose both colour inputs are the frame buffer colour (G_BL_CLR_MEM, cycle 1 bits 30 and 22, cycle 2 bits 28 and 20)
     * writes the old colour back: the draw changes depth only (a stencil pass). */
    int two = ((omode_h >> OM_CYCLETYPE) & 3) == 1;
    o->nocol = two ? (((m >> 28) & 3u) == 1u && ((m >> 20) & 3u) == 1u) : (((m >> 30) & 3u) == 1u && ((m >> 22) & 3u) == 1u);
    /* FX2a: two cycle only. The one cycle darkness stencil (CLD_SURF into an 8 bit slot) reads memory the port does not hold, it stays a write. */
    o->memb = two && ((m >> 20) & 3u) == 1u && ((m >> 16) & 3u) == 1u;
    o->set_memb = 1;
    o->zcmp = (m & 0x10u) != 0; o->zupd = (m & 0x20u) != 0; o->decal = (m & 0xc00u) == 0xc00u;
}

/* ---- render state words ---- */
#define ND_TMPL_N 256u
#define ND_TMPL_IDX 512u /* open hash index, power of two */

typedef struct { NdTmplKey key; unsigned n; NdTmplWord w[ND_TMPL_WORDS]; } NdTmplEnt;
static NdTmplEnt tmpl_ent[ND_TMPL_N];
static uint16_t tmpl_idx[ND_TMPL_IDX]; /* entry id + 1, 0 = empty */
static unsigned tmpl_n;

static unsigned key_hash(const NdTmplKey* k) {
    const unsigned char* b = (const unsigned char*)k;
    unsigned h = 2166136261u;
    for (unsigned i = 0; i < sizeof *k; i++) h = (h ^ b[i]) * 16777619u;
    return h;
}

static void put(NdTmplEnt* e, unsigned reg, uint32_t val) { e->w[e->n].reg = (uint16_t)reg; e->w[e->n].val = val; e->n++; }

static void build_words(NdTmplEnt* e, const NdTevOut* tev) {
    const NdTmplKey* k = &e->key;
    unsigned dfunc, dmask, den = 1, sten = 0, sfn = G_ALWAYS, sop = 0, sref = 0, swm = 0;
    float bias = 0.0f;
    e->n = 0;
    /* depth test and stencil of the pass (flush_draw main state, flush_draw_cpu decal2 passes and zmark pass 2) */
    if (k->pass == 1) { /* decal pass A: no colour, no depth write, mark the stencil where the stored depth is not behind the decal */
        bias = -ND_DECAL_EPS; dfunc = G_LEQUAL; dmask = 0;
        sten = 1; sfn = G_ALWAYS; sref = 1; swm = 0xFF; sop = G_KEEP | G_KEEP << 4 | G_REPLACE << 8;
    } else if (k->pass == 2) { /* decal pass B: draw where the mark is set and the decal bias test passes */
        memcpy(&bias, &k->dbias, 4); dfunc = G_GEQUAL; dmask = k->zupd ? G_WRITE_ALL : G_WRITE_COLOR;
        sten = 1; sfn = G_EQUAL; sref = 1; swm = 0xFF; sop = G_KEEP | G_ST_ZERO << 4 | G_ST_ZERO << 8;
    } else if (k->pass == 3) { /* zmark pass 2: constant depth where pass 1 marked the stencil, cleared as it passes */
        dfunc = G_ALWAYS; dmask = G_WRITE_DEPTH;
        sten = 1; sfn = G_EQUAL; sref = 1; swm = 0xFF; sop = G_KEEP | G_KEEP << 4 | G_ST_ZERO << 8;
        if (k->decal) memcpy(&bias, &k->dbias, 4);
    } else {
        if (k->decal) memcpy(&bias, &k->dbias, 4);
        if (k->z && k->zmark) { /* B3b pass 1: tests the model with its own depth and marks the stencil where it is nearer than the world */
            dfunc = G_GREATER; dmask = G_WRITE_DEPTH;
            sten = 1; sfn = G_ALWAYS; sref = 1; swm = 0xFF; sop = G_KEEP | G_KEEP << 4 | G_REPLACE << 8;
        } else if (k->z && k->zcmp) { dfunc = k->decal ? G_GEQUAL : G_GREATER; dmask = k->nocol ? G_WRITE_DEPTH : k->zupd ? G_WRITE_ALL : G_WRITE_COLOR; }
        else if (k->z && k->zupd) { dfunc = G_ALWAYS; dmask = k->nocol ? G_WRITE_DEPTH : G_WRITE_ALL; }
        else { den = 0; dfunc = G_GREATER; dmask = G_WRITE_COLOR; }
    }
    uint32_t depth = den | dfunc << 4 | dmask << 8;
    put(e, R_COLOR_OP, 0x00E40100u);
    put(e, R_FRAGOP_130, 0x80003C00u);
    put(e, R_DM_SCALE, f32tof24(-1.0f));
    put(e, R_DM_OFFSET, f32tof24(bias));
    put(e, R_CULL, k->cull & 3u);
    put(e, R_DM_ENABLE, 1);
    put(e, R_EARLY_EN1, 0); /* sh_early(false, GPU_EARLYDEPTH_GREATER, 0) */
    put(e, R_EARLY_EN2, 0);
    put(e, R_EARLY_FUNC, 1);
    put(e, R_EARLY_REF, 0);
    put(e, R_ALPHA_TEST, k->atest_on ? (1u | G_GREATER << 4 | (uint32_t)k->aref << 8) : (G_ALWAYS << 4));
    put(e, R_STENCIL_TEST, sten ? (1u | sfn << 4 | sref << 16 | swm << 8 | 0xFFu << 24) : (G_ALWAYS << 4 | 0xFFu << 24)); /* off: stc_set(6, false, ALWAYS, 0, 0xff, 0) */
    put(e, R_STENCIL_OP, sop);
    put(e, R_DEPTH_COLOR_MASK, depth);
    put(e, R_DEPTH_BUF_RD, (0xAF02u >> ((depth >> 3) & 14u) << 24) & 0x3000000u); /* C3Di_EffectBind: derived from the depth word */
    put(e, R_BLEND_COLOR, 0);
    /* blend: flush_draw (r_blend) or blend_off (off-screen target, depends on memb from the TEV build) */
    int one_zero, off_acc = 0;
    if (k->off_active) {
        one_zero = k->r_replace || (k->off_i8 && !k->r_blend && !tev->memb);
        off_acc = !one_zero;
    } else one_zero = !k->r_blend;
    put(e, R_BLEND_FUNC, one_zero ? (G_ONE << 16 | G_ZERO << 20 | G_ONE << 24 | G_ZERO << 28)
        : off_acc ? (G_SRC_ALPHA << 16 | G_ONE_MINUS_SRC_ALPHA << 20 | G_ONE << 24 | G_ONE_MINUS_SRC_ALPHA << 28)
                  : (G_SRC_ALPHA << 16 | G_ONE_MINUS_SRC_ALPHA << 20 | G_SRC_ALPHA << 24 | G_ONE_MINUS_SRC_ALPHA << 28));
    put(e, R_LOGIC_OP, 0);
    /* TEV: C3Di_TexEnvBind writes 5 words per stage at 0xC0 + 8 i (i 0 to 3) and 0xF0 + 8 (i - 4) (i 4, 5) */
    for (unsigned i = 0; i < 6; i++) {
        unsigned base = i < 4 ? 0xC0u + 8u * i : 0xF0u + 8u * (i - 4u);
        for (unsigned j = 0; j < 5; j++) put(e, base + j, tev->st[i][j]);
    }
    put(e, R_TEXENV_UPDATE, 0); /* plan draws: C3D_TexEnvBufUpdate(C3D_Both, 0) */
    put(e, R_TEXENV_BUFCOL, tev->buf_set ? tev->buf_color : 0);
}

int nd_tmpl_build(const NdTmplKey* in) {
    NdTmplKey kk = *in;
    memset(kk.pad, 0, sizeof kk.pad);
    if (!kk.decal) kk.dbias = 0; /* the bias is not used without decal: one key for both */
    if (kk.combine != 8) { kk.w0 = kk.w1 = 0; kk.two = kk.t1only = 0; }
    unsigned h = key_hash(&kk) & (ND_TMPL_IDX - 1u);
    for (;;) {
        uint16_t s = tmpl_idx[h];
        if (s == 0) break;
        if (memcmp(&tmpl_ent[s - 1u].key, &kk, sizeof kk) == 0) return s - 1;
        h = (h + 1u) & (ND_TMPL_IDX - 1u);
    }
    /* ponytail: table full returns -1, the caller falls back to the walker. Add eviction when S3 fills more than 256 keys. */
    if (tmpl_n >= ND_TMPL_N) return -1;
    PmPlan plan;
    NdTevIn ti;
    NdTevOut to;
    memset(&ti, 0, sizeof ti);
    ti.k = &kk;
    if (kk.combine == 8) {
        memset(&plan, 0, sizeof plan);
        if (pm_comb_build(kk.w0, kk.w1, kk.two, kk.t1only, &plan) != 0) return -1; /* the game only sets combine 8 from a plan that built */
        ti.plan = &plan;
    }
    nd_tev_build(&ti, &to);
    NdTmplEnt* e = &tmpl_ent[tmpl_n];
    e->key = kk;
    build_words(e, &to);
    tmpl_idx[h] = (uint16_t)(tmpl_n + 1u);
    return (int)tmpl_n++;
}

unsigned nd_tmpl_words(int id, const NdTmplWord** out) {
    *out = tmpl_ent[id].w;
    return tmpl_ent[id].n;
}
