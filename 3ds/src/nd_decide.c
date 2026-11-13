/* nd_decide.c: NPSO2, the pure pipeline decision. No walker global is read or written: every input is an NdCombIn or an NdPsoDesc field.
 * nd_comb_core is the combine work of nd_comb.c (sync_legacy, decide, cc_eval, cc_legacy, const_rgb_tex_alpha) on explicit inputs. The device
 * (nd_comb.c: sync_combine_work, nd_comb_decide) calls it, so the decision is one piece of code. The host links it (3ds/tests/host.d/decide.sh).
 * Colours are draw values: the plan this file makes keeps the colour names and has constants 0 (nd_pso_colour makes the values).
 *
 * Globals the moved code read before, and where each comes from now (NdCombIn field, NdPsoDesc field):
 *   nd_ws.comb_w0, comb_w1 -> in->w0, w1 (cw0, cw1)        nd_ws.omode_h -> in->omode_h (omh)     nd_ws.prim, envc -> in->prim, env (draw values)
 *   nd_ws.prim_lod (low byte) -> in->prim_lo8 (plod)       keyc, keys -> in->key_c, key_s         conv_k4 -> in->conv_k4
 *   nd_ws.tst1.comb_a..comb2_ad (tst_f) -> in->tst[16] (decoded from cw0, cw1 by nd_comb_tst)
 *   leg_mode -> in->leg_mode (leg_prev)     nd_ws.tex_on -> tex_on     nd_ws.use_t1 -> use_t1     nd_ws.td[1].valid -> t1_valid   dual_on -> dual_on
 *   box_prod, box_folded, comb_set, comb_alpha_shade -> the fields of the same name     box_pat -> in->box_pat (desc: 0, no field)
 *   pm_comb_vprim -> the vprim argument (desc vprim)     comb_mode5 (kept when leg 4) -> the mode5_prev argument (desc: 0)
 * pm_comb.c module state kept: pm_k4 (pm_comb_set_k4, nd_pso_build sets it from conv_k4). */
#include <stdint.h>
#include <string.h>
#include <math.h>
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
    const unsigned* t = c->tst;
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

/* ---- the legacy combine mode (sync_legacy) on explicit state. c->leg_mode and c->box_pat are updated. o: mode5, cc_on, cc, env_on. ---- */
static void dc_legacy(NdCombIn* c, int mode5_prev, NdCombOut* o) {
    unsigned cc;
    const unsigned* t = c->tst;
    o->cc_on = o->env_on = 0;
    o->mode5 = mode5_prev; /* kept when the constant mode returns first */
    if (nd_const_rgb(c, &cc)) {
        o->cc_on = 1; o->cc = cc;
        c->leg_mode = 4;
        return;
    }
    /* Decal modes put TEXEL0 in the addend, which is mux d, not a/b/c. */
    int wants_tex = (t[0] == G_CCMUX_TEXEL0 || t[1] == G_CCMUX_TEXEL0 || t[2] == G_CCMUX_TEXEL0 || t[3] == G_CCMUX_TEXEL0 || t[2] == G_CCMUX_TEXEL0_ALPHA);
    int wants_prim = t[0] == G_CCMUX_PRIMITIVE || t[1] == G_CCMUX_PRIMITIVE || t[2] == G_CCMUX_PRIMITIVE;
    int mode = 0;
    int alpha_tex = t[4] == 1 || t[5] == 1 || t[6] == 1 || t[7] == 1 || t[4] == 2 || t[5] == 2 || t[6] == 2 || t[7] == 2;
    o->mode5 = 0;
    c->box_pat = c->box_prod && !c->box_folded; /* V10: the 2-cycle window with a pattern in tile 0 (not white): product on the CPU */
    if (wants_tex && t[0] == G_CCMUX_PRIMITIVE && t[1] == G_CCMUX_ENVIRONMENT && t[2] == G_CCMUX_TEXEL0 && t[3] == G_CCMUX_ENVIRONMENT && alpha_tex && t[12] != 0) {
        o->mode5 = 1;
        o->env_on = 1;
        {
            int a_t0 = t[4] == 1 || t[5] == 1 || t[6] == 1 || t[7] == 1;
            c->box_pat = !a_t0;
            c->leg_mode = a_t0 ? 5 : 7;
        }
        return;
    }
    if (t[0] == G_CCMUX_PRIMITIVE && t[1] == G_CCMUX_TEXEL0 && t[2] == G_CCMUX_PRIMITIVE_ALPHA && t[3] == G_CCMUX_TEXEL0) {
        c->leg_mode = 6;
        return;
    }
    if (wants_tex) mode = alpha_tex ? 2 : 3;
    else if (wants_prim && !c->comb_set) mode = 1;
    else if (wants_prim && t[0] != G_CCMUX_SHADE) mode = 1;
    c->leg_mode = mode;
}

