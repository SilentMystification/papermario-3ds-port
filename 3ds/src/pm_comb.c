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

enum { TN_ZERO, TN_ONE, TN_T0, TN_SH, TN_PR, TN_EN, TN_CMB, TN_PLOD, TN_PRV, TN_T1 };
typedef struct { unsigned char n, a; } Term; /* a: 1 = the alpha part of the source */
typedef struct { unsigned char func; Term t[3]; unsigned char inv[3]; } Sk;

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
    case 6: return (slot == 0 || slot == 3) ? TM(TN_ONE, 0) : TM(TN_ZERO, 0);
    default: break;
    }
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

/* Lower one group. Returns the stage count (1 or 2), or a negative reason. */
static int lower(Term A, Term B, Term C, Term D, Sk out[2]) {
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
    if (iso(A)) {
        if (isz(D)) out[0] = mk(PM_F_MODULATE, B, C, C, 1, 0, 0);
        else out[0] = mk(PM_F_MULADD, B, C, D, 1, 0, 0);
        return 1;
    }
    if (teq(B, D)) { out[0] = mk(PM_F_INTERP, A, B, C, 0, 0, 0); return 1; }
    if (isz(A)) {
        if (isz(D)) { out[0] = mk(PM_F_REPLACE, z, z, z, 0, 0, 0); return 1; }
        if (D.n == TN_CMB) return -3;
        out[0] = mk(PM_F_MODULATE, B, C, C, 0, 0, 0);
        out[1] = mk(PM_F_SUBTRACT, D, prv, prv, 0, 0, 0);
        return 2;
    }
    if (C.n == TN_CMB || D.n == TN_CMB) return -3;
    out[0] = mk(PM_F_SUBTRACT, A, B, B, 0, 0, 0);
    if (isz(D)) out[1] = mk(PM_F_MODULATE, prv, C, C, 0, 0, 0);
    else out[1] = mk(PM_F_MULADD, prv, C, D, 0, 0, 0);
    return 2;
}

typedef struct { Sk s[6]; int n; } Seq;

static int add_cycle(Seq* r, Seq* a, int start_r, int start_a, Term v[2][4]) {
    Sk o[2];
    int n;
    n = lower(v[0][0], v[0][1], v[0][2], v[0][3], o);
    if (n < 0) return n;
    if (start_r + n > 6) return -1;
    for (int i = 0; i < n; i++) r->s[start_r + i] = o[i];
    r->n = start_r + n;
    n = lower(v[1][0], v[1][1], v[1][2], v[1][3], o);
    if (n < 0) return n;
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
    case TN_EN: name = PM_NM_ENV; break;
    case TN_PLOD: name = PM_NM_PLOD; break;
    default: return 5;
    }
    if (name) {
        if (al->stage_name[stage] == name) *src = PM_S_CONST;
        else if (al->buf == name) *src = PM_S_BUF;
        else if (al->vtx == name) *src = PM_S_VTX;
        else if (al->stage_name[stage] == PM_NM_NONE) { al->stage_name[stage] = (unsigned char)name; *src = PM_S_CONST; }
        else if (al->buf == PM_NM_NONE) { al->buf = (unsigned char)name; *src = PM_S_BUF; }
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

int pm_comb_build(unsigned w0, unsigned w1, int two, int t1only, PmPlan* out) {
    Term v[2][2][4]; /* cycle, group, A B C D */
    unsigned s[2][8];
    Seq R, A;
    Alloc al;
    int rc, nst, start;
    memset(out, 0, sizeof *out);
    memset(&R, 0, sizeof R);
    memset(&A, 0, sizeof A);
    memset(&al, 0, sizeof al);
    s[0][0] = (w0 >> 20) & 0xf; s[0][1] = (w1 >> 28) & 0xf; s[0][2] = (w0 >> 15) & 0x1f; s[0][3] = (w1 >> 15) & 7;
    s[0][4] = (w0 >> 12) & 7;   s[0][5] = (w1 >> 12) & 7;   s[0][6] = (w0 >> 9) & 7;      s[0][7] = (w1 >> 9) & 7;
    s[1][0] = (w0 >> 5) & 0xf;  s[1][1] = (w1 >> 24) & 0xf; s[1][2] = w0 & 0x1f;          s[1][3] = (w1 >> 6) & 7;
    s[1][4] = (w1 >> 21) & 7;   s[1][5] = (w1 >> 3) & 7;    s[1][6] = (w1 >> 18) & 7;     s[1][7] = w1 & 7;
    for (int c = 0; c < (two ? 2 : 1); c++) {
        for (int i = 0; i < 4; i++) {
            v[c][0][i] = term_rgb(i, s[c][i], c, t1only);
            v[c][1][i] = term_a(i, s[c][4 + i], c, t1only);
        }
    }
    rc = add_cycle(&R, &A, 0, 0, v[0]);
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
            for (int k = 0; k < 3; k++) {
                int e = src_of(q[g]->s[i].t[k], q[g]->s[i].inv[k], g, i, &al, &ch->src[k], &ch->op[k], out);
                if (e) return e;
            }
        }
        out->st[i].cname = al.stage_name[i];
    }
    out->bufname = al.buf;
    out->vtxname = al.vtx;
    return 0;
}

static unsigned name_colour(int n, unsigned prim, unsigned env, unsigned plod) {
    switch (n) {
    case PM_NM_PRIM: return prim;
    case PM_NM_ENV: return env;
    case PM_NM_PLOD: return plod * 0x01010101u;
    default: return 0;
    }
}

void pm_comb_colours(PmPlan* p, unsigned prim, unsigned env, unsigned plod) {
    for (int i = 0; i < p->n; i++) p->st[i].constant = name_colour(p->st[i].cname, prim, env, plod);
    p->buf = name_colour(p->bufname, prim, env, plod);
    p->vtx = (p->vtxname == PM_NM_SHADE) ? 0 : name_colour(p->vtxname, prim, env, plod);
}

static void unp(unsigned c, float o[4]) {
    o[0] = (float)((c >> 24) & 255u) / 255.f;
    o[1] = (float)((c >> 16) & 255u) / 255.f;
    o[2] = (float)((c >> 8) & 255u) / 255.f;
    o[3] = (float)(c & 255u) / 255.f;
}

static float cl(float v) { return v < 0.f ? 0.f : v > 1.f ? 1.f : v; }

void pm_comb_eval(const PmPlan* p, const float t0[4], const float t1[4], const float shade[4], float out[4]) {
    float prev[4], vtx[4], buf[4];
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
                const float* sv = ch->src[k] == PM_S_TEX0 ? t0 : ch->src[k] == PM_S_TEX1 ? t1 : ch->src[k] == PM_S_VTX ? vtx : ch->src[k] == PM_S_CONST ? cst
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
                default: r = a * b + d; break;
                }
                res[g ? 3 : c] = cl(r);
            }
        }
        memcpy(prev, res, sizeof prev);
    }
    memcpy(out, prev, sizeof prev);
}
