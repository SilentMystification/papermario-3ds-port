/* S25: RDP colour combiner (A-B)*C+D, one or two cycles, as PICA texenv stages.
 * No libctru here: the stage description is plain data (PmPlan), pm_gpu.c writes it to the GPU.
 *
 * Forms, per channel group (colour and alpha are lowered apart, then share stages):
 *   D when C == 0 or A == B; A*C and A*C+D (B == 0); (1-B)*C+D (A == 1, operand 1-x); lerp(A,B,C) when B == D
 *   (INTERPOLATE); D - B*C (A == 0, two stages); (A-B)*C (SUBTRACT then MODULATE, exact: a negative A-B
 *   gives 0 after the RDP clamp too); the general (A-B)*C+D as SUBTRACT then MULTIPLY_ADD (the PICA clamps A-B
 *   at 0: not exact when A < B).
 * The PICA has one constant per stage, one buffer colour and the vertex colour. PRIM, ENV and PRIM_LOD_FRAC
 * are put in those three slots by name. The vertex colour carries a constant when the shade is not read.
 * TEXEL0 and TEXEL1 read together: TEXTURE0 and TEXTURE1 (TEXEL1 coordinates are a per-draw affine of the first). */
#include <string.h>
#include "pm_gpu.h"

enum { TN_ZERO, TN_ONE, TN_T0, TN_SH, TN_PR, TN_EN, TN_CMB, TN_PLOD, TN_PRV, TN_T1, TN_KEY, TN_PK, TN_KS, TN_HALF, TN_K4, TN_FC, TN_FKP, TN_FKN, TN_KH, TN_X0, TN_X1, TN_X2, TN_X3, TN_NOISE };
typedef struct { unsigned char n, a; } Term; /* a: 1 = the alpha part of the source */
typedef struct { unsigned char func, scale; Term t[3]; unsigned char inv[3]; } Sk;

#define TM(n_, a_) ((Term){ (unsigned char)(n_), (unsigned char)(a_) })

static int teq(Term x, Term y) {
    if (x.n != y.n) return 0;
    if (x.n == TN_ZERO || x.n == TN_ONE) return 1;
    return x.a == y.a;
}
static int isz(Term x) { return x.n == TN_ZERO; }
static int iso(Term x) { return x.n == TN_ONE; }

static Sk mk(int f, Term a, Term b, Term c, int ia, int ib, int ic) {
    Sk s;
    memset(&s, 0, sizeof s);
    s.func = (unsigned char)f;
    s.t[0] = a; s.t[1] = b; s.t[2] = c;
    s.inv[0] = (unsigned char)ia; s.inv[1] = (unsigned char)ib; s.inv[2] = (unsigned char)ic;
    return s;
}

/* The terms of one cycle. rgb: slot 0..3 = A B C D. */
static Term term_rgb(int slot, unsigned sel, int cyc2, int t1only) {
    switch (sel) {
    case 0: return cyc2 ? TM(TN_CMB, 0) : TM(TN_ZERO, 0);
    case 1: return TM(TN_T0, 0);
    case 2: return t1only ? TM(TN_T0, 0) : TM(TN_T1, 0);
    case 3: return TM(TN_PR, 0);
    case 4: return TM(TN_SH, 0);
    case 5: return TM(TN_EN, 0);
    case 6: /* B6: slot 1 is CENTER, slot 2 is the key SCALE */
        return slot == 1 ? TM(TN_KEY, 0) : slot == 2 ? TM(TN_KS, 0) : slot == 0 || slot == 3 ? TM(TN_ONE, 0) : TM(TN_ZERO, 0);
    default: break;
    }
    if (slot == 0 && sel == 7) return TM(TN_NOISE, 0); /* PT07: the RDP noise input, a random value per pixel */
    if (slot == 1 && sel == 7) return TM(TN_K4, 0); /* the convert constant K4 (flame: (TEXEL1 - K4) * PRIM alpha) */
    if (slot == 2) {
        switch (sel) {
        case 7: return cyc2 ? TM(TN_CMB, 1) : TM(TN_ZERO, 0);
        case 8: return TM(TN_T0, 1);
        case 9: return t1only ? TM(TN_T0, 1) : TM(TN_T1, 1);
        case 10: return TM(TN_PR, 1);
        case 11: return TM(TN_SH, 1);
        case 12: return TM(TN_EN, 1);
        case 14: return TM(TN_PLOD, 0);
        default: break;
        }
    }
    return TM(TN_ZERO, 0);
}