/* ---- the audit of the decision (cc_legacy, cc_v, cc_decode, cc_rdp, cc_eval) ---- */
typedef struct { float t0[4], t1[4], sh[4], pr[4], en[4], ke[4], ks[4]; float k4; } DcIn;
typedef struct { unsigned key; int use, why; float el, ep; PmPlan plan; } Dec;

static float dc_clamp(float v) { return v < 0.f ? 0.f : v > 1.f ? 1.f : v; }

static void dc_legacy_col(int mode, const DcIn* in, int t1src, float out[4], const NdCombIn* c) {
    const float* t = t1src ? in->t1 : in->t0;
    float sa = c->comb_alpha_shade ? in->sh[3] : 1.f;
    unsigned cc = 0;
    switch (mode) {
    case 1: memcpy(out, in->pr, 4 * sizeof(float)); return;
    case 2: for (int i = 0; i < 3; i++) out[i] = t[i] * in->sh[i]; out[3] = t[3] * sa; return;
    case 3: for (int i = 0; i < 3; i++) out[i] = t[i] * in->sh[i]; out[3] = sa; return;
    case 4:
        if (nd_const_rgb(c, &cc)) {
            float cf[4];
            unpack8(cc, cf);
            out[0] = cf[0]; out[1] = cf[1]; out[2] = cf[2]; out[3] = t[3] * cf[3];
            return;
        }
        break;
    case 5: case 7:
        for (int i = 0; i < 3; i++) out[i] = (in->pr[i] - in->en[i]) * t[i] + in->en[i];
        out[3] = mode == 7 ? 1.f : t[3];
        return;
    case 6:
        for (int i = 0; i < 3; i++) out[i] = dc_clamp((t[i] - in->pr[i]) * in->pr[3] + in->pr[i]);
        out[3] = t[3];
        return;
    default: break;
    }
    memcpy(out, in->sh, 4 * sizeof(float));
}

static float dc_v(int slot, unsigned sel, int ch, const DcIn* in, const float* cb, int alpha, float prlod) {
    int rgb_c = !alpha && slot == 2;
    if (alpha) {
        switch (sel) {
        case 0: return slot == 2 ? 0.f : cb[3];
        case 1: return in->t0[3];
        case 2: return in->t1[3];
        case 3: return in->pr[3];
        case 4: return in->sh[3];
        case 5: return in->en[3];
        case 6: return slot == 2 ? prlod : 1.f;
        default: return 0.f;
        }
    }
    switch (sel) {
    case 0: return cb[ch];
    case 1: return in->t0[ch];
    case 2: return in->t1[ch];
    case 3: return in->pr[ch];
    case 4: return in->sh[ch];
    case 5: return in->en[ch];
    case 6: return slot == 1 ? in->ke[ch] : slot == 2 ? in->ks[ch] : slot == 0 || slot == 3 ? 1.f : 0.f;
    case 7: return slot == 1 ? in->k4 : slot == 0 ? 0.5f /* PT07: NOISE, the audit uses the mean */ : rgb_c ? cb[3] : 0.f;
    case 8: return rgb_c ? in->t0[3] : 0.f;
    case 9: return rgb_c ? in->t1[3] : 0.f;
    case 10: return rgb_c ? in->pr[3] : 0.f;
    case 11: return rgb_c ? in->sh[3] : 0.f;
    case 12: return rgb_c ? in->en[3] : 0.f;
    case 14: return rgb_c ? prlod : 0.f;
    default: return 0.f;
    }
}

