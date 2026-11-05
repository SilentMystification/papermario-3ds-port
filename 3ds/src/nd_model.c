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
    m->ok = 0;
    for (unsigned i = 0; i < 8; i++) {
        uint32_t w0 = w[2 * i], w1 = w[2 * i + 1];
        uint32_t op = w0 >> 24;
        if (op == OP_ENDDL) break;
        if (op != OP_SETOTHERMODE_H && op != OP_SETOTHERMODE_L) return 0; /* B6: another op is a fallback */
        unsigned len = (w0 & 0xFFu) + 1u, field = (w0 >> 8) & 0xFFu, sft = 32u - field - len;
        uint32_t mask = len >= 32 ? 0xFFFFFFFFu : ((1u << len) - 1u) << sft;
        if (op == OP_SETOTHERMODE_H) { mh |= mask; vh = (vh & ~mask) | (w1 & mask); }
        else { ml |= mask; vl = (vl & ~mask) | (w1 & mask); }
    }
    if ((mh & ND_OMH_READ) != ND_OMH_READ || (ml & ND_OML_READ) != ND_OML_READ) return 0; /* B5 */
    m->om_h = vh; m->om_l = vl;
    nd_render_state(vh, vl, 0, &m->r);
    m->ok = 1;
    return 1;
}
