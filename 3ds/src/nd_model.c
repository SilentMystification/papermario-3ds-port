/* nd_model.c: native model draws (plan S3N). S3N-1: the draw parameters of a raw triangle (nd_gv_params), moved from pm_gbi.c gv_params_calc.
 * Pure C: the host test builds it (3ds/tools/nd_gv_test.c). No walker state and no pm_gpu call inside: the caller passes the values. */
#include <stdint.h>
#include <string.h>
#include "nd.h"
#include "nd_ops.h"

float nd_shift_mul(unsigned n) {
    return n == 0 ? 1.f : n <= 10 ? 1.f / (float)(1 << n) : (float)(1 << (16 - n));
}

float nd_prim_ndc_z(unsigned prim_z) { return 2.f * (float)(prim_z ? prim_z - 1u : 0u) / 32767.f - 1.f; }

void nd_gv_params(const NdGvIn* in, NdGvP* o) {
    int env = in->comb5 && !in->plan_active;
    int plan = in->plan_active && in->plan_vtx_ok;
    int a255 = !in->plan_active && !plan && !in->alpha_shade;
    int boost = in->boost;
    int primz = in->primz != 0;
    int tw = in->tw, th = in->th;
    if (tw < 1) tw = 1;
    if (th < 1) th = 1;
    float ms = nd_shift_mul(in->shs), mt = nd_shift_mul(in->sht);
    const float* en = in->en;
    o->vp[0] = in->vp[0]; o->vp[1] = in->vp[1]; o->vp[2] = in->vp[2]; o->vp[3] = in->vp[3];
    for (int q = 0; q < 4; q++) { o->cm[q] = 1.f; o->ca[q] = 0.f; }
    o->zm[0] = 1.f; o->zm[1] = 0.f;
    uint32_t tss = in->tss;
    if (in->tex_on) {
        float itw = 1.f / (float)tw, ith = 1.f / (float)th; /* H5: one divide per axis, not one per term */
        o->uvso[0] = (float)(tss >> 16) / 65536.f / 32.f * ms * itw;
        o->uvso[1] = (float)(tss & 0xffffu) / 65536.f / 32.f * mt * ith;
        o->uvso[2] = -((float)in->tile_x0 + in->tile_fx) * itw;
        o->uvso[3] = -((float)in->tile_y0 + in->tile_fy) * ith;
    } else { /* H06b1: the vertex texcoords are 0, texel (0, 0) centre */
        o->uvso[0] = o->uvso[1] = 0.f;
        o->uvso[2] = -((float)in->tile_x0 + in->tile_fx) / (float)tw + 0.5f / (float)tw;
        o->uvso[3] = -((float)in->tile_y0 + in->tile_fy) / (float)th + 0.5f / (float)th;
    }
    if (env) { for (int q = 0; q < 4; q++) { o->cm[q] = 0.f; o->ca[q] = en[q]; } } /* the CPU order: ENV or the plan constant, then alpha 255, then boost */
    else if (plan) { const float* vc = in->vc; for (int q = 0; q < 4; q++) { o->cm[q] = 0.f; o->ca[q] = vc[q]; } }
    if (a255) { o->cm[3] = 0.f; o->ca[3] = 1.f; }
    if (boost) for (int q = 0; q < 3; q++) { o->ca[q] = o->ca[q] * 0.5f + (1.f - en[q]) * 0.5f; o->cm[q] *= 0.5f; }
    if (primz) { o->zm[0] = 0.f; o->zm[1] = nd_prim_ndc_z(in->prim_z); }
    o->boost = boost;
    o->fxon = 0; /* o->fx is read only when fxon: it is cleared where it is filled */
    if (in->lighting) { /* GV5: the RSP light table (ambient is slot lt_n), the look-at vectors for texgen */
        nd_lights(&o->fx, in->texgen != 0, in->lt, in->lt_n, in->lk);
        o->fxon = 1;
    }
    if (in->fogmix && in->fog) { /* GV5: the RSP fog factor clamp(z / w * fm + fo) is the alpha. z / w of the N64 clip position from the PICA clip z and w. */
        if (in->zc_ok) {
            if (!o->fxon) memset(&o->fx, 0, sizeof o->fx);
            o->fx.lflag[2] = 1.f;
            o->fx.fogu[0] = (float)in->fog_fm / in->za;
            o->fx.fogu[1] = (float)in->fog_fo - (float)in->fog_fm * in->zb / in->za;
            o->cm[3] = 1.f; o->ca[3] = 0.f; /* the fog factor replaces the alpha, after the alpha 255 rule */
            o->fxon = 1;
        }
    }
}