static Term term_a(int slot, unsigned sel, int cyc2, int t1only) {
    switch (sel) {
    case 0: return slot == 2 ? TM(TN_ZERO, 0) : (cyc2 ? TM(TN_CMB, 1) : TM(TN_ZERO, 0));
    case 1: return TM(TN_T0, 1);
    case 2: return t1only ? TM(TN_T0, 1) : TM(TN_T1, 1);
    case 3: return TM(TN_PR, 1);
    case 4: return TM(TN_SH, 1);
    case 5: return TM(TN_EN, 1);
    case 6: return slot == 2 ? TM(TN_PLOD, 0) : TM(TN_ONE, 0);
    default: return TM(TN_ZERO, 0);
    }
}

static int hold_alpha; /* the colour group of this cycle needs the cycle 1 alpha in its first stage: the alpha group goes last */

static int cur_grp, fold_off; /* group being lowered (0 colour); fold_off: build without the folded form */
static unsigned char fold_t[6]; /* B, C, D of the folded colour group: n and alpha flag each */
static int is_k(Term t) { return t.n == TN_ZERO || t.n == TN_ONE || t.n == TN_PR || t.n == TN_EN || t.n == TN_PLOD; }

static int short_step;
static unsigned char kspec[4][5];
static int nk, kover;
/* A derived constant (kind 0: B * C / 2, kind 1: C / 2), one of 4 per plan. */
static Term kterm(int kind, Term b, Term c) {
    if (nk >= 4) { kover = 1; return TM(TN_X0, 0); }
    kspec[nk][0] = (unsigned char)kind; kspec[nk][1] = b.n; kspec[nk][2] = b.a; kspec[nk][3] = c.n; kspec[nk][4] = c.a;
    return TM(TN_X0 + nk++, 0);
}

