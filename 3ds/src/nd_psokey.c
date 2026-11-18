/* nd_psokey.c: PSOFB, the device part of the pso decision: the key compare of the table lookup
 * (nd_psolk.c) and the constant colour. The decision is host only (3ds/tools/nd_decide.c, gen_pso_tab). */
#include <stdint.h>
#include <string.h>
#include "ultra64.h"
#include "nd.h"
#include "nd_api.h"
#include "pm_gpu.h"

void unpack8(unsigned c, float o[4]); /* nd_tex_set.c */
extern int pm_comb_vprim;             /* pm_comb.c */

/* ---- the combiner constants (cc_const_in, cc_alpha_k, const_rgb_tex_alpha) ---- */
static float dc_const_in(int slot, unsigned sel, int ch, const float* pr, const float* en, unsigned conv_k4) {
    if (sel == 0) return -2.f;
    if (sel == 3) return pr[ch];
    if (sel == 5) return en[ch];
    if (slot == 0 && sel == 6) return 1.f;
    if (slot == 3 && sel == 6) return 1.f;
    if (slot == 3 && sel == 7) return 0.f;
    if (slot == 1 && sel == 7) return (float)conv_k4 / 255.f;
    if (slot == 1 && sel > 7) return 0.f;
    if (slot == 0 && sel >= 8) return 0.f;
    if (slot == 2 && sel == 10) return pr[3];
    if (slot == 2 && sel == 12) return en[3];
    if (slot == 2 && sel >= 16) return 0.f;
    return -1.f;
}

static float dc_alpha_k(unsigned aa, unsigned ab, unsigned ac, unsigned ad, const float* pr, const float* en) {
    if (ab == 7 && ad == 7 && aa == 1 && (ac == 3 || ac == 5)) return ac == 3 ? pr[3] : en[3];
    if (ab == 7 && ad == 7 && ac == 1 && (aa == 3 || aa == 5)) return aa == 3 ? pr[3] : en[3];
    if (aa == 2 && ab == 1 && ad == 1 && (ac == 3 || ac == 5)) return 1.f;
    if (aa == 3 && ab == 0 && ac == 1 && ad == 0) return pr[3];
    return -1.f;
}

/* A combiner whose colour needs no texel and no shade and whose alpha is a constant times TEXEL0 alpha (const_rgb_tex_alpha). 1 and the RGBA constant. */
int nd_const_rgb(const NdCombIn* c, unsigned* out) {
    float pr[4], en[4], rgb[3], k, k2;
    int ch;
    const uint32_t* t = c->tst;
    int two = ((c->omode_h >> G_MDSFT_CYCLETYPE) & 3) == 1;
    if (((c->omode_h >> G_MDSFT_CYCLETYPE) & 3) > 1 || !c->comb_set || !c->tex_on) return 0;
    unpack8(c->prim, pr);
    unpack8(c->env, en);
    for (ch = 0; ch < 3; ch++) {
        float a, b, cc, d;
        if (two) {
            a = dc_const_in(0, t[8], ch, pr, en, c->conv_k4); b = dc_const_in(1, t[9], ch, pr, en, c->conv_k4);
            cc = dc_const_in(2, t[10], ch, pr, en, c->conv_k4); d = dc_const_in(3, t[11], ch, pr, en, c->conv_k4);
        } else {
            a = dc_const_in(0, t[0], ch, pr, en, c->conv_k4); b = dc_const_in(1, t[1], ch, pr, en, c->conv_k4);
            cc = dc_const_in(2, t[2], ch, pr, en, c->conv_k4); d = dc_const_in(3, t[3], ch, pr, en, c->conv_k4);
            if (a == -2.f) a = 0.f; /* COMBINED is 0 in the only cycle */
            if (b == -2.f) b = 0.f;
            if (cc == -2.f) cc = 0.f;
            if (d == -2.f) d = 0.f;
        }
        if (a < 0.f || b < 0.f || cc < 0.f || d < 0.f) return 0;
        rgb[ch] = (a - b) * cc + d;
        rgb[ch] = rgb[ch] < 0.f ? 0.f : rgb[ch] > 1.f ? 1.f : rgb[ch];
    }
    if (t[4] == 2 && c->t1_valid) return 0; /* VIS3: (T1 - T0) * E + T0 reads tile 1 when it is loaded */
    k = dc_alpha_k(t[4], t[5], t[6], t[7], pr, en);
    if (k < 0.f) return 0;
    if (two) {
        if (t[12] == 0 && t[13] == 7 && t[15] == 7 && (t[14] == 3 || t[14] == 5)) k2 = t[14] == 3 ? pr[3] : en[3];
        else if (t[12] == 7 && t[13] == 7 && t[14] == 7 && t[15] == 0) k2 = 1.f;
        else return 0;
        k *= k2;
    }
    *out = ((unsigned)(rgb[0] * 255.f + .5f) << 24) | ((unsigned)(rgb[1] * 255.f + .5f) << 16) |
           ((unsigned)(rgb[2] * 255.f + .5f) << 8) | (unsigned)(k * 255.f + .5f);
    return 1;
}