static void dc_decode(unsigned w0, unsigned w1, unsigned s[2][8]) {
    s[0][0] = (w0 >> 20) & 0xf; s[0][1] = (w1 >> 28) & 0xf; s[0][2] = (w0 >> 15) & 0x1f; s[0][3] = (w1 >> 15) & 7;
    s[0][4] = (w0 >> 12) & 7;   s[0][5] = (w1 >> 12) & 7;   s[0][6] = (w0 >> 9) & 7;      s[0][7] = (w1 >> 9) & 7;
    s[1][0] = (w0 >> 5) & 0xf;  s[1][1] = (w1 >> 24) & 0xf; s[1][2] = w0 & 0x1f;          s[1][3] = (w1 >> 6) & 7;
    s[1][4] = (w1 >> 21) & 7;   s[1][5] = (w1 >> 3) & 7;    s[1][6] = (w1 >> 18) & 7;     s[1][7] = w1 & 7;
    pm_comb_swap_c2(s[1]);
}

/* (A-B)*C+D. RDP: the result is a 9 bit value, 129..255 below zero wraps to 255 and 1..128 below zero gives 0 (0 - B*C only, as pm_comb.c lower). */
static float dc_rdp(float a, float b, float c, float d) {
    if (a == 0.f && d == 0.f && b * c > 0.f) return floorf(b * c * 255.f + .5f) >= 129.f ? 1.f : 0.f;
    return dc_clamp((a - b) * c + d);
}

static void dc_eval(const unsigned s[2][8], int two, const DcIn* in, float prlod, float out[4]) {
    float cb[4] = { 0, 0, 0, 0 };
    for (int cyc = 0; cyc < (two ? 2 : 1); cyc++) {
        float r[4];
        for (int ch = 0; ch < 3; ch++) {
            float a = dc_v(0, s[cyc][0], ch, in, cb, 0, prlod), b = dc_v(1, s[cyc][1], ch, in, cb, 0, prlod);
            float c = dc_v(2, s[cyc][2], ch, in, cb, 0, prlod), d = dc_v(3, s[cyc][3], ch, in, cb, 0, prlod);
            r[ch] = dc_rdp(a, b, c, d);
        }
        {
            float a = dc_v(0, s[cyc][4], 3, in, cb, 1, prlod), b = dc_v(1, s[cyc][5], 3, in, cb, 1, prlod);
            float c = dc_v(2, s[cyc][6], 3, in, cb, 1, prlod), d = dc_v(3, s[cyc][7], 3, in, cb, 1, prlod);
            r[3] = dc_rdp(a, b, c, d);
        }
        memcpy(cb, r, sizeof cb);
    }
    memcpy(out, cb, sizeof cb);
}

/* the 24 random inputs of the decision: a fixed seed per decision, so the choice is repeatable */
static unsigned lcg_s = 12345u;
static float lcg_f(void) { lcg_s = lcg_s * 1664525u + 1013904223u; return (float)((lcg_s >> 8) & 0xffff) / 65535.f; }

static Dec dec[128];
static int dec_n, dec_next; /* a full table replaces the oldest entry (a decision is recomputed in microseconds) */