/* Lower one group. Returns the stage count (1 to 4), or a negative reason. */
static int lower(Term A, Term B, Term C, Term D, Sk out[4]) {
    Term prv = TM(TN_PRV, 0);
    Term z = TM(TN_ZERO, 0);
    if (isz(C) || teq(A, B)) { out[0] = mk(PM_F_REPLACE, D, D, D, 0, 0, 0); return 1; }
    if (isz(B)) {
        if (isz(D)) {
            if (iso(A)) out[0] = mk(PM_F_REPLACE, C, C, C, 0, 0, 0);
            else out[0] = mk(PM_F_MODULATE, A, C, C, 0, 0, 0);
        } else if (iso(A)) {
            out[0] = mk(PM_F_ADD, C, D, D, 0, 0, 0);
        } else {
            out[0] = mk(PM_F_MULADD, A, C, D, 0, 0, 0);
        }
        return 1;
    }
    if (!cur_grp && is_k(B) && !isz(B) && is_k(C) && !isz(C) && !iso(C) && (A.n == TN_T0 || A.n == TN_T1) && D.n == TN_CMB && !D.a) {
        /* VIS1 (PT11): (T - K1) * K2 + COMBINED, cycle 2 of the draw_box style 0 window (T1 - ENV) * ENVA + T0 * PRIM. The sum T * K2 + COMBINED may pass 1 before
         * K1 * K2 comes off, and the PICA clamps each stage. Halved: H = COMBINED / 2, G = T * K2 / 2 + H (under 1), 2 * (G - K1 * K2 / 2) is exact. The clamp at 0 is the RDP clamp. */
        Term h = TM(TN_HALF, 0);
        Term kh = kterm(1, C, C);
        Term kb = kterm(0, B, C);
        out[0] = mk(PM_F_MODULATE, prv, h, h, 0, 0, 0);
        out[1] = mk(PM_F_MULADD, A, kh, prv, 0, 0, 0);
        out[2] = mk(PM_F_SUBTRACT, prv, kb, kb, 0, 0, 0); out[2].scale = 2;
        hold_alpha = 1;
        return 3;
    }
    if (B.n == TN_K4 && C.n == TN_CMB && C.a && D.n == TN_CMB && !D.a) {
        /* (X - K4) * COMBINED alpha + COMBINED, cycle 2. y = K4 * a + (1 - P), then X * a + (1 - y) = X * a + P - K4 * a.
         * The alpha of the first stage passes the cycle 1 alpha on to the second (the alpha group is placed last). */
        Term pa = TM(TN_PRV, 1), pc = TM(TN_PRV, 0);
        out[0] = mk(PM_F_MULADD, B, pa, pc, 0, 0, 1);
        out[1] = mk(PM_F_MULADD, A, pa, pc, 0, 0, 1);
        hold_alpha = 1;
        return 2;
    }
    if ((B.n == TN_K4 || (is_k(B) && !isz(B))) && is_k(C) && !isz(C) && !iso(C) && (A.n == TN_T0 || A.n == TN_T1) && (D.n == TN_T0 || D.n == TN_T1)) {
        /* FX2a, flame blur cycle 1: (T1 - K4) * k + T0 = k * T1 + T0 - K4 * k. The PICA clamps T1 - K4 at 0 and loses the negative part.
         * Halved: H = (k * T1 + T0) / 2 stays under 1, then 2 * (H - K4 * k / 2) is exact, the clamp at 0 is the RDP clamp.
         * FX3c: the same with a constant B (effect_63 alpha (T1A - PRIMA) * ENVA + T0A). */
        Term kc = kterm(0, B, C);
        out[0] = mk(PM_F_MODULATE, A, C, C, 0, 0, 0);
        out[1] = mk(PM_F_INTERP, prv, D, TM(TN_HALF, 0), 0, 0, 0);
        out[2] = mk(PM_F_SUBTRACT, prv, kc, kc, 0, 0, 0); out[2].scale = 2;
        return 3;
    }
    if (cur_grp && is_k(C) && teq(C, D) && !isz(C) && !iso(C) && (A.n == TN_T0 || A.n == TN_T1) && (B.n == TN_T0 || B.n == TN_T1)) {
        /* FX2a: (A - B) * k + k = k * (1 - B) + k * A, no negative step, the sum clamps at 1 as the RDP does. */
        out[0] = mk(PM_F_MODULATE, B, C, C, 1, 0, 0);
        out[1] = mk(PM_F_MULADD, A, C, prv, 0, 0, 0);
        return 2;
    }
    if (is_k(D) && !isz(D) && !is_k(C) && C.n != TN_PRV && !is_k(A) && !is_k(B)) {
        /* FX3c: (A - B) * C + D, D a constant, A B C sources (effect_63 (T1 - T0) * T0 + ENV and (COMBINED - T0) * T1 + PRIM). A - B is signed and
         * the PICA clamps it. s = (A + 1 - B) / 2, m = s * C + (1 - C) / 2 = (1 + C * (A - B)) / 2, result = 2 * (m + D / 2 - 1/2) = C * (A - B) + D.
         * Every stage stays in 0..1, the final clamp is the RDP clamp. 3 stages. */
        Term h = TM(TN_HALF, 0);
        Term dh = kterm(1, D, D);
        out[0] = mk(PM_F_INTERP, A, B, h, 0, 1, 0);
        out[1] = mk(PM_F_INTERP, prv, h, C, 0, 0, 0);
        out[2] = mk(PM_F_ADDSIGNED, prv, dh, dh, 0, 0, 0); out[2].scale = 2;
        return 3;
    }
    if (iso(A)) {
        if (isz(D)) out[0] = mk(PM_F_MODULATE, B, C, C, 1, 0, 0);
        else out[0] = mk(PM_F_MULADD, B, C, D, 1, 0, 0);
        return 1;
    }
    if (teq(B, D)) { out[0] = mk(PM_F_INTERP, A, B, C, 0, 0, 0); return 1; }
    if (isz(A)) {
        if (isz(D)) {
            /* 0 - B*C: the RDP result is a 9 bit value. A product over 128/255 wraps to 255 and a smaller one to 0, so the result
             * is a step at 128.5/255. PICA: B*C, minus 0.5 (x4), then two doublings (x8 each): 4 * 8 * 8 = 256 turns one step into 0 or 1. */
            Term h = TM(TN_HALF, 0);
            out[0] = mk(PM_F_MODULATE, B, C, C, 0, 0, 0);
            out[1] = mk(PM_F_SUBTRACT, prv, h, h, 0, 0, 0); out[1].scale = 4;
            out[2] = mk(PM_F_ADD, prv, prv, prv, 0, 0, 0);  out[2].scale = 4;
            out[3] = out[2];
            if (short_step) return 3; /* FX3c: out of stages, gain 4 * 8 = 32: a step that is 1/32 wide instead of 1/256 */
            return 4;
        }
        if (D.n == TN_CMB) return -3;
        out[0] = mk(PM_F_MODULATE, B, C, C, 0, 0, 0);
        out[1] = mk(PM_F_SUBTRACT, D, prv, prv, 0, 0, 0);
        return 2;
    }
    if (!cur_grp && !fold_off && !fold_t[2] && !is_k(A) && is_k(B) && is_k(C) && is_k(D)) {
        /* H06b3: (A - B) * C + D with B, C, D constants (hos_00 (T0 - ENV) * ENVA + PRIM) is C * A + K with K = D - B * C known at the draw.
         * The PICA clamps A - B at 0, the RDP keeps it negative. K is split into Kp and Kn (one of them is 0 per channel): A * C, + Kp, - Kn.
         * Each stage reads one constant, an exact result for every A. */
        fold_t[0] = B.n; fold_t[1] = B.a; fold_t[2] = C.n; fold_t[3] = C.a; fold_t[4] = D.n; fold_t[5] = D.a;
        out[0] = mk(PM_F_MODULATE, A, TM(TN_FC, 0), TM(TN_FC, 0), 0, 0, 0);
        out[1] = mk(PM_F_ADD, prv, TM(TN_FKP, 0), TM(TN_FKP, 0), 0, 0, 0);
        out[2] = mk(PM_F_SUBTRACT, prv, TM(TN_FKN, 0), TM(TN_FKN, 0), 0, 0, 0);
        return 3;
    }
    if (!cur_grp && !fold_t[2] && B.n == TN_CMB && !B.a && is_k(A) && teq(A, D) && C.n != TN_CMB) {
        /* H06b2: (K - COMBINED) * X + K, cycle 2. K - COMBINED is negative where COMBINED > K and the PICA clamps it at 0. With Q = 1 - COMBINED:
         * S = (Q + K) / 2, Mid = S * X + (1 - X) / 2, result = 2 * (Mid - (1 - K) / 2) = K + X * (2 S - 1). All stages stay in 0..1, so the clamp is exact.
         * HALF rides in the buffer colour, (1 - K) / 2 is the constant KH. */
        Term h = TM(TN_HALF, 0), kh = TM(TN_KH, 0);
        fold_t[0] = A.n; fold_t[1] = A.a;
        out[0] = mk(PM_F_INTERP, prv, A, h, 1, 0, 0);
        out[1] = mk(PM_F_INTERP, prv, h, C, 0, 0, 0);
        out[2] = mk(PM_F_SUBTRACT, prv, kh, kh, 0, 0, 0); out[2].scale = 2;
        return 3;
    }
    if (C.n == TN_CMB || D.n == TN_CMB) return -3;
    if (A.n == TN_NOISE && is_k(B) && !isz(B) && is_k(C) && !isz(C) && !iso(C) && D.n != TN_NOISE) {
        /* PT07: (NOISE - B) * C + D with B, C constants (the hologram: (NOISE - PRIM) * PRIM + TEXEL0) is D - B * C + NOISE * C. The PICA clamps at 0 after each
         * stage, so the stage order keeps the carry positive: D - B * C first (clamped only where D < B * C), then NOISE * C + that. */
        out[0] = mk(PM_F_SUBTRACT, D, kterm(2, B, C), kterm(2, B, C), 0, 0, 0);
        out[1] = mk(PM_F_MULADD, A, C, prv, 0, 0, 0);
        return 2;
    }
    out[0] = mk(PM_F_SUBTRACT, A, B, B, 0, 0, 0);
    if (isz(D)) out[1] = mk(PM_F_MODULATE, prv, C, C, 0, 0, 0);
    else out[1] = mk(PM_F_MULADD, prv, C, D, 0, 0, 0);
    return 2;
}