/* ---- S3N-1 (B4): texture descriptors of the map model handles ---- */
#define ND_HANDLE_N 128
static NdTexDesc hd_desc[ND_HANDLE_N];
static const void* hd_gfx[ND_HANDLE_N];
static uint8_t hd_ok[ND_HANDLE_N];

void nd_tex_handle_set(int index, const void* gfx, const void* raster, const void* palette, unsigned w, unsigned h, unsigned fmt, unsigned siz,
                       unsigned wrap_w, unsigned wrap_h, unsigned masks, unsigned maskt, unsigned extra, int has_aux) {
    if (index < 0 || index >= ND_HANDLE_N) return;
    hd_ok[index] = 0; hd_gfx[index] = gfx;
    if (extra != 0 || has_aux || !raster || w == 0 || h == 0 || siz > 3) return; /* B4: common case only */
    NdTexDesc d;
    memset(&d, 0, sizeof d);
    d.img = raster; d.pal = palette;
    d.fmt = (uint8_t)fmt; d.siz = (uint8_t)siz; d.cms = (uint8_t)wrap_w; d.cmt = (uint8_t)wrap_h; d.masks = (uint8_t)masks; d.maskt = (uint8_t)maskt;
    d.w = (uint16_t)w; d.h = (uint16_t)h; d.stride = (uint16_t)w;
    d.pal_n = palette ? (siz == 0 ? 16u : siz == 1 ? 256u : 0u) : 0u; /* gDPLoadTLUT_pal16, pal256 */
    hd_desc[index] = d; hd_ok[index] = 1;
}

const NdTexDesc* nd_tex_handle_desc(int index, const void* gfx) {
    if (index < 0 || index >= ND_HANDLE_N || !hd_ok[index] || hd_gfx[index] != gfx) return NULL;
    return &hd_desc[index];
}

/* ---- S3N-1 (B5, B6): render mode lists ---- */
/* the bits nd_render_state reads: om_h cycle type (bits 20, 21); om_l bits 0, 1, 4, 5, 10, 11, 12, 16 to 23, 28 to 31 */
#define ND_OMH_READ 0x00300000u
#define ND_OML_READ 0xF0FF1C33u

int nd_mode_compile(NdMode* m, const unsigned* w) {
    uint32_t mh = 0, ml = 0, vh = 0, vl = 0;
    m->ok = 0; m->has_cb = 0; m->has_tx = 0; m->has_g = 0; m->gand = 0xFFFFFFFFu; m->gor = 0; m->bad = 0;
    for (unsigned i = 0; i < 8; i++) {
        uint32_t w0 = w[2 * i], w1 = w[2 * i + 1];
        uint32_t op = w0 >> 24;
        if (op == OP_ENDDL) break;
        if (op == OP_SETCOMBINE) { m->has_cb = 1; m->cw0 = w0; m->cw1 = w1; continue; }
        if (op == OP_TEXTURE) { m->has_tx = 1; m->tw1 = w1; continue; }
        if (op == OP_GEOMETRYMODE) { /* B9: compose (geom & gand) | gor over the list */
            uint32_t a = w0 & 0xFFFFFFu;
            m->gor = (m->gor & a) | w1; m->gand &= a; m->has_g = 1; continue;
        }
        if (op != OP_SETOTHERMODE_H && op != OP_SETOTHERMODE_L) { m->bad = op; return 0; } /* B9: another op is a fallback, its code counted */
        unsigned len = (w0 & 0xFFu) + 1u, field = (w0 >> 8) & 0xFFu, sft = 32u - field - len;
        uint32_t mask = len >= 32 ? 0xFFFFFFFFu : ((1u << len) - 1u) << sft;
        if (op == OP_SETOTHERMODE_H) { mh |= mask; vh = (vh & ~mask) | (w1 & mask); }
        else { ml |= mask; vl = (vl & ~mask) | (w1 & mask); }
    }
    m->om_h = vh; m->om_l = vl; m->wh = mh; m->wl = ml; /* B8: partial lists compile; the read bits merge at draw (nd_model_state) */
    nd_render_state(vh, vl, 0, &m->r);
    m->ok = 1;
    return 1;
}

/* the GPU_CULLMODE values of citro3d (the register values), the cull of a native draw */
#define ND_CULL_NONE 0u
#define ND_CULL_FRONT_CCW 1u
#define ND_CULL_BACK_CCW 2u

/* ---- S3N-3d: the combine inputs of a native draw (nd_comb_decide) ---- */
/* The 16 combiner fields as op_setcombine (pm_gbi.c) decodes them into tst1, in NdCombIn.tst order. Returns 0 when the draw takes them.
 * 1: the combine reads tile 1 (the rename of op_setcombine) or the box product; both need tile state the native draw does not have (fallback).
 * -1: the combine reads KEYCENTER, KEYSCALE or K4, whose values are not native (nost). */