/* The 16 combiner fields of cw0, cw1 as op_setcombine decodes them (tile 1 renamed to tile 0 when use_t1), and the folded box product (box_fold_sync). */
void nd_comb_tst(uint32_t w0, uint32_t w1, int use_t1, int box_folded, uint32_t t[16]) {
    t[0] = (w0 >> 20) & 0xf;  t[1] = (w1 >> 28) & 0xf;  t[2] = (w0 >> 15) & 0x1f; t[3] = (w1 >> 15) & 7;
    t[4] = (w0 >> 12) & 7;    t[5] = (w1 >> 12) & 7;    t[6] = (w0 >> 9) & 7;      t[7] = (w1 >> 9) & 7;
    t[8] = (w0 >> 5) & 0xf;   t[9] = (w1 >> 24) & 0xf;  t[10] = w0 & 0x1f;         t[11] = (w1 >> 6) & 7;
    t[12] = (w1 >> 21) & 7;   t[13] = (w1 >> 3) & 7;    t[14] = (w1 >> 18) & 7;    t[15] = w1 & 7;
    if (use_t1) {
        if (t[0] == 2) t[0] = 1;
        if (t[1] == 2) t[1] = 1;
        if (t[3] == 2) t[3] = 1;
        if (t[2] == 2) t[2] = 1;
        if (t[2] == 9) t[2] = 8;
        if (t[4] == 2) t[4] = 1;
        if (t[5] == 2) t[5] = 1;
        if (t[6] == 2) t[6] = 1;
        if (t[7] == 2) t[7] = 1;
    }
    if (box_folded) {
        t[0] = G_CCMUX_PRIMITIVE; t[1] = G_CCMUX_ENVIRONMENT; t[2] = G_CCMUX_TEXEL0; t[3] = G_CCMUX_ENVIRONMENT;
        t[4] = 7; t[5] = 7; t[6] = 7; t[7] = 1; /* TEXEL1 alpha, renamed TEXEL0 */
    }
}

/* NPSO2D: the walker inputs w equal the description d in every input nd_comb_core reads except prim, env (leg 4 colour only), leg_mode (written
 * first) and box_pat (leg 4 and dual only). Equal inputs give the table's decision (nd_pso_fix adds the rest). */
/* The one-bit inputs packed in one byte (NdPsoKey.flags). Callers pass 0 or 1 values. */
uint8_t nd_pso_flags(int tex_on, int use_t1, int t1_valid, int dual_on, int box_prod, int box_folded, int comb_set, int comb_alpha_shade) {
    return (uint8_t)((tex_on ? 1u : 0u) | (use_t1 ? 2u : 0u) | (t1_valid ? 4u : 0u) | (dual_on ? 8u : 0u) | (box_prod ? 16u : 0u) |
                     (box_folded ? 32u : 0u) | (comb_set ? 64u : 0u) | (comb_alpha_shade ? 128u : 0u));
}

/* NPSO2D: the walker inputs w equal the key d in every input nd_comb_core reads except prim, env (leg 4 colour only), leg_mode (written first)
 * and box_pat (leg 4 and dual only). Equal inputs give the table's decision (nd_pso_fix adds the rest). A one-bit input above 1 is no match. */
int nd_pso_in_eq(const NdPsoKey* d, const NdCombIn* w, unsigned vprim) {
    uint32_t t[16];
    if ((unsigned)(w->tex_on | w->use_t1 | w->t1_valid | w->dual_on | w->box_prod | w->box_folded | w->comb_set | w->comb_alpha_shade) > 1u) return 0;
    if (w->w0 != d->cw0 || w->w1 != d->cw1 || w->omode_h != d->omh || (uint8_t)w->prim_lo8 != d->plod || w->key_c != d->key_c || w->key_s != d->key_s ||
        w->conv_k4 != d->conv_k4 || vprim != (unsigned)d->vprim)
        return 0;
    if (nd_pso_flags(w->tex_on, w->use_t1, w->t1_valid, w->dual_on, w->box_prod, w->box_folded, w->comb_set, w->comb_alpha_shade) != d->flags) return 0;
    nd_comb_tst(d->cw0, d->cw1, (d->flags >> 1) & 1, (d->flags >> 5) & 1, t);
    return memcmp(t, w->tst, sizeof t) == 0;
}

/* NPSO2D: a table decision under the walker's colours (leg 4 cc), box_pat (leg 4 pattern, dual) and mode5_prev (leg 4). Same code paths as nd_comb_core. */
void nd_pso_fix(NdCombOut* o, const NdCombIn* w, int mode5_prev) {
    if (o->leg == 4) { o->mode5 = mode5_prev; o->pat = w->box_pat; }
    if (o->cc_on) { unsigned v = 0; nd_const_rgb(w, &v); o->cc = v; }
    o->dual = w->dual_on | w->box_prod << 1 | o->pat << 2;
}