typedef struct { Sk s[6]; int n; } Seq;

static int add_cycle(Seq* r, Seq* a, int start_r, int start_a, Term v[2][4]) {
    Sk o[4];
    int n;
    int nr;
    hold_alpha = 0;
    cur_grp = 0;
    n = lower(v[0][0], v[0][1], v[0][2], v[0][3], o);
    if (n < 0) return n;
    if (start_r + n > 6) return -1;
    for (int i = 0; i < n; i++) r->s[start_r + i] = o[i];
    r->n = start_r + n;
    nr = n;
    cur_grp = 1;
    n = lower(v[1][0], v[1][1], v[1][2], v[1][3], o);
    if (n < 0) return n;
    if (hold_alpha && n < nr) { /* stages before the alpha group only pass the alpha */
        for (int i = 0; i < nr - n; i++) a->s[start_a + i] = mk(PM_F_REPLACE, TM(TN_PRV, 0), TM(TN_PRV, 0), TM(TN_PRV, 0), 0, 0, 0);
        start_a += nr - n;
    }
    if (start_a + n > 6) return -1;
    for (int i = 0; i < n; i++) a->s[start_a + i] = o[i];
    a->n = start_a + n;
    return 0;
}

/* Resolve a skeleton input to a hardware source. Returns 0, or a reason. */
typedef struct { unsigned char stage_name[6]; unsigned char buf, vtx; } Alloc;