static int nd_comb_native(uint32_t w0, uint32_t w1, uint32_t oh, uint32_t prim, uint32_t env, uint32_t prim_lod, int tex, NdCombIn* ci) {
    uint32_t* f = ci->tst;
    f[0] = (w0 >> 20) & 0xf;  f[1] = (w1 >> 28) & 0xf;  f[2] = (w0 >> 15) & 0x1f; f[3] = (w1 >> 15) & 7;
    f[4] = (w0 >> 12) & 7;    f[5] = (w1 >> 12) & 7;    f[6] = (w0 >> 9) & 7;      f[7] = (w1 >> 9) & 7;
    f[8] = (w0 >> 5) & 0xf;   f[9] = (w1 >> 24) & 0xf;  f[10] = w0 & 0x1f;         f[11] = (w1 >> 6) & 7;
    f[12] = (w1 >> 21) & 7;   f[13] = (w1 >> 3) & 7;    f[14] = (w1 >> 18) & 7;    f[15] = w1 & 7;
    int t0 = f[0] == 1 || f[1] == 1 || f[3] == 1 || f[2] == 1 || f[2] == 8 || f[4] == 1 || f[5] == 1 || f[6] == 1 || f[7] == 1;
    int t1 = f[0] == 2 || f[1] == 2 || f[3] == 2 || f[2] == 2 || f[2] == 9 || f[4] == 2 || f[5] == 2 || f[6] == 2 || f[7] == 2;
    int c2 = f[8] == 1 || f[9] == 1 || f[11] == 1 || f[10] == 1 || f[10] == 8 || f[12] == 1 || f[13] == 1 || f[14] == 1 || f[15] == 1 ||
             f[8] == 2 || f[9] == 2 || f[11] == 2 || f[10] == 2 || f[10] == 9 || f[12] == 2 || f[13] == 2 || f[14] == 2 || f[15] == 2;
    if (t1 && !t0 && !c2) return 1;
    int box = f[0] == 1 && f[1] >= 7 && f[2] == 2 && f[3] == 7 && f[4] == 7 && f[5] == 7 && f[6] == 7 && f[7] == 2 &&
              f[8] == 3 && f[9] == 5 && f[10] == 0 && f[11] == 5 && f[12] == 7 && f[13] == 7 && f[14] == 7 && f[15] == 0;
    if (box) return 1;
    if (f[1] == 6 || f[1] == 7 || f[9] == 6 || f[9] == 7 || f[2] == 6 || f[10] == 6) return -1;
    ci->w0 = w0; ci->w1 = w1; ci->omode_h = oh; ci->prim = prim; ci->env = env; ci->aud_prim_lod = prim_lod & 0xffu;
    ci->key_c = 0; ci->key_s = 0; ci->conv_k4 = 0;
    ci->leg_mode = 0; ci->tex_on = tex; ci->use_t1 = 0; ci->t1_valid = 0; ci->dual_on = 0;
    ci->box_prod = 0; ci->box_pat = 0; ci->box_folded = 0; ci->comb_set = 1;
    ci->comb_alpha_shade = ((w0 >> 12) & 7) == 4 || ((w0 >> 9) & 7) == 4 || ((w1 >> 12) & 7) == 4 || ((w1 >> 9) & 7) == 4;
    return 0;
}

