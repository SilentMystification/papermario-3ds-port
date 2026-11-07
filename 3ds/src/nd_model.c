/* nd_model.c: native model draws (plan S3N). S3N-1: the draw parameters of a raw triangle (nd_gv_params), moved from pm_gbi.c gv_params_calc.
 * Pure C: the host test builds it (3ds/tools/nd_gv_test.c). No walker state and no pm_gpu call inside: the caller passes the values. */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
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

/* MDL-1 (oracle decision): an R13 list is the record's texture block when every op is a texture, tile, load, sync, state or NOOP op. The walker
 * runs the list in place before the native draw (rec_aux), so its state is the bound tile state. cw: the combine words (G_SETCOMBINE, 0xFC) and
 * the texture scale (G_TEXTURE, 0xD7, cw[2] w1, cw[3] = 1) of the block. Returns 1 for a texture block, 0 for any other op (why 2, counted). */
int nd_r13_texonly(const uint32_t* aw, unsigned n, uint32_t* cw, int* has_cb) {
    *has_cb = 0; cw[3] = 0;
    if (n == 0 || n > 32) return 0;
    for (unsigned i = 0; i < n; i++) {
        const uint32_t w0 = aw[2 * i], w1 = aw[2 * i + 1];
        switch (w0 >> 24) {
        case 0xFD: case 0xF5: case 0xF2: case 0xF4: case 0xF3: case 0xF0: /* SETTIMG, SETTILE, SETTILESIZE, LOADTILE, LOADBLOCK, LOADTLUT */
        case 0xE8: case 0xE7: case 0xE6: /* RDPTILESYNC, PIPESYNC, LOADSYNC */
        case 0xE3: case 0xE2: /* G_SETOTHERMODE_L, _H: the texture block's render state (the block carries one) */
        case 0x00: /* NOOP */
            break;
        case 0xFC: /* G_SETCOMBINE */
            cw[0] = w0; cw[1] = w1; *has_cb = 1; /* the full word, as the walker key has it (the op byte included) */
            break;
        case 0xD7: /* G_TEXTURE: the texture scale */
            cw[2] = w1; cw[3] = 1;
            break;
        default:
            return 0;
        }
    }
    return 1;
}

/* ---- S3N-1 (B4): texture descriptors of the map model handles ---- */
#define ND_HANDLE_N 128
static NdTexDesc hd_desc[ND_HANDLE_N];
static const void* hd_gfx[ND_HANDLE_N];
static uint8_t hd_ok[ND_HANDLE_N];
static unsigned hd_gen = 1; /* S3N-6: bumped by each handle write */

void nd_tex_handle_set(int index, const void* gfx, const void* raster, const void* palette, unsigned w, unsigned h, unsigned fmt, unsigned siz,
                       unsigned wrap_w, unsigned wrap_h, unsigned masks, unsigned maskt, unsigned extra, int has_aux) {
    if (index < 0 || index >= ND_HANDLE_N) return;
    NdTexDesc d;
    memset(&d, 0, sizeof d);
    int ok = !(!raster || w == 0 || h == 0 || siz > 3); /* B4 (S3L-2): the desc is tile 0 (the main raster), so every extra form has it: mipmaps (extra 1), aux (2, 3). Tile 1 facts are not in the desc: a combine that reads tile 1 falls back (why 6, nd_model_state) */
    (void)extra; (void)has_aux;
    if (ok) {
        d.img = raster; d.pal = palette;
        d.fmt = (uint8_t)fmt; d.siz = (uint8_t)siz; d.cms = (uint8_t)wrap_w; d.cmt = (uint8_t)wrap_h; d.masks = (uint8_t)masks; d.maskt = (uint8_t)maskt;
        d.w = (uint16_t)w; d.h = (uint16_t)h; d.stride = (uint16_t)w;
        d.pal_n = palette ? (siz == 0 ? 16u : siz == 1 ? 256u : 0u) : 0u; /* gDPLoadTLUT_pal16, pal256 */
    }
    /* S3N-6: the handle generation moves only when the handle changes (a reload of the same descriptor keeps the cached draws) */
    if (hd_ok[index] != ok || hd_gfx[index] != gfx || (ok && memcmp(&hd_desc[index], &d, sizeof d) != 0)) hd_gen++;
#ifdef TARGET_3DS
    { /* S3L-2 (diagnostic): the first 16 handles without a desc, with their form */
        extern void pm_log_prof_line(const char* fmt, ...);
        static unsigned nform; if (!ok && nform++ < 16u) pm_log_prof_line("ndmform idx %d w %u h %u fmt %u siz %u extra %u aux %d raster %d pal %d", index, w, h, fmt, siz, extra, has_aux, raster != NULL, palette != NULL);
    }
#endif
    hd_gfx[index] = gfx; hd_ok[index] = (uint8_t)ok;
    if (ok) hd_desc[index] = d;
}