static int src_of(Term t, int inv, int grp, int stage, Alloc* al, unsigned char* src, unsigned char* op, PmPlan* pl) {
    int name = 0, part = t.a, ninv = inv;
    switch (t.n) {
    case TN_T0: *src = PM_S_TEX0; pl->uses_tex = 1; break;
    case TN_T1: *src = PM_S_TEX1; pl->uses_tex1 = 1; break;
    case TN_NOISE: *src = PM_S_TEX2; pl->uses_noise = 1; break;
    case TN_SH:
        *src = PM_S_VTX;
        if (al->vtx != PM_NM_SHADE) return 5;
        if (part) pl->uses_sha = 1;
        break;
    case TN_PRV: *src = PM_S_PREV; break;
    case TN_CMB: *src = PM_S_PREV; break;
    case TN_ZERO: name = PM_NM_ZERO; part = 0; break;
    case TN_ONE: name = PM_NM_ZERO; part = 0; ninv ^= 1; break;
    case TN_PR: name = PM_NM_PRIM; break;
    case TN_PK: name = PM_NM_PK; break;
    case TN_HALF: name = PM_NM_HALF; break;
    case TN_K4: name = PM_NM_K4; break;
    case TN_FC: name = PM_NM_FC; break;
    case TN_FKP: name = PM_NM_FKP; break;
    case TN_FKN: name = PM_NM_FKN; break;
    case TN_KH: name = PM_NM_KH; break;
    case TN_X0: case TN_X1: case TN_X2: case TN_X3: name = PM_NM_X0 + (t.n - TN_X0); break;
    case TN_KEY: name = PM_NM_KEYC; break;
    case TN_KS: name = PM_NM_KEYS; break;
    case TN_EN: name = PM_NM_ENV; break;
    case TN_PLOD: name = PM_NM_PLOD; break;
    default: return 5;
    }
    if (name) {
        /* The PICA buffer colour reads 0 in stage 0 (it shows from stage 1 on), so stage 0 never uses it. */
        if (al->stage_name[stage] == name) *src = PM_S_CONST;
        else if (stage > 0 && al->buf == name) *src = PM_S_BUF;
        else if (al->vtx == name) *src = PM_S_VTX;
        else if (al->stage_name[stage] == PM_NM_NONE) { al->stage_name[stage] = (unsigned char)name; *src = PM_S_CONST; }
        else if (stage > 0 && al->buf == PM_NM_NONE) { al->buf = (unsigned char)name; *src = PM_S_BUF; }
        else if (al->vtx == PM_NM_NONE) { al->vtx = (unsigned char)name; *src = PM_S_VTX; }
        else return 4;
    }
    if (grp == 1) *op = (unsigned char)(ninv ? PM_O_1MA : PM_O_A);
    else *op = (unsigned char)(part ? (ninv ? PM_O_1MA : PM_O_A) : (ninv ? PM_O_1MC : PM_O_C));
    return 0;
}

static int uses_shade(const Seq* r, const Seq* a) {
    const Seq* q[2] = { r, a };
    for (int g = 0; g < 2; g++)
        for (int i = 0; i < q[g]->n; i++)
            for (int k = 0; k < 3; k++)
                if (q[g]->s[i].t[k].n == TN_SH) return 1;
    return 0;
}

/* R13c: the RDP reads the texels of cycle 2 swapped: TEXEL0 is the tile 1 texel and TEXEL1 the tile 0 texel. s = A B C D, Aa Ab Ac Ad of cycle 2. */
void pm_comb_swap_c2(unsigned s[8]) {
    for (int i = 0; i < 8; i++) {
        unsigned v = s[i];
        if (v == 1 || v == 2) s[i] = 3 - v;
        else if (i == 2 && (v == 8 || v == 9)) s[i] = 17 - v;
    }
}