/* ---- S3N-3a: the state of one model record as a native draw (pure: record words and NdMCtx) ---- */
int nd_model_state(const uint32_t* rec, const NdMCtx* c, NdDraw* d, NdMState* st) {
    st->why = 0;
    const uint32_t f = rec[6]; /* R3 w0 flags */
    if (!c->mode || !c->mode->ok || (!c->has_comb && !c->mode->has_cb)) return (st->why = 1, 0);
    if (c->gv && c->gv->fxon) return (st->why = 4, 0); /* lit or fog: pm5 path, later row */
    if (rec[26] != 0) return (st->why = 2, 0);          /* R13: words after the record (special tile list, untextured ops) */
    if (rec[22] != 0 || rec[23] != 0) return (st->why = 3, 0); /* R11: custom gfx pre and post */
    const int tex_u = c->u0 != NULL && c->u0->dim != 0; /* the unit exists: tex_on of the combine decision */
    const uint32_t w0 = c->mode->has_cb ? c->mode->cw0 : c->comb_w0, w1 = c->mode->has_cb ? c->mode->cw1 : c->comb_w1; /* B9: the list combine wins (rm_apply runs after the entry) */
    const uint32_t geom = (c->geom_base & c->mode->gand) | c->mode->gor; /* B9: geometry after the list ops */
    const int zb = (geom & 1u) != 0; /* G_ZBUFFER */
    const unsigned ct = (geom & 0x600u) == 0x200u ? 1u : (geom & 0x600u) == 0x400u ? 2u : 0u; /* G_CULL_FRONT, G_CULL_BACK (pm_gbi ct) */
    int ccw = c->wind <= 0; if (ct == 2) ccw = !ccw;
    const uint8_t cull = c->cull_set ? (uint8_t)c->cull : (uint8_t)(ct ? (ccw ? ND_CULL_BACK_CCW : ND_CULL_FRONT_CCW) : ND_CULL_NONE);
    const uint32_t oh = (c->base_oh & ~c->mode->wh) | c->mode->om_h;
    const uint32_t ol = (c->base_ol & ~c->mode->wl) | c->mode->om_l;
    if (((ND_OMH_READ & ~c->mode->wh) & ~c->known_h) || ((ND_OML_READ & ~c->mode->wl) & ~c->known_l)) return -1; /* nost */
    NdRender rr;
    nd_render_state(oh, ol, 0, &rr);
    const NdRender* r = &rr;
    const int two = ((oh >> 20) & 3u) == 1u; /* 2 cycle (from the merged other mode) */
    const uint32_t prim = (f & 4u) ? rec[12] : c->prim; /* R6: the prim of the draw */
    /* S3N-3d: the combine choice is the walker's own function (nd_comb_decide) on the native inputs */
    NdCombIn ci;
    NdCombOut co;
    { int rc = nd_comb_native(w0, w1, oh, prim, c->env, c->prim_lod, tex_u, &ci); if (rc == 1) return (st->why = 8, 0); if (rc < 0) return (st->why = 8, -1); }
    nd_comb_decide(&ci, &co);
    const int combine = co.plan_on ? 8 : co.leg;
    const int tex = tex_u && combine >= 2; /* the walker reads a texel in the combine modes 2 and up only */
    if (combine == 8 && (co.plan.uses_tex1 || co.plan.uses_noise)) return (st->why = 6, 0);
    NdTmplKey k;
    memset(&k, 0, sizeof k);
    k.pass = 0; k.raw = 1; k.cull = cull;
    k.z = (uint8_t)zb; k.zmark = 0; k.zcmp = r->zcmp; k.zupd = r->zupd; k.nocol = r->nocol; k.decal = r->decal;
    memcpy(&k.dbias, &c->dbias, 4);
    k.atest_on = r->atest >= 0; k.aref = (uint8_t)(r->atest >= 0 ? r->atest : 0);
    k.combine = (uint8_t)combine; k.tex = (uint8_t)tex; k.tex1 = 0; k.i8_0 = (uint8_t)(tex && c->i8_0); k.i8_1 = 0;
    k.rgb_scale = (uint8_t)c->rgb_scale; k.r_mix = (uint8_t)c->r_mix; k.draw_3d = (uint8_t)c->draw_3d; k.rect_2d = 0;
    k.dview = c->dview != 0; k.off_active = 0; k.off_i8 = 0;
    k.r_memblend = (uint8_t)(r->set_memb ? r->memb : c->r_memblend);
    k.r_replace = (uint8_t)c->r_replace; k.r_blend = r->blend; k.noise = 0;
    k.w0 = combine == 8 ? w0 : 0; k.w1 = combine == 8 ? w1 : 0; k.two = combine == 8 ? (uint8_t)two : 0; k.t1only = 0;
    int id = nd_tmpl_build(&k);
    if (id < 0) return (st->why = 7, 0);
    NdTevIn ti = { &k, combine == 8 ? &co.plan : NULL, prim, c->cconst, c->mixcol };
    NdTevOut to;
    nd_tev_build(&ti, &to);
    memset(d, 0, sizeof *d);
    d->tmpl = id;
    for (int i = 0; i < 6; i++) d->tev_col[i] = to.st[i][3];
    d->buf_col = to.buf_color; d->has_buf = (uint8_t)to.buf_set;
    if (tex) d->u[0] = *c->u0;
    d->u[0].force = 0;
    if (c->gv) {
        memcpy(d->uvso, c->gv->uvso, 16); memcpy(d->cmul, c->gv->cm, 16); memcpy(d->cadd, c->gv->ca, 16);
        d->zmix[0] = c->gv->zm[0]; d->zmix[1] = c->gv->zm[1];
    }
    memcpy(&st->k, &k, sizeof k); st->to = to; st->tmpl = id;
    return 1;
}