static Dec* decide_miss(const NdCombIn* c, int two, unsigned key) {
    Dec* d;
    if (dec_n < 128) d = &dec[dec_n++];
    else { d = &dec[dec_next]; dec_next = (dec_next + 1) & 127; }
    unsigned s[2][8];
    float plod_f = (float)(uint8_t)c->prim_lo8 / 255.f;
    memset(d, 0, sizeof *d);
    d->key = key;
    dc_decode(c->w0, c->w1, s);
    if (c->leg_mode == 4) { d->why = -2; return d; } /* mode 4 is exact by construction (nd_const_rgb) */
    d->why = pm_comb_build(c->w0, c->w1, two, c->use_t1, &d->plan);
    if (!d->why && d->plan.uses_tex1 && !c->t1_valid) d->why = 6; /* TEXEL1 read, tile 1 not loaded */
    {
        float el = 0.f, ep = 0.f;
        lcg_s = 777u;
        for (int k = 0; k < 24; k++) {
            DcIn in;
            float f[4], g[4], h[4];
            unsigned pc = 0, ec = 0, pcs[2];
            for (int ch = 0; ch < 4; ch++) {
                in.t0[ch] = lcg_f(); in.t1[ch] = lcg_f(); in.sh[ch] = lcg_f();
                pcs[0] = (unsigned)(lcg_f() * 255.f + .5f); pcs[1] = (unsigned)(lcg_f() * 255.f + .5f);
                pc = pc << 8 | pcs[0]; ec = ec << 8 | pcs[1];
            }
            unpack8(pc, in.pr); unpack8(ec, in.en); unpack8(c->key_c, in.ke); unpack8(c->key_s, in.ks); in.k4 = (float)c->conv_k4 / 255.f;
            dc_eval(s, two, &in, plod_f, f);
            dc_legacy_col(c->leg_mode, &in, c->use_t1, g, c);
            for (int ch = 0; ch < 4; ch++) { float e = f[ch] > g[ch] ? f[ch] - g[ch] : g[ch] - f[ch]; if (e > el) el = e; }
            if (!d->why) {
                PmPlan t = d->plan;
                pm_comb_colours(&t, pc, ec, (uint8_t)c->prim_lo8, c->key_c, c->key_s);
                pm_comb_eval(&t, c->use_t1 ? in.t1 : in.t0, in.t1, in.sh, h);
                for (int ch = 0; ch < 4; ch++) { float e = f[ch] > h[ch] ? f[ch] - h[ch] : h[ch] - f[ch]; if (e > ep) ep = e; }
            }
        }
        d->el = el;
        d->ep = d->why ? 9.f : ep;
        d->use = !d->why && el > 0.012f && ep < el;
    }
    return d;
}

static Dec* decide(const NdCombIn* c, int two) {
    unsigned key = 2166136261u, i;
    unsigned parts[] = { c->w0, c->w1, (unsigned)two, (unsigned)c->leg_mode, (unsigned)(c->tex_on | c->use_t1 << 1 | c->t1_valid << 2), (unsigned)c->comb_alpha_shade,
                         (unsigned)(uint8_t)c->prim_lo8, c->key_c, c->key_s, c->conv_k4, (unsigned)pm_comb_vprim }; /* pm_comb_colours and the K4 input read these too */
    for (i = 0; i < sizeof parts / sizeof parts[0]; i++) key = (key ^ parts[i]) * 16777619u;
    {
        static unsigned last_i;
        if (last_i < (unsigned)dec_n && dec[last_i].key == key) return &dec[last_i];
        for (i = 0; i < (unsigned)dec_n; i++)
            if (dec[i].key == key) { last_i = i; return &dec[i]; }
    }
    return decide_miss(c, two, key); /* HCCUT4: the miss part (a new combine key) is cold */
}

/* The combine decision (the dry sync_combine_work) on the inputs in. The plan in out has the colour names and constants as pm_comb_build makes them
 * (not coloured). Fields the work leaves as they were are 0 here, or the previous value for mode5 (mode5_prev, kept when the constant mode 4 is chosen). */
void nd_comb_core(const NdCombIn* in, unsigned vprim, int mode5_prev, NdCombOut* out) {
    NdCombIn c = *in;
    const int svp = pm_comb_vprim;
    const unsigned cyc = (c.omode_h >> G_MDSFT_CYCLETYPE) & 3;
    pm_comb_vprim = (int)vprim;
    memset(out, 0, sizeof *out);
    dc_legacy(&c, mode5_prev, out);
    out->use = -1; out->cyc = (int)cyc; out->dual = c.dual_on | c.box_prod << 1 | c.box_pat << 2;
    if (cyc <= 1 && c.comb_set && !c.dual_on && (!c.box_prod || !c.box_folded)) { /* VIS1: a draw_box window with a pattern in tile 0 reads TEXEL0 * TEXEL1 on the GPU plan too */
        Dec* d = decide(&c, (int)cyc);
        out->use = d->use; out->why = d->why;
        if (d->use) { out->plan = d->plan; out->plan_on = 1; }
    }
    out->leg = c.leg_mode; out->pat = c.box_pat;
    pm_comb_vprim = svp;
}

/* ---- the description form ---- */
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