static int build(unsigned w0, unsigned w1, int two, int t1only, int lead, PmPlan* out) {
    Term v[2][2][4]; /* cycle, group, A B C D */
    unsigned s[2][8];
    Seq R, A;
    Alloc al;
    int rc, nst, start;
    memset(out, 0, sizeof *out);
    memset(&R, 0, sizeof R);
    memset(&A, 0, sizeof A);
    memset(&al, 0, sizeof al);
    memset(fold_t, 0, sizeof fold_t);
    memset(kspec, 0, sizeof kspec); nk = kover = 0;
    s[0][0] = (w0 >> 20) & 0xf; s[0][1] = (w1 >> 28) & 0xf; s[0][2] = (w0 >> 15) & 0x1f; s[0][3] = (w1 >> 15) & 7;
    s[0][4] = (w0 >> 12) & 7;   s[0][5] = (w1 >> 12) & 7;   s[0][6] = (w0 >> 9) & 7;      s[0][7] = (w1 >> 9) & 7;
    s[1][0] = (w0 >> 5) & 0xf;  s[1][1] = (w1 >> 24) & 0xf; s[1][2] = w0 & 0x1f;          s[1][3] = (w1 >> 6) & 7;
    s[1][4] = (w1 >> 21) & 7;   s[1][5] = (w1 >> 3) & 7;    s[1][6] = (w1 >> 18) & 7;     s[1][7] = w1 & 7;
    pm_comb_swap_c2(s[1]);
    for (int c = 0; c < (two ? 2 : 1); c++) {
        for (int i = 0; i < 4; i++) {
            v[c][0][i] = term_rgb(i, s[c][i], c, t1only);
            v[c][1][i] = term_a(i, s[c][4 + i], c, t1only);
        }
    }
    /* B6: PRIM - CENTER (both constants) is one constant PK, B becomes 0. CENTER anywhere else has no source (reason 5). */
    for (int c = 0; c < (two ? 2 : 1); c++)
        if (v[c][0][0].n == TN_PR && v[c][0][1].n == TN_KEY) { v[c][0][0] = TM(TN_PK, 0); v[c][0][1] = TM(TN_ZERO, 0); }
    for (int i = 0; i < lead; i++) { /* a first stage that only passes, so no real stage is stage 0 */
        R.s[i] = A.s[i] = mk(PM_F_REPLACE, TM(TN_PRV, 0), TM(TN_PRV, 0), TM(TN_PRV, 0), 0, 0, 0);
        R.n = A.n = i + 1;
    }
    rc = add_cycle(&R, &A, lead, lead, v[0]);
    if (rc) return -rc;
    if (two) {
        /* Cycle 2 starts after both groups of cycle 1 are done, so COMBINED is the cycle 1 result. */
        start = R.n > A.n ? R.n : A.n;
        for (int i = R.n; i < start; i++) R.s[i] = mk(PM_F_REPLACE, TM(TN_PRV, 0), TM(TN_PRV, 0), TM(TN_PRV, 0), 0, 0, 0);
        for (int i = A.n; i < start; i++) A.s[i] = mk(PM_F_REPLACE, TM(TN_PRV, 0), TM(TN_PRV, 0), TM(TN_PRV, 0), 0, 0, 0);
        rc = add_cycle(&R, &A, start, start, v[1]);
        if (rc) return -rc;
    }
    nst = R.n > A.n ? R.n : A.n;
    for (int i = R.n; i < nst; i++) R.s[i] = mk(PM_F_REPLACE, TM(TN_PRV, 0), TM(TN_PRV, 0), TM(TN_PRV, 0), 0, 0, 0);
    for (int i = A.n; i < nst; i++) A.s[i] = mk(PM_F_REPLACE, TM(TN_PRV, 0), TM(TN_PRV, 0), TM(TN_PRV, 0), 0, 0, 0);
    if (nst > 6) return 1;
    if (uses_shade(&R, &A)) al.vtx = PM_NM_SHADE;
    out->n = nst;
    for (int i = 0; i < nst; i++) {
        const Seq* q[2] = { &R, &A };
        for (int g = 0; g < 2; g++) {
            PmTevCh* ch = &out->st[i].g[g];
            ch->func = q[g]->s[i].func;
            ch->scale = q[g]->s[i].scale;
            for (int k = 0; k < 3; k++) {
                int e = src_of(q[g]->s[i].t[k], q[g]->s[i].inv[k], g, i, &al, &ch->src[k], &ch->op[k], out);
                if (e) return e;
            }
        }
        out->st[i].cname = al.stage_name[i];
    }
    out->bufname = al.buf;
    out->vtxname = al.vtx;
    memcpy(out->fold, fold_t, sizeof fold_t);
    memcpy(out->ks, kspec, sizeof kspec);
    if (kover) return 7;
    return 0;
}