const NdTexDesc* nd_tex_handle_desc(int index, const void* gfx) {
    if (index < 0 || index >= ND_HANDLE_N || !hd_ok[index] || hd_gfx[index] != gfx) return NULL;
    return &hd_desc[index];
}

/* S3L-2: the reason of a handle with no desc: 9 the handle is set but its form is not in B4, 10 the handle index holds another gfx (never set, or set by a failed load) */
int nd_tex_handle_miss(int index, const void* gfx) {
    if (index < 0 || index >= ND_HANDLE_N || hd_gfx[index] != gfx) return 10;
    return 9;
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

/* S3L-3: the R13 words (aux, the words the texture block wrote after the record; the walker runs them in place of the bind) as a pre-list mode:
 * the same ops as a render mode list, applied before it. */
int nd_aux_state(const uint32_t* aw, unsigned n, NdMode* ax) {
    uint32_t tmp[18];
    if (n == 0 || n > 8) return 0;
    memcpy(tmp, aw, n * 2u * sizeof(uint32_t));
    if (n < 8) { tmp[2 * n] = (uint32_t)OP_ENDDL << 24; tmp[2 * n + 1] = 0; }
    if (!nd_mode_compile(ax, (const unsigned*)tmp)) return 0;
    return !ax->has_g;
}

/* the GPU_CULLMODE values of citro3d (the register values), the cull of a native draw */
#define ND_CULL_NONE 0u
#define ND_CULL_FRONT_CCW 1u
#define ND_CULL_BACK_CCW 2u

/* ---- S3N-3d: the combine inputs of a native draw (nd_comb_decide) ---- */
/* The 16 combiner fields as op_setcombine (pm_gbi.c) decodes them into tst1, in NdCombIn.tst order. Returns 0 when the draw takes them.
 * 1: the combine reads tile 1 (the rename of op_setcombine) or the box product; both need tile state the native draw does not have (fallback).
 * -1: the combine reads KEYCENTER, KEYSCALE or K4, whose values are not native (nost). */
static int nd_comb_native(uint32_t w0, uint32_t w1, uint32_t oh, uint32_t prim, uint32_t env, uint32_t prim_lod, int tex, int t1v, NdCombIn* ci) {
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
    ci->leg_mode = 0; ci->tex_on = tex; ci->use_t1 = 0; ci->t1_valid = t1v; ci->dual_on = 0;
    ci->box_prod = 0; ci->box_pat = 0; ci->box_folded = 0; ci->comb_set = 1;
    ci->comb_alpha_shade = ((w0 >> 12) & 7) == 4 || ((w0 >> 9) & 7) == 4 || ((w1 >> 12) & 7) == 4 || ((w1 >> 9) & 7) == 4;
    return 0;
}

/* ---- S3N-3a: the state of one model record as a native draw (pure: record words and NdMCtx) ---- */
int pm_debug_has(const char* name);
void pm_log_d(const char* fmt, ...);
void pm_log_prof_line(const char* fmt, ...);
static int nd_model_state_in(const uint32_t* rec, const NdMCtx* c, NdDraw* d, NdMState* st);
/* MDL-0b: every why code of nd_model_state, uncapped, one line per 300 frames (mdlw) */
static unsigned mw_cnt[32], mw_ok, mw_f = 100u; /* mw_f: the frame of the last line + 300 */
unsigned pm_frame_count(void);
int nd_model_state(const uint32_t* rec, const NdMCtx* c, NdDraw* d, NdMState* st) {
    int r = nd_model_state_in(rec, c, d, st);
    if (r == 1) mw_ok++; else mw_cnt[st->why & 31]++;
    unsigned f = pm_frame_count();
    if (f >= mw_f) { mw_f = f - f % 100u + 100u; /* the next line at the next multiple of 100 frames (pm_frame_count, the shot frame) */
        char b[200]; unsigned n = 0; for (unsigned i = 0; i < 32; i++) if (mw_cnt[i]) n += (unsigned)snprintf(b + n, sizeof b - n, " %u:%u", i, mw_cnt[i]);
        extern unsigned nd_r13_nohit; pm_log_prof_line("mdlw f%u ok %u why%s r13nohit %u", f, mw_ok, n ? b : " none", nd_r13_nohit); nd_r13_nohit = 0; memset(mw_cnt, 0, sizeof mw_cnt); mw_ok = 0; }
    return r;
}
static int nd_model_state_in(const uint32_t* rec, const NdMCtx* c, NdDraw* d, NdMState* st) {
    st->why = 0;
    const uint32_t f = rec[6]; /* R3 w0 flags */
    if (!c->mode || !c->mode->ok || (!c->has_comb && !c->mode->has_cb)) return (st->why = 1, 0);
    if (c->gv && c->gv->fxon && (!c->nrm_ok || pm_debug_has("pm5off"))) return (st->why = 4, 0); /* lit or fog without the walker normal rows (PM5-3: with them the range is a pm5 draw) */
    if (rec[26] != 0 && !c->aux_ok) { /* MDL-0 count (switch mdl0): the first 12 why 2 records, their R13 words */
        static unsigned m2_ = 0; if (m2_ < 12u && pm_debug_has("mdl0")) { const uint32_t* aw = rec + NDMC_RW; m2_++; pm_log_d("mdl2 n %u w0 %08x %08x w1 %08x %08x gfx %d", (unsigned)rec[26], aw[0], aw[1], aw[2], aw[3], c->aux_ok); }
        return (st->why = 2, 0); } /* R13: words after the record (special tile list, untextured ops); S3L-3: the untextured form is decoded by the caller (aux_ok) */
    if (rec[22] != 0 || rec[23] != 0) { /* MDL-0 count (switch mdl0): the first 12 why 3 records: list addresses and the first two words of each list */
        static unsigned m3_ = 0; if (m3_ < 12u && pm_debug_has("mdl0")) { m3_++; const uint32_t* a = rec[22] ? (const uint32_t*)(uintptr_t)rec[22] : NULL; const uint32_t* b = rec[23] ? (const uint32_t*)(uintptr_t)rec[23] : NULL;
            pm_log_d("mdl3 pre %08x post %08x pre w %08x %08x post w %08x %08x", (unsigned)rec[22], (unsigned)rec[23], a ? a[0] : 0u, a ? a[1] : 0u, b ? b[0] : 0u, b ? b[1] : 0u); }
        return (st->why = 3, 0); } /* R11: custom gfx pre and post */
    const int tex_u = c->u0 != NULL && c->u0->dim != 0; /* the unit exists: tex_on of the combine decision */
    const uint32_t w0 = c->mode->has_cb ? c->mode->cw0 : c->comb_w0, w1 = c->mode->has_cb ? c->mode->cw1 : c->comb_w1; /* B9: the list combine wins (rm_apply runs after the entry) */
    const uint32_t geom = c->geom_at_set ? c->geom_at : (c->geom_base & c->mode->gand) | c->mode->gor; /* B9: geometry after the list ops; S3N-3c: or the batch's */
    const int zb = (geom & 1u) != 0; /* G_ZBUFFER */
    const unsigned ct = (geom & 0x600u) == 0x200u ? 1u : (geom & 0x600u) == 0x400u ? 2u : 0u; /* G_CULL_FRONT, G_CULL_BACK (pm_gbi ct) */
    int ccw = c->wind <= 0; if (ct == 2) ccw = !ccw;
    const uint8_t cull = (uint8_t)(ct ? (ccw ? ND_CULL_BACK_CCW : ND_CULL_FRONT_CCW) : ND_CULL_NONE); /* S3N-3c: from the draw's geometry, no walker cull */
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
    /* MDL-1b: tile 1 valid from the walker state after pm_gbi_bind_tile1 (the ctx t1_valid, set before the draw): one decision */
    const int t1v = c->t1_valid;
    { int rc = nd_comb_native(w0, w1, oh, prim, c->env, c->prim_lod, tex_u, t1v, &ci); if (rc == 1) return (st->why = 8, 0); if (rc < 0) return (st->why = 8, -1); }
    nd_comb_decide(&ci, &co);
    const int combine = co.plan_on ? 8 : co.leg;
    const int tex = tex_u && combine >= 2; /* the walker reads a texel in the combine modes 2 and up only */
    /* MDL-1b: tex1 is native when the R13 ctx has unit 1 (the walker's plan reads it); noise and pm5 with tex1 stay why 6 (named) */
    if (combine == 8 && (co.plan.uses_noise || (co.plan.uses_tex1 && (!c->u1 || c->gv && c->gv->fxon)))) return (st->why = 6, 0);
    const int t1n = combine == 8 && co.plan.uses_tex1;
    if (c->zmark && zb && r->decal) return (st->why = 15, 0); /* S5-2: zmark with decal (the walker's pass 2 bias): named fallback */
    NdTmplKey k;
    memset(&k, 0, sizeof k);
    k.pass = 0; k.raw = 1; k.cull = cull;
    /* S5-1 (b): decal with blend, the walker's decal2: pass A (stencil mark) then pass B (colour, stencil EQUAL). The model draw emits both (nd_model_draw). */
    if (zb && r->zcmp && r->decal && r->blend && !r->nocol) k.pass = 2;
    k.z = (uint8_t)zb; k.zmark = (uint8_t)(c->zmark && zb); k.zcmp = r->zcmp; k.zupd = r->zupd; k.nocol = r->nocol; k.decal = r->decal;
    memcpy(&k.dbias, &c->dbias, 4);
    k.atest_on = r->atest >= 0; k.aref = (uint8_t)(r->atest >= 0 ? r->atest : 0);
    k.combine = (uint8_t)combine; k.tex = (uint8_t)tex; k.tex1 = (uint8_t)t1n; k.i8_0 = (uint8_t)(tex && c->i8_0); k.i8_1 = (uint8_t)(t1n && c->i8_1);
    k.rgb_scale = (uint8_t)c->rgb_scale; k.r_mix = (uint8_t)c->r_mix; k.draw_3d = (uint8_t)c->draw_3d; k.rect_2d = 0;
    k.dview = c->dview != 0; k.off_active = 0; k.off_i8 = 0;
    k.r_memblend = (uint8_t)(r->set_memb ? r->memb : c->r_memblend);
    k.r_replace = (uint8_t)c->r_replace; k.r_blend = r->blend; k.noise = 0;
    k.w0 = combine == 8 ? w0 : 0; k.w1 = combine == 8 ? w1 : 0; k.two = combine == 8 ? (uint8_t)two : 0; k.t1only = (uint8_t)(t1n ? c->t1only : 0);
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
    if (t1n) { d->u[1] = *c->u1; memcpy(d->uvx, c->uvx, 16); d->loc_uvx = c->loc_uvx; d->has_uvx = 1; } /* MDL-1b: unit 1 and its uv map (the raw path's lines) */
    d->u[0].force = 0;
    if (c->gv) {
        memcpy(d->uvso, c->gv->uvso, 16); memcpy(d->cmul, c->gv->cm, 16); memcpy(d->cadd, c->gv->ca, 16);
        d->zmix[0] = c->gv->zm[0]; d->zmix[1] = c->gv->zm[1];
        if (c->gv->fxon) { d->use5 = 1; d->fx5 = c->gv->fx; memcpy(d->nrm5, c->nrm5, sizeof d->nrm5); }
    }
    memcpy(&st->k, &k, sizeof k); st->to = to; st->tmpl = id;
    return 1;
}

/* ---- S3N-3c: a model record as native draws. The palette 0 is post x the combined matrix of the walker's stack; the ranges are the static groups of the
 * shape list in view (pm_gbi.c pm_dlc_native_ranges). Ranges with the same state key and contiguous indices merge into one draw. ---- */
static NdNat nat[ND_NAT_MAX];
static int nat_n;
int nd_nat_why; /* the reason of the last fallback: the nd_model_state why, or 11 too many draws, 12 the list is not static */
const NdNat* nd_model_native(unsigned j) { return j < (unsigned)nat_n ? &nat[j] : NULL; }
static NdRng nat_rng[ND_RAW_MAX]; /* S3N-6: the static ranges of the last nd_model_draw (copied at its start) */
static int nat_nr; static uint32_t nat_geom;

/* ---- S3N-6: the native model cache (see nd.h NdMKey). The draws of an entry live in a pool; an entry names its first draw and the count. ---- */
#define NDMC_N 1024u
#define NDMC_POOL 512u
static NdMEnt mc_tab[NDMC_N];
static NdNat mc_pool[NDMC_POOL];
static unsigned mc_n, mc_pool_n, mc_clears_n;
unsigned nd_handle_gen(void) { return hd_gen; }
unsigned nd_mc_clears(void) { return mc_clears_n; }
/* S3N-7: the key words the cache compares: the record words (0 to 27), the nd_st base bits (28 to 32), and the handle and dlc generations (53, 54: a map load reuses the geometry addresses). The walker words (33 to 91) stay in the entry
 * for the ndmcache diagnostics (pm_gpu.c ndc_cmp): the words that a diff depends on are added here, with the reason in agent_s3n.md. */
static const uint8_t ndmc_cmp[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 38, 53, 54 }; /* S3L-3: 38 = the R13 words (hash) */
#define NDMC_NCMP ((unsigned)(sizeof ndmc_cmp / sizeof ndmc_cmp[0]))
static int mc_eq(const NdMKey* a, const NdMKey* b) { for (unsigned i = 0; i < NDMC_NCMP; i++) if (a->w[ndmc_cmp[i]] != b->w[ndmc_cmp[i]]) return 0; return 1; }
uint32_t nd_mc_hash(const NdMKey* k) {
    uint32_t h = 2166136261u;
    for (unsigned i = 0; i < NDMC_NCMP; i++) { h ^= k->w[ndmc_cmp[i]]; h *= 16777619u; }
    return h ^ (h >> 15);
}
/* S3N-7 diagnostics: the entry last stored per record address (slot = rp >> 2 and 1023), with the clear generation it was stored in. A miss is classed
 * against that entry: new (no entry for this record), clr (cleared since), else the first key word that differs (rec: record words, st: walker state, hd: handle
 * and dlc generations, gv: walker gv state), same (equal key, not found). ent and pool: the high water of the table and the pool. */
#define NDMC_LAST 1024u
static uint32_t mc_lastp[NDMC_LAST]; static uint16_t mc_lasti[NDMC_LAST]; static unsigned mc_lastg[NDMC_LAST];
static unsigned mg_cls[9], mc_ent_hi, mc_pool_hi, mc_ins;
void nd_mc_miss(const NdMKey* k) {
    unsigned s = (k->rp >> 2) & (NDMC_LAST - 1u);
    if (mc_lastp[s] != k->rp) { mg_cls[0]++; return; }
    if (mc_lastg[s] != mc_clears_n) { mg_cls[1]++; return; }
    const NdMEnt* e = &mc_tab[mc_lasti[s]];
    for (unsigned j = 0; j < NDMC_RW + NDMC_SW; j++) {
        if (e->k.w[j] == k->w[j]) continue;
        if (j < NDMC_RW) mg_cls[2]++;
        else if (j == NDMC_RW + 25u || j == NDMC_RW + 26u) mg_cls[4]++;
        else if (j < NDMC_RW + 27u) mg_cls[3]++;
        else mg_cls[5]++;
        return;
    }
    mg_cls[6]++;
}
unsigned nd_mc_size(void) { return NDMC_N; }
const NdMEnt* nd_mc_ent(unsigned i) { return &mc_tab[i]; }
void nd_mc_diag_take(unsigned o[15]) {
    for (unsigned i = 0; i < 9; i++) { o[i] = mg_cls[i]; mg_cls[i] = 0; }
    o[9] = mc_ins; mc_ins = 0; o[10] = mc_n; o[11] = mc_pool_n; o[12] = mc_ent_hi; o[13] = mc_pool_hi; o[14] = NDMC_N;
}
static void mc_clear_all(void) {
    for (unsigned i = 0; i < NDMC_N; i++) mc_tab[i].used = 0;
    mc_n = 0; mc_pool_n = 0; mc_clears_n++;
}
/* the hash is set once per record (pm_gpu_model_key): the probe compares it before the key words */
NdMEnt* nd_mc_find(const NdMKey* k) {
    uint32_t h = k->h & (NDMC_N - 1u);
    for (unsigned p = 0; p < NDMC_N; p++, h = (h + 1u) & (NDMC_N - 1u)) {
        NdMEnt* e = &mc_tab[h];
        if (!e->used) return NULL;
        if (e->k.h == k->h && mc_eq(&e->k, k)) return e;
    }
    return NULL;
}
static NdMEnt* mc_slot(const NdMKey* k) {
    if (mc_n >= NDMC_N / 4u * 3u) mc_clear_all();
    uint32_t h = k->h & (NDMC_N - 1u);
    for (;;) {
        NdMEnt* e = &mc_tab[h];
        if (!e->used) { e->used = 1; e->k = *k; mc_n++; mc_ins++; if (mc_n > mc_ent_hi) mc_ent_hi = mc_n; return e; }
        if (e->k.h == k->h && mc_eq(&e->k, k)) return e;
        h = (h + 1u) & (NDMC_N - 1u);
    }
}
static void mc_note(const NdMKey* k, const NdMEnt* e) { /* S3N-7: the last entry of this record address (diagnostic classes) */
    unsigned s = (k->rp >> 2) & (NDMC_LAST - 1u);
    mc_lastp[s] = k->rp; mc_lasti[s] = (uint16_t)(e - mc_tab); mc_lastg[s] = mc_clears_n;
}
NdMEnt* nd_mc_store_neg(const NdMKey* k, unsigned ep, unsigned why) {
    NdMEnt* e = mc_slot(k);
    e->neg = 1; e->why = (uint8_t)(why & 15u); e->ep = ep; e->tex = 0; e->hs = -1; e->hg = 0; e->n = 0; e->nraw = 0; e->off = mc_pool_n; e->geom = 0;
    mc_note(k, e);
    return e;
}
NdMEnt* nd_mc_store(const NdMKey* k, int tex, int hs, unsigned hg) {
    if (nat_nr > ND_RAW_MAX || nat_nr < 0) { mg_cls[7]++; return NULL; }
    if (nat_n > (int)ND_NAT_MAX) { mg_cls[8]++; return NULL; }
    NdMEnt* e = mc_slot(k);
    if (mc_pool_n + (unsigned)nat_n > NDMC_POOL) { mc_clear_all(); e = mc_slot(k); }
    memcpy(e->raw, nat_rng, sizeof(NdRng) * (unsigned)nat_nr);
    e->nraw = (unsigned)nat_nr; e->geom = nat_geom;
    e->tex = (uint8_t)(tex != 0); e->hs = hs; e->hg = hg; e->neg = 0;
    e->off = mc_pool_n; e->n = (unsigned)nat_n;
    for (int j = 0; j < nat_n; j++) {
        NdNat* q = &mc_pool[mc_pool_n + (unsigned)j];
        *q = nat[j];
        q->d.pal = (const float(*)[16])q->pal; /* the pointer of the copy, not of nat[] */
    }
    mc_pool_n += (unsigned)nat_n;
    if (mc_pool_n > mc_pool_hi) mc_pool_hi = mc_pool_n;
    mc_note(k, e);
    return e;
}
NdNat* nd_mc_nat(NdMEnt* e, unsigned j) { return &mc_pool[e->off + j]; }

#ifdef TARGET_3DS
extern float pm_eye_e, pm_eye_conv;
void pm_gbi_gv_at(unsigned tss, uint32_t geo, int tw, int th, NdGvP* o);
void pm_gpu_pcomp_native(float o[16], const float m[4][4]);
void pm_gpu_native_submit_draw(NdDraw* d, const uint16_t* sd, unsigned nidx, int tex);
void pm_gpu_flush(void);

int nd_model_draw(const uint32_t* rec, const NdMCtx* c0, const void* list, int emit) {
    static NdRng rng[512];
    float comb[4][4], pal0[16];
    nat_n = 0; nd_nat_why = 0;
    if (!c0->mode || !c0->mode->ok) return (nd_nat_why = 1, -1);
    nd_mtx_combine(comb, pm_eye_e, pm_eye_conv);
    pm_gpu_pcomp_native(pal0, comb);
    const uint32_t entry = (c0->geom_base & c0->mode->gand) | c0->mode->gor;
    int nr = pm_dlc_native_ranges(list, entry, rng, 512u);
    if (nr < 0) return (nd_nat_why = nr == -2 ? 11 : 12, -1);
    nat_nr = nr <= ND_RAW_MAX ? nr : -1; nat_geom = entry;
    if (nat_nr >= 0) memcpy(nat_rng, rng, sizeof(NdRng) * (unsigned)nr);
    for (int r = 0; r < nr; r++) {
        NdMCtx c = *c0;
        NdGvP gv;
        pm_gbi_gv_at(c0->tss, rng[r].geom, c0->tex_w, c0->tex_h, &gv);
        c.gv = &gv; c.rgb_scale = gv.boost ? 2 : 1; /* the walker sets rgb_scale from the boost of the batch (dlc_seg) */
        c.geom_at = rng[r].geom; c.geom_at_set = 1;
        NdDraw dd; NdMState ss;
        int rc = nd_model_state(rec, &c, &dd, &ss);
        if (rc != 1) { nat_n = 0; nd_nat_why = ss.why; return rc < 0 ? -2 : -1; }
        const int d2 = ss.k.pass == 2; /* S5-1: decal2, no merge (each range has its own pass A) */
        if (!d2 && nat_n && nat[nat_n - 1].d.tmpl == dd.tmpl && nat[nat_n - 1].sd + nat[nat_n - 1].nidx == rng[r].sd) { nat[nat_n - 1].nidx += rng[r].nidx; continue; }
        if (nat_n + (d2 ? 2 : 1) > ND_NAT_MAX) { nat_n = 0; return (nd_nat_why = 11, -1); }
        if (d2) { /* pass A: the stencil mark, no colour, no depth write */
            NdTmplKey ka = ss.k; ka.pass = 1;
            int ida = nd_tmpl_build(&ka);
            if (ida < 0) { nat_n = 0; return (nd_nat_why = 7, -1); }
            NdNat* a = &nat[nat_n++];
            memcpy(a->pal, pal0, sizeof pal0);
            a->d = dd; a->d.tmpl = ida; a->d.pal = (const float(*)[16])a->pal; a->d.npal = 1;
            a->s = ss; a->sd = rng[r].sd; a->nidx = rng[r].nidx;
        }
        NdNat* n = &nat[nat_n++];
        memcpy(n->pal, pal0, sizeof pal0);
        dd.pal = (const float(*)[16])n->pal; dd.npal = 1;
        n->d = dd; n->s = ss; n->sd = rng[r].sd; n->nidx = rng[r].nidx; n->zp = 0; n->zn = 0.f;
        if (ss.k.zmark) { /* S5-2 (a): zmark pass 2 after the pass 1 draw: the same triangles, constant depth zn * w (pal row 2 := zn * row 3 = z clip of the walker's b2.z), stencil EQUAL cleared */
            if (nat_n >= ND_NAT_MAX) { nat_n = 0; return (nd_nat_why = 11, -1); }
            NdTmplKey kb = ss.k; kb.pass = 3; kb.zmark = 0;
            int idb = nd_tmpl_build(&kb);
            if (idb < 0) { nat_n = 0; return (nd_nat_why = 7, -1); }
            NdNat* b = &nat[nat_n++];
            memcpy(b->pal, pal0, sizeof pal0);
            for (int i = 0; i < 4; i++) b->pal[8 + i] = c.zn * pal0[12 + i];
            b->d = dd; b->d.tmpl = idb; b->d.pal = (const float(*)[16])b->pal; b->d.npal = 1;
            b->s = ss; b->s.k = kb; b->sd = rng[r].sd; b->nidx = rng[r].nidx; b->zp = 1; b->zn = c.zn;
        }
    }
    if (emit) {
        pm_gpu_flush(); /* the walker batch open before this record goes out first */
        for (int j = 0; j < nat_n; j++) pm_gpu_native_submit_draw(&nat[j].d, nat[j].sd, nat[j].nidx, nat[j].d.u[0].dim != 0);
    }
    return nat_n;
}
#endif