static void pso_in(const NdPsoDesc* d, uint32_t prim, uint32_t env, NdCombIn* in) {
    memset(in, 0, sizeof *in);
    in->w0 = d->cw0; in->w1 = d->cw1; in->omode_h = d->omh; in->prim = prim; in->env = env; in->prim_lo8 = d->plod;
    in->key_c = d->key_c; in->key_s = d->key_s; in->conv_k4 = d->conv_k4;
    nd_comb_tst(d->cw0, d->cw1, d->use_t1, d->box_folded, in->tst);
    in->leg_mode = d->leg_prev; in->tex_on = d->tex_on; in->use_t1 = d->use_t1; in->t1_valid = d->t1_valid; in->dual_on = d->dual_on;
    in->box_prod = d->box_prod; in->box_pat = 0; in->box_folded = d->box_folded; in->comb_set = d->comb_set; in->comb_alpha_shade = d->comb_alpha_shade;
}

/* The decision of a description. Colours are draw values: the plan has no constants, the TEV words have no colour (nd_pso_colour). The TEV key is the one
 * nd_model_state_in makes for a draw with no unit, no tile 1 raster flag, no fog or mix state (i8, rgb_scale, r_mix, draw_3d, rect_2d, dview, off_*: 0). */
void nd_pso_build(const NdPsoDesc* d, NdPsoOut* o) {
    NdCombIn in;
    NdTmplKey k;
    memset(o, 0, sizeof *o);
    pso_in(d, 0, 0, &in);
    pm_comb_set_k4(d->conv_k4);
    nd_comb_core(&in, d->vprim, 0, &o->comb);
    nd_render_state(d->omh, d->oml, d->zimg, &o->rnd);
    {
        const int combine = o->comb.plan_on ? 8 : o->comb.leg;
        const int tex = d->tex_on && combine >= 2;
        const int nz = combine == 8 && o->comb.plan.uses_noise;
        const int t1n = combine == 8 && o->comb.plan.uses_tex1;
        NdTevIn ti;
        memset(&k, 0, sizeof k);
        k.combine = (uint8_t)combine; k.tex = (uint8_t)tex; k.tex1 = (uint8_t)t1n; k.noise = (uint8_t)nz;
        ti.k = &k; ti.plan = combine == 8 ? &o->comb.plan : NULL; ti.prim = 0; ti.cconst = 0; ti.mixcol = 0;
        nd_tev_build(&ti, &o->tev);
    }
}

/* The constant colour of the description under prim and env (mode 4): 1 and *cc, or 0. */
int nd_pso_cc(const NdPsoDesc* d, uint32_t prim, uint32_t env, uint32_t* cc) {
    NdCombIn in;
    unsigned v;
    pso_in(d, prim, env, &in);
    in.leg_mode = 4;
    if (!nd_const_rgb(&in, &v)) return 0;
    *cc = v;
    return 1;
}

/* The colour words of a draw: the stage colours (tev_col, the word 3 of each TEV stage), the buffer colour and the vertex colour of the plan. cconst: the
 * constant colour of leg 4 (nd_pso_cc). A stage the template left without a colour keeps its template word. */
void nd_pso_colour(const NdPsoOut* o, uint32_t prim, uint32_t env, uint32_t plod, uint32_t key, uint32_t keys, uint32_t cconst,
                   uint32_t tev_col[6], uint32_t* buf_col, uint32_t* vtx) {
    for (int i = 0; i < 6; i++) tev_col[i] = o->tev.st[i][3];
    *buf_col = o->tev.buf_set ? o->tev.buf_color : 0;
    *vtx = 0;
    if (o->comb.plan_on) {
        PmPlan p = o->comb.plan;
        pm_comb_colours(&p, prim, env, plod, key, keys);
        *vtx = p.vtx;
        if (o->tev.buf_set) {
            *buf_col = nd_abgr(p.buf);
            for (int i = 0; i < p.n && i < 6; i++) tev_col[i] = nd_abgr(p.st[i].constant);
        }
    } else if (o->tev.st[0][3] == 0) { /* the template set a colour source (nd_tev_build modes 1, 4, 5, 6, 7) */
        const int leg = o->comb.leg;
        tev_col[0] = nd_abgr(leg == 4 ? cconst : leg == 7 ? (prim | 0xffu) : prim);
    }
}