/* Reason 4 at stage 0 can come from the buffer colour, which stage 0 cannot read: try again with one passing stage first. */
static int build_lead(unsigned w0, unsigned w1, int two, int t1only, PmPlan* out) {
    int rc = build(w0, w1, two, t1only, 0, out);
    return rc == 4 ? build(w0, w1, two, t1only, 1, out) : rc;
}
int pm_comb_build(unsigned w0, unsigned w1, int two, int t1only, PmPlan* out) {
    int rc;
    fold_off = 0;
    short_step = 0;
    rc = build_lead(w0, w1, two, t1only, out);
    if (rc == 1) { short_step = 1; rc = build_lead(w0, w1, two, t1only, out); short_step = 0; }
    if (rc && fold_t[2]) { fold_off = 1; rc = build_lead(w0, w1, two, t1only, out); fold_off = 0; } /* the folded form needs 1 stage more: out of stages or constants, use the plain form */
    return rc;
}

static unsigned pm_k4;
void pm_comb_set_k4(unsigned k4) { pm_k4 = k4 & 255u; }

static unsigned pk_colour(unsigned prim, unsigned key) {
    unsigned o = prim & 255u; /* alpha of PK: PRIM alpha */
    for (int sh = 8; sh < 32; sh += 8) {
        int d = (int)((prim >> sh) & 255u) - (int)((key >> sh) & 255u);
        o |= (unsigned)(d < 0 ? 0 : d) << sh; /* PICA constants are not negative: clamp, the RDP result clamps too in the common case */
    }
    return o;
}

static void unp(unsigned c, float o[4]);
static float k_val(unsigned n, unsigned a, int ch, const float pr[4], const float en[4], unsigned plod) {
    switch (n) {
    case TN_ONE: return 1.f;
    case TN_K4: return (float)pm_k4 / 255.f;
    case TN_PR: return pr[a ? 3 : ch];
    case TN_EN: return en[a ? 3 : ch];
    case TN_PLOD: return (float)plod / 255.f;
    default: return 0.f;
    }
}
/* H06b3: the constants of the folded form. kind 0 is C, 1 is Kp = max(D - B * C, 0), 2 is Kn = max(B * C - D, 0), per colour channel. */
static unsigned fold_colour(const PmPlan* p, int kind, unsigned prim, unsigned env, unsigned plod) {
    float pr[4], en[4];
    unsigned o = 0;
    unp(prim, pr); unp(env, en);
    for (int ch = 0; ch < 4; ch++) {
        int c3 = ch < 3 ? ch : 0;
        float b = k_val(p->fold[0], p->fold[1], c3, pr, en, plod), c = k_val(p->fold[2], p->fold[3], c3, pr, en, plod), d = k_val(p->fold[4], p->fold[5], c3, pr, en, plod);
        float k = d - b * c, v = kind == 0 ? c : kind == 1 ? (k > 0.f ? k : 0.f) : (k < 0.f ? -k : 0.f);
        o = o << 8 | (unsigned)(v * 255.f + .5f);
    }
    return o;
}

/* H06b2: (1 - K) / 2 per channel, K named by fold[0], fold[1]. */
static unsigned kh_colour(const PmPlan* p, unsigned prim, unsigned env, unsigned plod) {
    float pr[4], en[4];
    unsigned o = 0;
    unp(prim, pr); unp(env, en);
    for (int ch = 0; ch < 4; ch++) o = o << 8 | (unsigned)((1.f - k_val(p->fold[0], p->fold[1], ch < 3 ? ch : 0, pr, en, plod)) * 127.5f + .5f);
    return o;
}

/* FX2a, FX3c: a derived constant, spec p->ks[i] = kind, B term, B alpha flag, C term, C alpha flag. Kind 0 is B * C / 2, kind 1 is C / 2, kind 2 is B * C, per channel. */
static unsigned x_colour(const PmPlan* p, int i, unsigned prim, unsigned env, unsigned plod) {
    float pr[4], en[4];
    unsigned o = 0;
    unp(prim, pr); unp(env, en);
    for (int ch = 0; ch < 4; ch++) {
        int c3 = ch < 3 ? ch : 0;
        float v = k_val(p->ks[i][3], p->ks[i][4], c3, pr, en, plod) * (p->ks[i][0] == 2 ? 1.f : .5f);
        if (p->ks[i][0] != 1) v *= k_val(p->ks[i][1], p->ks[i][2], c3, pr, en, plod); /* kind 2: B * C (PT07) */
        o = o << 8 | (unsigned)(v * 255.f + .5f);
    }
    return o;
}

