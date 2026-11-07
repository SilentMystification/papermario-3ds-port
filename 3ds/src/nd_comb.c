/* nd_comb.c: the combine group (plan_s10_path group C, ST-2, cut from pm_gbi.c, pure cut). 3DS only: the decision and the
 * sync_combine work call pm_gpu_*. The typedefs CcIn, Dec and ScMemo are copies of the pm_gbi.c ones (ST-2). */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "pm_gbi.h"
#include "nd_int.h"
#include "pm_gpu.h"
#include "nd.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"
#include "nd_ops.h"


/* globals of the combine state, owned by pm_gbi.c until their group moves (D10 debt, listed in the ST-2 commit) */
extern unsigned aux_key, box_save[4], lcg_s, sl_cc;
extern int box_pat, box_prod, comb_alpha_shade, comb_dry, comb_mode5, dual_on, leg_mode, plan_active, sl_const, sl_env;
extern int ls_use, ls_why, ls_cyc, ls_dual, ls_frame, suntile_lines;
extern PmPlan cur_plan;
extern const char* const cc_nm_a[8], * const cc_nm_aa[8], * const cc_nm_ac[8];
extern unsigned* const tst_f[16];
extern float vp_sx, vp_sy, vp_tx, vp_ty;
int suntile_on(void); int ph_pick(void); void grab_cur(const void* src, int w, int h); void ens_clear(void);
float lcg_f(void); void unpack8(unsigned c, float o[4]);
static void sync_combine_work(void);

#define PMD(word) ({ static int v_ = -1; if (v_ < 0) v_ = pm_debug_has(word); v_; })
#define GP_SET(k, same) do { if (gp_on()) gp_note(k, (same)); } while (0)
#define pm_gpu_flush() pm_gpu_flush_at(__LINE__) /* PG0: the flush counter by cause (prof mode) */

typedef struct { float t0[4], t1[4], sh[4], pr[4], en[4], ke[4], ks[4]; float k4; } CcIn;
typedef struct { unsigned key; int use, why; float el, ep; PmPlan plan; } Dec;
typedef struct { unsigned sg; int ok, leg, mode5, pat, use, why, cyc, dual, plan_on, cc_on, env_on; unsigned cc; PmPlan plan; } ScMemo;
void set_leg(int m);
void sync_legacy(void);
int combchk_on(void);
void combchk_run(const NdCombIn* in, int worked);
void cc07_dbg(void);
void cc_legacy(int mode, const CcIn* in, int t1src, float out[4]);
Dec dec[128];
int dec_n, dec_next; /* a full table replaces the oldest entry (a decision is recomputed in microseconds) */
ScMemo scm[16];
unsigned scm_next;
extern const char* const cc_nm_b[8], * const cc_nm_c[16], * const cc_nm_d[8];


/* Combiner input as a constant, or -1 when it needs a texel or the shade, -2 for COMBINED.
 * slot 0..3 is A, B, C, D of the colour equation. */