static unsigned name_colour(const PmPlan* p, int n, unsigned prim, unsigned env, unsigned plod, unsigned key, unsigned keys) {
    switch (n) {
    case PM_NM_FC: return fold_colour(p, 0, prim, env, plod);
    case PM_NM_FKP: return fold_colour(p, 1, prim, env, plod);
    case PM_NM_FKN: return fold_colour(p, 2, prim, env, plod);
    case PM_NM_KH: return kh_colour(p, prim, env, plod);
    case PM_NM_X0: case PM_NM_X1: case PM_NM_X2: case PM_NM_X3: return x_colour(p, n - PM_NM_X0, prim, env, plod);
    case PM_NM_KEYC: return key;
    case PM_NM_KEYS: return keys;
    case PM_NM_PK: return pk_colour(prim, key);
    case PM_NM_PRIM: return prim;
    case PM_NM_ENV: return env;
    case PM_NM_PLOD: return plod * 0x01010101u;
    case PM_NM_HALF: return 0x80808080u;
    case PM_NM_K4: return pm_k4 * 0x01010101u;
    default: return 0;
    }
}

void pm_comb_colours(PmPlan* p, unsigned prim, unsigned env, unsigned plod, unsigned key, unsigned keys) {
    for (int i = 0; i < p->n; i++) p->st[i].constant = name_colour(p, p->st[i].cname, prim, env, plod, key, keys);
    p->buf = name_colour(p, p->bufname, prim, env, plod, key, keys);
    p->vtx = (p->vtxname == PM_NM_SHADE) ? 0 : name_colour(p, p->vtxname, prim, env, plod, key, keys);
}

static void unp(unsigned c, float o[4]) {
    o[0] = (float)((c >> 24) & 255u) / 255.f;
    o[1] = (float)((c >> 16) & 255u) / 255.f;
    o[2] = (float)((c >> 8) & 255u) / 255.f;
    o[3] = (float)(c & 255u) / 255.f;
}

static float cl(float v) { return v < 0.f ? 0.f : v > 1.f ? 1.f : v; }

void pm_comb_eval(const PmPlan* p, const float t0[4], const float t1[4], const float shade[4], float out[4]) {
    float prev[4], vtx[4], buf[4], noise[4] = { .5f, .5f, .5f, 1.f }; /* PT07: the audit gives NOISE its mean */
    memcpy(vtx, shade, sizeof vtx);
    if (p->vtxname != PM_NM_SHADE) unp(p->vtx, vtx);
    unp(p->buf, buf);
    memcpy(prev, vtx, sizeof prev);
    for (int i = 0; i < p->n; i++) {
        float cst[4], res[4];
        unp(p->st[i].constant, cst);
        for (int g = 0; g < 2; g++) {
            const PmTevCh* ch = &p->st[i].g[g];
            float in[3][3];
            for (int k = 0; k < 3; k++) {
                const float* sv = ch->src[k] == PM_S_TEX0 ? t0 : ch->src[k] == PM_S_TEX1 ? t1 : ch->src[k] == PM_S_TEX2 ? noise : ch->src[k] == PM_S_VTX ? vtx : ch->src[k] == PM_S_CONST ? cst
                                : ch->src[k] == PM_S_PREV ? prev : buf;
                for (int c = 0; c < 3; c++) {
                    float x;
                    switch (ch->op[k]) {
                    case PM_O_C: x = g ? sv[3] : sv[c]; break;
                    case PM_O_1MC: x = 1.f - (g ? sv[3] : sv[c]); break;
                    case PM_O_A: x = sv[3]; break;
                    default: x = 1.f - sv[3]; break;
                    }
                    in[k][c] = x;
                }
            }
            for (int c = 0; c < (g ? 1 : 3); c++) {
                float a = in[0][c], b = in[1][c], d = in[2][c], r;
                switch (ch->func) {
                case PM_F_REPLACE: r = a; break;
                case PM_F_MODULATE: r = a * b; break;
                case PM_F_ADD: r = a + b; break;
                case PM_F_SUBTRACT: r = a - b; break;
                case PM_F_INTERP: r = a * d + b * (1.f - d); break;
                case PM_F_ADDSIGNED: r = a + b - .5f; break;
                default: r = a * b + d; break;
                }
                res[g ? 3 : c] = cl(r * (ch->scale > 1 ? (float)ch->scale : 1.f));
            }
        }
        memcpy(prev, res, sizeof prev);
    }
    memcpy(out, prev, sizeof prev);
}