float cc_const_in(int slot, unsigned sel, int ch, const float* pr, const float* en) {
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

/* Alpha of one cycle as K * TEXEL0 alpha. Returns K, or -1 if the form is not that.
 * (T1 - T0) * E + T0 reads two textures. Tile 1 is not loaded, so TEXEL0 stands for both. */
float cc_alpha_k(unsigned aa, unsigned ab, unsigned ac, unsigned ad, const float* pr, const float* en) {
    if (ab == 7 && ad == 7 && aa == 1 && (ac == 3 || ac == 5)) return ac == 3 ? pr[3] : en[3];
    if (ab == 7 && ad == 7 && ac == 1 && (aa == 3 || aa == 5)) return aa == 3 ? pr[3] : en[3];
    if (aa == 2 && ab == 1 && ad == 1 && (ac == 3 || ac == 5)) return 1.f;
    if (aa == 3 && ab == 0 && ac == 1 && ad == 0) return pr[3];
    return -1.f;
}

/* A combiner whose colour needs no texel and no shade, and whose alpha is a constant times
 * TEXEL0 alpha: a flat colour through a texture's alpha mask (a drop shadow is black this way).
 * Returns 1 and the RGBA constant, the alpha byte being the factor on texture alpha. */
int const_rgb_tex_alpha(unsigned* out) {
    float pr[4], en[4], rgb[3], k, k2;
    int ch;
    int two = ((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3) == 1;
    if (((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3) > 1 || !comb_set || !nd_ws.tex_on) return 0;
    unpack8(nd_ws.prim, pr);
    unpack8(nd_ws.envc, en);
    for (ch = 0; ch < 3; ch++) {
        float a, b, c, d;
        if (two) {
            a = cc_const_in(0, nd_ws.tst1.comb2_a, ch, pr, en); b = cc_const_in(1, nd_ws.tst1.comb2_b, ch, pr, en);
            c = cc_const_in(2, nd_ws.tst1.comb2_c, ch, pr, en); d = cc_const_in(3, nd_ws.tst1.comb2_d, ch, pr, en);
        } else {
            a = cc_const_in(0, nd_ws.tst1.comb_a, ch, pr, en); b = cc_const_in(1, nd_ws.tst1.comb_b, ch, pr, en);
            c = cc_const_in(2, nd_ws.tst1.comb_c, ch, pr, en); d = cc_const_in(3, nd_ws.tst1.comb_d, ch, pr, en);
            if (a == -2.f) a = 0.f; /* COMBINED is 0 in the only cycle */
            if (b == -2.f) b = 0.f;
            if (c == -2.f) c = 0.f;
            if (d == -2.f) d = 0.f;
        }
        if (a < 0.f || b < 0.f || c < 0.f || d < 0.f) return 0;
        rgb[ch] = (a - b) * c + d;
        rgb[ch] = rgb[ch] < 0.f ? 0.f : rgb[ch] > 1.f ? 1.f : rgb[ch];
    }
    if (nd_ws.tst1.comb_aa == 2 && nd_ws.td[1].valid) return 0; /* VIS3: (T1 - T0) * E + T0 reads tile 1 when it is loaded */
    k = cc_alpha_k(nd_ws.tst1.comb_aa, nd_ws.tst1.comb_ab, nd_ws.tst1.comb_ac, nd_ws.tst1.comb_ad, pr, en);
    if (k < 0.f) return 0;
    if (two) {
        /* Second cycle alpha: COMBINED * constant, or COMBINED passed through. */
        if (nd_ws.tst1.comb2_aa == 0 && nd_ws.tst1.comb2_ab == 7 && nd_ws.tst1.comb2_ad == 7 && (nd_ws.tst1.comb2_ac == 3 || nd_ws.tst1.comb2_ac == 5)) k2 = nd_ws.tst1.comb2_ac == 3 ? pr[3] : en[3];
        else if (nd_ws.tst1.comb2_aa == 7 && nd_ws.tst1.comb2_ab == 7 && nd_ws.tst1.comb2_ac == 7 && nd_ws.tst1.comb2_ad == 0) k2 = 1.f;
        else return 0;
        k *= k2;
    }
    *out = ((unsigned)(rgb[0] * 255.f + .5f) << 24) | ((unsigned)(rgb[1] * 255.f + .5f) << 16) |
           ((unsigned)(rgb[2] * 255.f + .5f) << 8) | (unsigned)(k * 255.f + .5f);
    return 1;
}

float cc_v(int slot, unsigned sel, int ch, const CcIn* in, const float* cb, int alpha, float prlod) {
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

/* s[cycle][0..7] = A B C D (colour), Aa Ab Ac Ad (alpha) */
void cc_decode(unsigned w0, unsigned w1, unsigned s[2][8]) {
    s[0][0] = (w0 >> 20) & 0xf; s[0][1] = (w1 >> 28) & 0xf; s[0][2] = (w0 >> 15) & 0x1f; s[0][3] = (w1 >> 15) & 7;
    s[0][4] = (w0 >> 12) & 7;   s[0][5] = (w1 >> 12) & 7;   s[0][6] = (w0 >> 9) & 7;      s[0][7] = (w1 >> 9) & 7;
    s[1][0] = (w0 >> 5) & 0xf;  s[1][1] = (w1 >> 24) & 0xf; s[1][2] = w0 & 0x1f;          s[1][3] = (w1 >> 6) & 7;
    s[1][4] = (w1 >> 21) & 7;   s[1][5] = (w1 >> 3) & 7;    s[1][6] = (w1 >> 18) & 7;     s[1][7] = w1 & 7;
    pm_comb_swap_c2(s[1]);
}

float cc_clamp(float v) { return v < 0.f ? 0.f : v > 1.f ? 1.f : v; }

/* (A-B)*C+D. RDP: the result is a 9 bit value, 129..255 below zero wraps to 255 and 1..128 below zero gives 0. The plan models this for
 * 0 - B*C only (pm_comb.c lower), so the reference applies it only there. Other negative results clamp to 0 as before. */
float cc_rdp(float a, float b, float c, float d) {
    if (a == 0.f && d == 0.f && b * c > 0.f) return floorf(b * c * 255.f + .5f) >= 129.f ? 1.f : 0.f;
    return cc_clamp((a - b) * c + d);
}

/* The RDP result, one or two cycles. */
void cc_eval(const unsigned s[2][8], int two, const CcIn* in, float prlod, float out[4]) {
    float cb[4] = { 0, 0, 0, 0 };
    for (int cyc = 0; cyc < (two ? 2 : 1); cyc++) {
        float r[4];
        for (int ch = 0; ch < 3; ch++) {
            float a = cc_v(0, s[cyc][0], ch, in, cb, 0, prlod), b = cc_v(1, s[cyc][1], ch, in, cb, 0, prlod);
            float c = cc_v(2, s[cyc][2], ch, in, cb, 0, prlod), d = cc_v(3, s[cyc][3], ch, in, cb, 0, prlod);
            r[ch] = cc_rdp(a, b, c, d);
        }
        {
            float a = cc_v(0, s[cyc][4], 3, in, cb, 1, prlod), b = cc_v(1, s[cyc][5], 3, in, cb, 1, prlod);
            float c = cc_v(2, s[cyc][6], 3, in, cb, 1, prlod), d = cc_v(3, s[cyc][7], 3, in, cb, 1, prlod);
            r[3] = cc_rdp(a, b, c, d);
        }
        memcpy(cb, r, sizeof cb);
    }
    memcpy(out, cb, sizeof cb);
}

void cc_text(const unsigned s[2][8], int cyc, char* o, int n) {
    snprintf(o, n, "(%s-%s)*%s+%s / (%s-%s)*%s+%s",
             s[cyc][0] < 8 ? cc_nm_a[s[cyc][0]] : "0", s[cyc][1] < 8 ? cc_nm_b[s[cyc][1]] : "0",
             s[cyc][2] < 16 ? cc_nm_c[s[cyc][2]] : "0", cc_nm_d[s[cyc][3]],
             cc_nm_aa[s[cyc][4]], cc_nm_aa[s[cyc][5]], cc_nm_ac[s[cyc][6]], cc_nm_aa[s[cyc][7]]);
}

/* 1 if the combiner reads texel tile t (0 or 1). */
int cc_uses(const unsigned s[2][8], int two, int t) {
    int u = 0;
    for (int cyc = 0; cyc < (two ? 2 : 1); cyc++) {
        for (int i = 0; i < 4; i++) {
            unsigned v = s[cyc][i];
            if (v == (unsigned)t + 1 || (i == 2 && v == (unsigned)t + 8)) u = 1;
        }
        for (int i = 4; i < 8; i++)
            if (s[cyc][i] == (unsigned)t + 1) u = 1;
    }
    return u;
}

float cc_rgb_in(unsigned sel, int ch, const float* t0, const float* t1, const float* pr,
                       const float* en, int a_ok) {
    switch (sel) {
    case 1: return t0[ch];
    case 2: return t1[ch];
    case 3: return pr[ch];
    case 4: return 1.f; /* SHADE: none off screen */
    case 5: return en[ch];
    case 6: return a_ok ? 1.f : 0.f;
    default: return 0.f;
    }
}

/* (A - B) * C + D, cycle 1 only. COMBINED reads as 0 there. */
void cc_cycle1(const float* t0, const float* t1, float out[4]) {
    float pr[4], en[4];
    unpack8(nd_ws.prim, pr);
    unpack8(nd_ws.envc, en);
    for (int ch = 0; ch < 3; ch++) {
        float a = cc_rgb_in(nd_ws.tst1.comb_a, ch, t0, t1, pr, en, 1);
        float b = cc_rgb_in(nd_ws.tst1.comb_b, ch, t0, t1, pr, en, 0);
        float c;
        switch (nd_ws.tst1.comb_c) {
        case 8: c = t0[3]; break;
        case 9: c = t1[3]; break;
        case 10: c = pr[3]; break;
        case 11: c = 1.f; break;
        case 12: c = en[3]; break;
        default: c = cc_rgb_in(nd_ws.tst1.comb_c, ch, t0, t1, pr, en, 0); break;
        }
        float d = cc_rgb_in(nd_ws.tst1.comb_d, ch, t0, t1, pr, en, 1);
        float v = (a - b) * c + d;
        out[ch] = v < 0.f ? 0.f : v > 1.f ? 1.f : v;
    }
    {
        unsigned sel[4] = { nd_ws.tst1.comb_aa, nd_ws.tst1.comb_ab, nd_ws.tst1.comb_ac, nd_ws.tst1.comb_ad };
        float v[4];
        for (int i = 0; i < 4; i++) {
            switch (sel[i]) {
            case 1: v[i] = t0[3]; break;
            case 2: v[i] = t1[3]; break;
            case 3: v[i] = pr[3]; break;
            case 4: v[i] = 1.f; break;
            case 5: v[i] = en[3]; break;
            case 6: v[i] = (i == 2) ? 0.f : 1.f; break;
            default: v[i] = 0.f; break;
            }
        }
        float r = (v[0] - v[1]) * v[2] + v[3];
        out[3] = r < 0.f ? 0.f : r > 1.f ? 1.f : r;
    }
}

Dec* decide(int two) {
    unsigned key = 2166136261u, i;
    unsigned parts[] = { nd_ws.comb_w0, nd_ws.comb_w1, (unsigned)two, (unsigned)leg_mode, (unsigned)(nd_ws.tex_on | nd_ws.use_t1 << 1 | nd_ws.td[1].valid << 2), (unsigned)comb_alpha_shade,
                         (unsigned)(u8)nd_ws.prim_lod, keyc, keys, conv_k4 }; /* S3N-3d: pm_comb_colours and the K4 input read these too */
    for (i = 0; i < sizeof parts / sizeof parts[0]; i++) key = (key ^ parts[i]) * 16777619u;
    {
        static unsigned last_i;
        if (last_i < (unsigned)dec_n && dec[last_i].key == key) return &dec[last_i];
        for (i = 0; i < (unsigned)dec_n; i++)
            if (dec[i].key == key) { last_i = i; return &dec[i]; }
    }
    {
        Dec* d;
        if (dec_n < 128) d = &dec[dec_n++];
        else { d = &dec[dec_next]; dec_next = (dec_next + 1) & 127; }
        unsigned s[2][8];
        float plod_f = (float)(u8)nd_ws.prim_lod / 255.f;
        memset(d, 0, sizeof *d);
        d->key = key;
        cc_decode(nd_ws.comb_w0, nd_ws.comb_w1, s);
        if (leg_mode == 4) { d->why = -2; return d; } /* mode 4 is exact by construction (const_rgb_tex_alpha) */
        d->why = pm_comb_build(nd_ws.comb_w0, nd_ws.comb_w1, two, nd_ws.use_t1, &d->plan);
        if (!d->why && d->plan.uses_tex1 && !nd_ws.td[1].valid) d->why = 6; /* TEXEL1 read, tile 1 not loaded */
        {
            float el = 0.f, ep = 0.f;
            lcg_s = 777u;
            for (int k = 0; k < 24; k++) {
                CcIn in;
                float f[4], g[4], h[4];
                unsigned pc = 0, ec = 0, pcs[2];
                for (int c = 0; c < 4; c++) {
                    in.t0[c] = lcg_f(); in.t1[c] = lcg_f(); in.sh[c] = lcg_f();
                    pcs[0] = (unsigned)(lcg_f() * 255.f + .5f); pcs[1] = (unsigned)(lcg_f() * 255.f + .5f);
                    pc = pc << 8 | pcs[0]; ec = ec << 8 | pcs[1];
                }
                unpack8(pc, in.pr); unpack8(ec, in.en); unpack8(keyc, in.ke); unpack8(keys, in.ks); in.k4 = (float)conv_k4 / 255.f;
                cc_eval(s, two, &in, plod_f, f);
                cc_legacy(leg_mode, &in, nd_ws.use_t1, g);
                for (int c = 0; c < 4; c++) { float e = f[c] > g[c] ? f[c] - g[c] : g[c] - f[c]; if (e > el) el = e; }
                if (!d->why) {
                    PmPlan t = d->plan;
                    pm_comb_colours(&t, pc, ec, (u8)nd_ws.prim_lod, keyc, keys);
                    pm_comb_eval(&t, nd_ws.use_t1 ? in.t1 : in.t0, in.t1, in.sh, h);
                    for (int c = 0; c < 4; c++) { float e = f[c] > h[c] ? f[c] - h[c] : h[c] - f[c]; if (e > ep) ep = e; }
                }
            }
            d->el = el;
            d->ep = d->why ? 9.f : ep;
            d->use = !d->why && el > 0.012f && ep < el;
        }
        return d;
    }
}

unsigned scm_sig(void) {
    unsigned parts[] = { nd_ws.comb_w0, nd_ws.comb_w1, nd_ws.omode_h, nd_ws.omode_l, nd_ws.prim, nd_ws.envc, (unsigned)(u8)nd_ws.prim_lod, nd_ws.tst1.tile_fmt[0] * 4 + nd_ws.tst1.tile_siz[0],
        (unsigned)(nd_ws.tex_on | nd_ws.use_t1 << 1 | dual_on << 2 | box_prod << 3 | box_pat << 4 | comb_set << 5), nd_ws.geom };
    unsigned sg = 2166136261u;
    for (unsigned k = 0; k < sizeof parts / sizeof parts[0]; k++) sg = (sg ^ parts[k]) * 16777619u;
    return sg;
}

ScMemo* scm_find(unsigned sg) {
    for (unsigned i = 0; i < 16; i++) if (scm[i].ok && scm[i].sg == sg) return &scm[i];
    return NULL;
}

void scm_store(unsigned sg) {
    ScMemo* m = &scm[scm_next++ & 15];
    m->sg = sg; m->ok = 1; m->leg = leg_mode; m->mode5 = comb_mode5; m->pat = box_pat; m->use = ls_use; m->why = ls_why; m->cyc = ls_cyc; m->dual = ls_dual;
    m->plan_on = plan_active; m->cc_on = sl_const; m->cc = sl_cc; m->env_on = sl_env;
    if (plan_active) m->plan = cur_plan;
}

void scm_replay(const ScMemo* m) {
    if (m->cc_on) pm_gpu_set_const(m->cc);
    if (m->env_on) pm_gpu_set_env(nd_ws.envc);
    leg_mode = m->leg; comb_mode5 = m->mode5; box_pat = m->pat;
    ls_use = m->use; ls_why = m->why; ls_cyc = m->cyc; ls_dual = m->dual; ls_frame = rm_frame;
    plan_active = m->plan_on;
    if (m->plan_on) { cur_plan = m->plan; pm_gpu_plan_key(nd_ws.comb_w0, nd_ws.comb_w1, ((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3) == 1, nd_ws.use_t1); pm_gpu_set_plan(&cur_plan); }
    else pm_gpu_set_combine(leg_mode);
}

/* R2B: the sync_combine input hash (the fields the skip test reads); sync_combine and the g2_same skip use the same value */
unsigned sc_sig_now(void) {
    unsigned parts[] = { nd_ws.comb_w0, nd_ws.comb_w1, nd_ws.omode_h, nd_ws.prim, nd_ws.envc, (unsigned)(u8)nd_ws.prim_lod, keyc, keys, conv_k4, (unsigned)leg_mode,
                         (unsigned)(nd_ws.tex_on | nd_ws.use_t1 << 1 | nd_ws.td[1].valid << 2 | dual_on << 3 | box_prod << 4 | box_pat << 5 | box_folded << 6 | comb_set << 7 | comb_alpha_shade << 8) };
    unsigned sg = 2166136261u;
    for (unsigned k = 0; k < sizeof parts / sizeof parts[0]; k++) sg = (sg ^ parts[k]) * 16777619u;
    return sg;
}

void comb_in_walker(NdCombIn* in) {
    in->w0 = nd_ws.comb_w0; in->w1 = nd_ws.comb_w1; in->omode_h = nd_ws.omode_h; in->prim = nd_ws.prim; in->env = nd_ws.envc; in->prim_lo8 = (u8)nd_ws.prim_lod;
    in->key_c = keyc; in->key_s = keys; in->conv_k4 = conv_k4;
    for (int i = 0; i < 16; i++) in->tst[i] = *tst_f[i];
    in->leg_mode = leg_mode; in->tex_on = nd_ws.tex_on; in->use_t1 = nd_ws.use_t1; in->t1_valid = nd_ws.td[1].valid; in->dual_on = dual_on;
    in->box_prod = box_prod; in->box_pat = box_pat; in->box_folded = box_folded; in->comb_set = comb_set; in->comb_alpha_shade = comb_alpha_shade;
}

uint32_t gvk_bits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }

unsigned pm_gbi_gvkey(uint32_t* o, unsigned max) {
    uint32_t v[48]; unsigned n = 0;
    v[n++] = nd_ws.envc; v[n++] = (uint32_t)comb_mode5; v[n++] = (uint32_t)plan_active; v[n++] = cur_plan.vtxname; v[n++] = cur_plan.vtx;
    v[n++] = (uint32_t)dual_on; v[n++] = nd_ws.tst1.comb2_a; v[n++] = nd_ws.tst1.comb2_b; v[n++] = nd_ws.tst1.comb2_c; v[n++] = nd_ws.tst1.comb2_d;
    v[n++] = (uint32_t)comb_alpha_shade; v[n++] = (uint32_t)nd_ws.use_t1; v[n++] = (uint32_t)nd_ws.tex_on; v[n++] = (uint32_t)nd_ws.tile_x0; v[n++] = (uint32_t)nd_ws.tile_y0;
    v[n++] = gvk_bits(nd_ws.tile_fx); v[n++] = gvk_bits(nd_ws.tile_fy);
    v[n++] = nd_ws.tst1.tile_shift[0][0]; v[n++] = nd_ws.tst1.tile_shift[0][1]; v[n++] = nd_ws.tst1.tile_shift[1][0]; v[n++] = nd_ws.tst1.tile_shift[1][1];
    v[n++] = gvk_bits(vp_sx); v[n++] = gvk_bits(vp_sy); v[n++] = gvk_bits(vp_tx); v[n++] = gvk_bits(vp_ty);
    v[n++] = nd_ws.omode_l; v[n++] = nd_ws.prim_z; v[n++] = (uint32_t)fog_fm; v[n++] = (uint32_t)fog_fo; v[n++] = (uint32_t)fogmix_on;
    v[n++] = (uint32_t)img_role(0);
    if (n > max) n = max;
    memcpy(o, v, n * sizeof(uint32_t));
    return n;
}

/* S3N-3d: the walker's sticky state at a draw (ndmcheck compare only: the native draw takes these as inputs): the TEXTURE scale word, the geometry mode and the combine words. */
void pm_gbi_sticky(unsigned* tss, unsigned* geo, unsigned* cw0, unsigned* cw1) {
    *tss = ((unsigned)nd_ws.tex_ss << 16) | ((unsigned)nd_ws.tex_ts & 0xffffu); *geo = nd_ws.geom; *cw0 = nd_ws.comb_w0; *cw1 = nd_ws.comb_w1;
}

/* S3N-3d: nd_comb_decide. The combine work (sync_combine_work) on the inputs in. Every global it reads or writes is set from in and put back
 * after the call, so the walker state is the same before and after. comb_dry: no pm_gpu call, no stats, no log. */
void nd_comb_decide(const NdCombIn* in, NdCombOut* out) {
    struct { uint32_t w0, w1, omh, prim, env, kc, ks, ck4; unsigned alod, slcc, tst[16]; int leg, tex, use1, t1v, dual, bprod, bpat, bfold, cset, cas;
             int mode5, pact, use, why, cyc, dualo, lsf, slc, sle; PmPlan plan; } sv;
    int i;
    sv.w0 = nd_ws.comb_w0; sv.w1 = nd_ws.comb_w1; sv.omh = nd_ws.omode_h; sv.prim = nd_ws.prim; sv.env = nd_ws.envc; sv.alod = nd_ws.prim_lod; sv.kc = keyc; sv.ks = keys; sv.ck4 = conv_k4;
    sv.leg = leg_mode; sv.tex = nd_ws.tex_on; sv.use1 = nd_ws.use_t1; sv.t1v = nd_ws.td[1].valid; sv.dual = dual_on; sv.bprod = box_prod; sv.bpat = box_pat; sv.bfold = box_folded;
    sv.cset = comb_set; sv.cas = comb_alpha_shade;
    for (i = 0; i < 16; i++) sv.tst[i] = *tst_f[i];
    sv.mode5 = comb_mode5; sv.pact = plan_active; sv.plan = cur_plan; sv.use = ls_use; sv.why = ls_why; sv.cyc = ls_cyc; sv.dualo = ls_dual; sv.lsf = ls_frame;
    sv.slc = sl_const; sv.sle = sl_env; sv.slcc = sl_cc;
    nd_ws.comb_w0 = in->w0; nd_ws.comb_w1 = in->w1; nd_ws.omode_h = in->omode_h; nd_ws.prim = in->prim; nd_ws.envc = in->env; nd_ws.prim_lod = (nd_ws.prim_lod & 0xff00u) | in->prim_lo8;
    keyc = in->key_c; keys = in->key_s; conv_k4 = in->conv_k4;
    leg_mode = in->leg_mode; nd_ws.tex_on = in->tex_on; nd_ws.use_t1 = in->use_t1; nd_ws.td[1].valid = in->t1_valid; dual_on = in->dual_on;
    box_prod = in->box_prod; box_pat = in->box_pat; box_folded = in->box_folded; comb_set = in->comb_set; comb_alpha_shade = in->comb_alpha_shade;
    for (i = 0; i < 16; i++) *tst_f[i] = in->tst[i];
    comb_dry = 1;
    sync_combine_work();
    comb_dry = 0;
    out->leg = leg_mode; out->mode5 = comb_mode5; out->pat = box_pat; out->use = ls_use; out->why = ls_why; out->cyc = ls_cyc; out->dual = ls_dual;
    out->plan_on = plan_active; out->cc_on = sl_const; out->cc = sl_cc; out->env_on = sl_env; out->plan = cur_plan;
    nd_ws.comb_w0 = sv.w0; nd_ws.comb_w1 = sv.w1; nd_ws.omode_h = sv.omh; nd_ws.prim = sv.prim; nd_ws.envc = sv.env; nd_ws.prim_lod = sv.alod; keyc = sv.kc; keys = sv.ks; conv_k4 = sv.ck4;
    leg_mode = sv.leg; nd_ws.tex_on = sv.tex; nd_ws.use_t1 = sv.use1; nd_ws.td[1].valid = sv.t1v; dual_on = sv.dual; box_prod = sv.bprod; box_pat = sv.bpat; box_folded = sv.bfold;
    comb_set = sv.cset; comb_alpha_shade = sv.cas;
    for (i = 0; i < 16; i++) *tst_f[i] = sv.tst[i];
    comb_mode5 = sv.mode5; plan_active = sv.pact; cur_plan = sv.plan; ls_use = sv.use; ls_why = sv.why; ls_cyc = sv.cyc; ls_dual = sv.dualo; ls_frame = sv.lsf;
    sl_const = sv.slc; sl_env = sv.sle; sl_cc = sv.slcc;
}

void sync_combine_work_timed(void) {
    if (!pm_ph_on || !ph_pick()) { sync_combine_work(); return; }
    unsigned t0 = pm_ticks();
    sync_combine_work();
    pm_gbi_sync_ticks += (pm_ticks() - t0) * 8u; /* W2: one call in 8 timed */
}

void sync_combine_work(void) {
    if (gp_on() && !comb_dry) { /* PG0: an estimate of how many calls repeat the inputs of the call before (hash of the combiner, render and tile state the function reads) */
        static unsigned last_sig;
        unsigned sg = scm_sig();
        GP_SET(5, sg == last_sig);
        last_sig = sg;
    }
    unsigned cyc = (nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3;
    sync_legacy();
    plan_active = 0;
    ls_use = -1; ls_cyc = (int)cyc; ls_dual = dual_on | box_prod << 1 | box_pat << 2; ls_frame = rm_frame;
    if (cyc <= 1 && comb_set && !dual_on && (!box_prod || !box_folded)) { /* VIS1: a draw_box window with a pattern in tile 0 reads TEXEL0 * TEXEL1 on the GPU plan too. Triangles (rotated and scaled windows) have no CPU composite like box_pattern_rect, so they lost the corner image: no border, no round corners */
        Dec* d = decide((int)cyc);
        ls_use = d->use; ls_why = d->why;
        if (d->use) {
            cur_plan = d->plan;
            pm_comb_colours(&cur_plan, nd_ws.prim, nd_ws.envc, (u8)nd_ws.prim_lod, keyc, keys);
            if (!comb_dry) { pm_gpu_plan_key(nd_ws.comb_w0, nd_ws.comb_w1, (int)cyc, nd_ws.use_t1); pm_gpu_set_plan(&cur_plan); }
            plan_active = 1;
            cc07_dbg();
            return;
        }
    }
    cc07_dbg();
    if (!comb_dry) pm_gpu_set_combine(leg_mode);
}

void sync_combine(void) {
    {   /* DIP2: the same inputs as the call before give the same combine mode or plan. Skip the work. The inputs are everything sync_legacy, const_rgb_tex_alpha, decide and
         * pm_comb_colours read. pm_gpu_set_combine at the box pattern draw (below) and the start of a display list clear sc_ok. Switch nodip2 turns it off. */
        unsigned sg = sc_sig_now();
        if (sc_ok && sg == sc_sig) { GP_SET(5, 1); return; }
        sc_sig = sg;
        sc_ok = 0; sr_ok = 0; /* set again below when the work is done */
    }
    int leg0 = leg_mode, pat0 = box_pat;
    NdCombIn cin; int chk = combchk_on() ? (comb_in_walker(&cin), 1) : 0; /* S3N-3d: inputs before the work */
    int hit = 0;
    { unsigned sig = scm_sig(); ScMemo* m = scm_find(sig); if (m) { scm_replay(m); hit = 1; } else { sync_combine_work_timed(); scm_store(sig); } }
    if (chk) combchk_run(&cin, !hit);
    sc_ok = 1;
    if (leg_mode != leg0 || box_pat != pat0) {   /* the work may change leg_mode and box_pat (inputs): the signature of the next call must start from the new value (PG4: no other input changes, so the hash is made again only then) */
        unsigned parts[] = { nd_ws.comb_w0, nd_ws.comb_w1, nd_ws.omode_h, nd_ws.prim, nd_ws.envc, (unsigned)(u8)nd_ws.prim_lod, keyc, keys, conv_k4, (unsigned)leg_mode,
                             (unsigned)(nd_ws.tex_on | nd_ws.use_t1 << 1 | nd_ws.td[1].valid << 2 | dual_on << 3 | box_prod << 4 | box_pat << 5 | box_folded << 6 | comb_set << 7 | comb_alpha_shade << 8) };
        unsigned sg = 2166136261u;
        for (unsigned k = 0; k < sizeof parts / sizeof parts[0]; k++) sg = (sg ^ parts[k]) * 16777619u;
        sc_sig = sg;
    }
}

/* All texels of tile 0 are 1.0 (I4 0xF or I8 0xFF)? Then TEXEL0 * TEXEL1 is TEXEL1. */
int tile0_white(void) {
    const u8* src = nd_ws.td[0].src;
    if (!nd_ws.td[0].valid || !src || nd_ws.td[0].fmt != G_IM_FMT_I || nd_ws.td[0].siz > 1) return 0;
    for (int y = 0; y < nd_ws.td[0].h; y++) {
        for (int x = 0; x < nd_ws.td[0].w; x++) {
            if (nd_ws.td[0].siz == 0) {
                unsigned b = src[(y * nd_ws.td[0].stride + x) >> 1];
                if (((x & 1) ? (b & 15u) : (b >> 4)) != 15u) return 0;
            } else if (src[y * nd_ws.td[0].stride + x] != 255) {
                return 0;
            }
        }
    }
    return 1;
}

/* Called before each textured draw. Tiles 0 and 1 are known by then, the combine mode came
 * before them. */
void box_fold_sync(void) {
    int want = box_prod && nd_ws.td[1].valid && tile0_white();
    if (want == box_folded) return;
    box_folded = want;
    comb_gen++;
    if (want) {
        box_save[0] = nd_ws.tst1.comb_a; box_save[1] = nd_ws.tst1.comb_b; box_save[2] = nd_ws.tst1.comb_c; box_save[3] = nd_ws.tst1.comb_d;
        nd_ws.tst1.comb_a = G_CCMUX_PRIMITIVE; nd_ws.tst1.comb_b = G_CCMUX_ENVIRONMENT; nd_ws.tst1.comb_c = G_CCMUX_TEXEL0; nd_ws.tst1.comb_d = G_CCMUX_ENVIRONMENT;
        nd_ws.tst1.comb_aa = 7; nd_ws.tst1.comb_ab = 7; nd_ws.tst1.comb_ac = 7; nd_ws.tst1.comb_ad = 1; /* TEXEL1 alpha, renamed TEXEL0 */
        nd_ws.use_t1 = 1;
    } else {
        nd_ws.tst1.comb_a = box_save[0]; nd_ws.tst1.comb_b = box_save[1]; nd_ws.tst1.comb_c = box_save[2]; nd_ws.tst1.comb_d = box_save[3];
        nd_ws.tst1.comb_aa = 7; nd_ws.tst1.comb_ab = 7; nd_ws.tst1.comb_ac = 7; nd_ws.tst1.comb_ad = 2;
        nd_ws.use_t1 = 0;
    }
    sync_combine();
}

float t1_p[4]; /* the unit 1 uv map set by dual_bind: scale s t, offset s t */
int t1_ox, t1_oy, t1_w, t1_h; /* window origin in the slot, texture size of unit 1 */
void dual_bind(void) {
    unsigned key = 2166136261u, i;
    int tw0, th0, tw1, th1;
    if (!plan_active || !cur_plan.uses_tex1 || !nd_ws.td[0].valid || !nd_ws.td[1].valid) return;
    {
        unsigned parts[] = { (unsigned)nd_ws.td[1].src, nd_ws.td[1].fmt, nd_ws.td[1].siz, (unsigned)nd_ws.td[1].w, (unsigned)nd_ws.td[1].h, (unsigned)nd_ws.td[1].stride,
                             (unsigned)nd_ws.td[1].pal, nd_ws.tst1.tile_wrap[1][0], nd_ws.tst1.tile_wrap[1][1], nd_ws.tst1.tile_wrap[1][2], nd_ws.tst1.tile_wrap[1][3] };
        for (i = 0; i < sizeof parts / sizeof parts[0]; i++) key = (key ^ parts[i]) * 16777619u;
    }
    if (key != aux_key || !pm_tex_aux_valid()) {
        pm_gpu_flush();
        pm_tex_set_wrap(nd_ws.tst1.tile_wrap[1][0], nd_ws.tst1.tile_wrap[1][1], nd_ws.tst1.tile_wrap[1][2], nd_ws.tst1.tile_wrap[1][3]);
        if (nd_ws.td[1].fmt == 0 && nd_ws.td[1].siz == 2) grab_cur(nd_ws.td[1].src, nd_ws.td[1].w, nd_ws.td[1].h);
        tex_gen++; /* TBREP */
        pm_tex_load(nd_ws.td[1].src, nd_ws.td[1].fmt, nd_ws.td[1].siz, nd_ws.td[1].w, nd_ws.td[1].h, nd_ws.td[1].pal, nd_ws.td[1].pal_n, nd_ws.td[1].stride);
        pm_tex_set_aux(nd_ws.tst1.tile_wrap[1][0], nd_ws.tst1.tile_wrap[1][1], nd_ws.tst1.tile_wrap[1][2], nd_ws.tst1.tile_wrap[1][3]);
        aux_key = key;
        nd_ws.cur_tile = -1;
        ens_clear();
    }
    ensure_tile(0);
    pm_tex_size(&tw0, &th0);
    pm_tex_aux_size(&tw1, &th1);
    {   /* V12c: the two tiles may have different shifts (bulb glow: tile 0 shift 1, tile 1 shift 0) and TEXEL1 may be a slot with a window origin */
        int ox1, oy1;
        float rs = shift_mul(nd_ws.tst1.tile_shift[1][0]) / shift_mul(nd_ws.tst1.tile_shift[0][0]), rt = shift_mul(nd_ws.tst1.tile_shift[1][1]) / shift_mul(nd_ws.tst1.tile_shift[0][1]); if (pm_debug_has("texrt1")) rt = 1.f; /* TEXGARBLE test switch (scratch): ignore the t shift of tile 1 */
        pm_tex_aux_origin(&ox1, &oy1);
        t1_p[0] = (float)tw0 / (float)tw1 * rs;
        t1_p[1] = (float)th0 / (float)th1 * rt;
        t1_p[2] = ((float)nd_ws.tile_x0 * rs - (float)(nd_ws.td[1].x0 - ox1)) / (float)tw1;
        t1_p[3] = ((float)nd_ws.tile_y0 * rt - (float)(nd_ws.td[1].y0 - oy1)) / (float)th1;
        t1_ox = ox1; t1_oy = oy1; t1_w = tw1; t1_h = th1;
        pm_gpu_set_tex1(t1_p[0], t1_p[1], t1_p[2], t1_p[3]);
        if (suntile_on() && suntile_lines < 400) {
            suntile_lines++;
            pm_log("suntile t1 f%u uls %d ult %d lrs %d lrt %d | cms %u cmt %u masks %u maskt %u shifts %u %u | "
                   "t1 tex %dx%d fmt %u siz %u td.x0 %d td.y0 %d | tile_x0 %d tile_y0 %d fx %.2f fy %.2f | origin %d %d | "
                   "t0 tex %dx%d tile0 uls %d ult %d lrs %d lrt %d cms %u cmt %u masks %u maskt %u | "
                   "p %.5f %.5f %.5f %.5f | aux_off %d",
                   pm_frame_count(), nd_ws.tst1.tile_sz[1][0], nd_ws.tst1.tile_sz[1][1], nd_ws.tst1.tile_sz[1][2], nd_ws.tst1.tile_sz[1][3],
                   nd_ws.tst1.tile_wrap[1][0], nd_ws.tst1.tile_wrap[1][1], nd_ws.tst1.tile_wrap[1][2], nd_ws.tst1.tile_wrap[1][3], nd_ws.tst1.tile_shift[1][0], nd_ws.tst1.tile_shift[1][1],
                   tw1, th1, nd_ws.td[1].fmt, nd_ws.td[1].siz, nd_ws.td[1].x0, nd_ws.td[1].y0, nd_ws.tile_x0, nd_ws.tile_y0, (double)nd_ws.tile_fx, (double)nd_ws.tile_fy, ox1, oy1,
                   tw0, th0, nd_ws.tst1.tile_sz[0][0], nd_ws.tst1.tile_sz[0][1], nd_ws.tst1.tile_sz[0][2], nd_ws.tst1.tile_sz[0][3],
                   nd_ws.tst1.tile_wrap[0][0], nd_ws.tst1.tile_wrap[0][1], nd_ws.tst1.tile_wrap[0][2], nd_ws.tst1.tile_wrap[0][3],
                   (double)t1_p[0], (double)t1_p[1], (double)t1_p[2], (double)t1_p[3], pm_tex_aux_off_idx());
        }
    }
}

/* R13b: a tile 1 window that wraps (G_TX_WRAP, mask bits) is read at (s - sl) & (size - 1). The draw may start outside the window
 * (water_splash: window origin 65, droplet s from 0, so the RDP reads the window at 31, 0, 1 ...). dual_bind maps s linearly, which
 * reads the slot far from the window and finds nothing grabbed there. Fold the start of this draw into the window. The draw is
 * kept inside the window when it fits. The wrap inside one draw is not reproduced, the part past the window edge reads the
 * neighbour pixels of the slot. Tile 0 uv range of the draw: u0..u1 and v0..v1. */
void t1_fold(float u0, float u1, float v0, float v1) {
    if (pm_tex_aux_off_idx() < 0) return; /* an ordinary texture: the GPU repeat and mirror give the RDP wrap, the fold moved the start by the mask size (wrong for a mirrored axis, period twice the mask) */
    float o[2] = { t1_p[2], t1_p[3] };
    const float lo[2] = { u0, v0 }, ex[2] = { (u1 - u0) * t1_p[0] * (float)t1_w, (v1 - v0) * t1_p[1] * (float)t1_h };
    const float sc[2] = { t1_p[0], t1_p[1] };
    const int org[2] = { t1_ox, t1_oy }, dim[2] = { t1_w, t1_h };
    int changed = 0;
    for (int a = 0; a < 2; a++) {
        unsigned mask = nd_ws.tst1.tile_wrap[1][2 + a];
        float win, ws, wsz, wrapped;
        if (!mask || mask > 9) continue;
        if (nd_ws.tst1.tile_wrap[1][a] & 2u) continue; /* V21: a CLAMP axis has no wrap or mirror: the RDP reads (s - sl) clamped to the window, the linear map of dual_bind is right. Folding moved the star icons on the Bowser cards by 9 texels. */
        wsz = (float)(1u << mask);
        win = (lo[a] * sc[a] + o[a]) * (float)dim[a] - (float)org[a]; /* window position of the draw start, texels */
        if (win >= 0.f && win < wsz) continue;
        ws = win - wsz * floorf(win / wsz);
        { float lim = ex[a] >= wsz && (float)dim[a] > wsz ? (float)dim[a] : wsz; /* FX2a: a draw over the whole window (flame noise scroll) reads the wrapped pad of the texture */
          wrapped = ws + ex[a] > lim ? (lim - ex[a] > 0.f ? lim - ex[a] : 0.f) : ws; }
        o[a] += (wrapped - win) / (float)dim[a];
        changed = 1;
    }
    if (changed) pm_gpu_set_tex1(t1_p[0], t1_p[1], o[0], o[1]);
}



void op_setcombine(u32 w0, u32 w1) {
            GP_SET(0, nd_ws.comb_w0 == w0 && nd_ws.comb_w1 == w1 && comb_set);
            /* POND 3a: an open batch is drawn with the combine it was queued with. A new combine closes it first (the copy record's combine closed the blocker batch with the water combine). */
            if (!(nd_ws.comb_w0 == w0 && nd_ws.comb_w1 == w1 && comb_set)) pm_gpu_flush();
            nd_ws.comb_w0 = w0;
            nd_ws.comb_w1 = w1;
            nd_ws.tst1.comb_a = (w0 >> 20) & 0xf;
            nd_ws.tst1.comb_c = (w0 >> 15) & 0x1f;
            nd_ws.tst1.comb_b = (w1 >> 28) & 0xf;
            nd_ws.tst1.comb_d = (w1 >> 15) & 7;
            comb_set = 1;
            nd_ws.tst1.comb_aa = (w0 >> 12) & 7;
            nd_ws.tst1.comb_ac = (w0 >> 9) & 7;
            nd_ws.tst1.comb_ab = (w1 >> 12) & 7;
            nd_ws.tst1.comb_ad = (w1 >> 9) & 7;
            nd_ws.tst1.comb2_a = (w0 >> 5) & 0xf;
            nd_ws.tst1.comb2_c = w0 & 0x1f;
            nd_ws.tst1.comb2_b = (w1 >> 24) & 0xf;
            nd_ws.tst1.comb2_d = (w1 >> 6) & 7;
            nd_ws.tst1.comb2_aa = (w1 >> 21) & 7;
            nd_ws.tst1.comb2_ac = (w1 >> 18) & 7;
            nd_ws.tst1.comb2_ab = (w1 >> 3) & 7;
            nd_ws.tst1.comb2_ad = w1 & 7;
            /* TEXEL1 with no TEXEL0 anywhere: the texture is tile 1. Rename it so one binding serves. */
            {
                int t0 = nd_ws.tst1.comb_a == 1 || nd_ws.tst1.comb_b == 1 || nd_ws.tst1.comb_d == 1 || nd_ws.tst1.comb_c == 1 || nd_ws.tst1.comb_c == 8 ||
                         nd_ws.tst1.comb_aa == 1 || nd_ws.tst1.comb_ab == 1 || nd_ws.tst1.comb_ac == 1 || nd_ws.tst1.comb_ad == 1;
                int t1 = nd_ws.tst1.comb_a == 2 || nd_ws.tst1.comb_b == 2 || nd_ws.tst1.comb_d == 2 || nd_ws.tst1.comb_c == 2 || nd_ws.tst1.comb_c == 9 ||
                         nd_ws.tst1.comb_aa == 2 || nd_ws.tst1.comb_ab == 2 || nd_ws.tst1.comb_ac == 2 || nd_ws.tst1.comb_ad == 2;
                int c2 = nd_ws.tst1.comb2_a == 1 || nd_ws.tst1.comb2_b == 1 || nd_ws.tst1.comb2_d == 1 || nd_ws.tst1.comb2_c == 1 || nd_ws.tst1.comb2_c == 8 ||
                         nd_ws.tst1.comb2_aa == 1 || nd_ws.tst1.comb2_ab == 1 || nd_ws.tst1.comb2_ac == 1 || nd_ws.tst1.comb2_ad == 1 ||
                         nd_ws.tst1.comb2_a == 2 || nd_ws.tst1.comb2_b == 2 || nd_ws.tst1.comb2_d == 2 || nd_ws.tst1.comb2_c == 2 || nd_ws.tst1.comb2_c == 9 ||
                         nd_ws.tst1.comb2_aa == 2 || nd_ws.tst1.comb2_ab == 2 || nd_ws.tst1.comb2_ac == 2 || nd_ws.tst1.comb2_ad == 2;
                nd_ws.use_t1 = 0;
                if (t1 && !t0 && !c2) {
                    nd_ws.use_t1 = 1;
                    if (nd_ws.tst1.comb_a == 2) nd_ws.tst1.comb_a = 1;
                    if (nd_ws.tst1.comb_b == 2) nd_ws.tst1.comb_b = 1;
                    if (nd_ws.tst1.comb_d == 2) nd_ws.tst1.comb_d = 1;
                    if (nd_ws.tst1.comb_c == 2) nd_ws.tst1.comb_c = 1;
                    if (nd_ws.tst1.comb_c == 9) nd_ws.tst1.comb_c = 8;
                    if (nd_ws.tst1.comb_aa == 2) nd_ws.tst1.comb_aa = 1;
                    if (nd_ws.tst1.comb_ab == 2) nd_ws.tst1.comb_ab = 1;
                    if (nd_ws.tst1.comb_ac == 2) nd_ws.tst1.comb_ac = 1;
                    if (nd_ws.tst1.comb_ad == 2) nd_ws.tst1.comb_ad = 1;
                }
            }
            box_folded = 0;
            box_prod = nd_ws.tst1.comb_a == 1 && nd_ws.tst1.comb_b >= 7 && nd_ws.tst1.comb_c == 2 && nd_ws.tst1.comb_d == 7 &&
                       nd_ws.tst1.comb_aa == 7 && nd_ws.tst1.comb_ab == 7 && nd_ws.tst1.comb_ac == 7 && nd_ws.tst1.comb_ad == 2 &&
                       nd_ws.tst1.comb2_a == 3 && nd_ws.tst1.comb2_b == 5 && nd_ws.tst1.comb2_c == 0 && nd_ws.tst1.comb2_d == 5 &&
                       nd_ws.tst1.comb2_aa == 7 && nd_ws.tst1.comb2_ab == 7 && nd_ws.tst1.comb2_ac == 7 && nd_ws.tst1.comb2_ad == 0;
            comb_alpha_shade = ((w0 >> 12) & 7) == 4 || ((w0 >> 9) & 7) == 4 ||
                               ((w1 >> 12) & 7) == 4 || ((w1 >> 9) & 7) == 4;
            cmb_gen_seen = comb_gen; /* R2B */
}

/* ---- MV-10: debug switch combchk, cut from pm_gbi.c (no change) ---- */
/* S3N-3d: debug switch combchk. Each combine work of sync_combine runs nd_comb_decide on its inputs and compares the outputs with the walker state. */
int combchk_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("combchk"); return v; }
