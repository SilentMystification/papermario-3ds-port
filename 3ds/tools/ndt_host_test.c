/* NDT2: host unit tests for the native draw rewrite. No emulator. Each part runs in well under 1 s.
 *   1 combiner: pm_comb_build, pm_comb_colours and pm_comb_eval (pm_comb.c, the TexEnv stage model) against the N64 colour
 *     formula (A - B) * C + D, clamped, selectors and word layout from include/PR/gbi.h (G_CCMUX_*, GCCc0w0 .. GCCc1w1).
 *     Known difference: the PICA clamps A - B at 0 (pm_comb.c header), so a sample with A < B is counted apart. Exit 1 when a
 *     sample with A >= B differs, or when a built combine misses the reference.
 *   2 matrix: fp32 4 x 4 product (one order, as the seam rules in design_native_draw.md section 5). Repeated products are
 *     bit-equal. The reversed order differs, which shows that the order is observable.
 *   3 fp24 dp4 seam emulation: fp24 (s1 e7 m16, round to nearest even) after each multiply and each add. Shared-edge s16
 *     vertices, two matrices with equal value bits must give bit-equal output. Two products of one transform (two orders)
 *     give the count of differing shared-edge results (the seam risk that one product per model removes).
 *   4 record packing: sizeof and offsetof of RecKey (3ds/src/pm_gbi.c:3476-3480, copied as text). The size and alignment
 *     must match the design note (96 B, aligned 32).
 * Build and run: sh 3ds/tools/ndt_host_test.sh. Exit 0 when all gates pass. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include "pm_gpu.h"

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;

/* ---- deterministic random numbers ---- */
static u32 rng_s = 12345u;
static u32 rnd(void) { rng_s = rng_s * 1664525u + 1013904223u; return rng_s >> 8; }
static float rndf(void) { return (float)(rnd() & 0xffffff) / 16777216.f; }

/* ================= 1 combiner ================= */
/* gbi.h GCCc0w0 / GCCc0w1 / GCCc1w0 / GCCc1w1 packing of the first (a0 .. d0) and second (a1 .. d1) cycle */
static void pack_comb(unsigned* w0, unsigned* w1,
    unsigned a0, unsigned b0, unsigned c0, unsigned d0, unsigned Aa0, unsigned Ab0, unsigned Ac0, unsigned Ad0,
    unsigned a1, unsigned b1, unsigned c1, unsigned d1, unsigned Aa1, unsigned Ab1, unsigned Ac1, unsigned Ad1) {
    *w0 = (0xfcu << 24) | (a0 & 15) << 20 | (c0 & 31) << 15 | (Aa0 & 7) << 12 | (Ac0 & 7) << 9 | (a1 & 15) << 5 | (c1 & 31);
    *w1 = (b0 & 15) << 28 | (d0 & 7) << 15 | (Ab0 & 7) << 12 | (Ad0 & 7) << 9 | (b1 & 15) << 24 | (Aa1 & 7) << 21 |
          (Ac1 & 7) << 18 | (d1 & 7) << 6 | (Ab1 & 7) << 3 | (Ad1 & 7);
}
/* reference inputs for one sample (RGBA 0..1) */
typedef struct { float t0[4], t1[4], sh[4], prim[4], env[4]; float plod, k4; } Inp;
static float sel_rgb_c(unsigned s, const Inp* in, int ch) {
    /* C slot (gbi.h G_CCMUX_*_ALPHA = 8..12, PRIM_LOD_FRAC = 14): the alpha of the source */
    switch (s) {
    case 8: return in->t0[3]; case 9: return in->t1[3]; case 10: return in->prim[3]; case 11: return in->sh[3];
    case 12: return in->env[3]; case 14: return in->plod; default: break;
    }
    (void)ch;
    return 0.f;
}
static float sel_rgb_chan(unsigned s, const Inp* in, int ch) {
    const float* t0 = in->t0; const float* t1 = in->t1; const float* sh = in->sh; const float* pr = in->prim; const float* en = in->env;
    switch (s) {
    case 1: return t0[ch]; case 2: return t1[ch]; case 3: return pr[ch]; case 4: return sh[ch]; case 5: return en[ch];
    case 6: return 1.f; case 7: return in->k4; default: return 0.f;
    }
}
static float sel_alpha(unsigned s, const Inp* in) {
    switch (s) {
    case 1: return in->t0[3]; case 2: return in->t1[3]; case 3: return in->prim[3]; case 4: return in->sh[3];
    case 5: return in->env[3]; case 6: return 1.f; default: return 0.f;
    }
}
static float clampf(float v) { return v < 0.f ? 0.f : v > 1.f ? 1.f : v; }
/* the N64 cycle formula on one channel: (A - B) * C + D, no clamp inside, clamp at the end */
static float rdp(float a, float b, float c, float d) { return clampf((a - b) * c + d); }

static int test_combiner(void) {
    static int hist_a[8], hist_b[8], hist_c[16], hist_d[8], hist_aa[8];
    static const unsigned Asel[] = { 0, 1, 3, 4, 5, 6 }, Bsel[] = { 0, 1, 3, 4, 5, 7 }, Csel[] = { 8, 9, 10, 11, 12, 14 }, Dsel[] = { 0, 1, 3, 4, 5, 6 };
    static const unsigned AAsel[] = { 0, 1, 3, 4, 5, 6 }, ACsel[] = { 1, 3, 4, 5 };
    int built = 0, refused = 0, cmp = 0, known = 0, bad = 0;
    float maxe_other = 0.f; /* largest difference on a sample with A >= B, in 8 bit units */
    unsigned long n = 0;
    for (n = 0; n < 20000; n++) {
        unsigned a = Asel[rnd() % 6], b = Bsel[rnd() % 6], c = Csel[rnd() % 6], d = Dsel[rnd() % 6];
        unsigned Aa = AAsel[rnd() % 6], Ab = AAsel[rnd() % 6], Ac = ACsel[rnd() % 4], Ad = AAsel[rnd() % 6];
        unsigned w0, w1; PmPlan p;
        Inp in;
        int k;
        pack_comb(&w0, &w1, a, b, c, d, Aa, Ab, Ac, Ad, 0, 0, 0, 0, 0, 0, 0, 0);
        if (pm_comb_build(w0, w1, 0, 0, &p) != 0) { refused++; continue; }
        built++;
        for (k = 0; k < 4; k++) { in.t0[k] = rndf(); in.t1[k] = rndf(); in.sh[k] = rndf(); in.prim[k] = rndf(); in.env[k] = rndf(); }
        in.plod = rndf(); in.k4 = rndf();
        {
            unsigned prim = 0, env = 0, plod = (unsigned)(in.plod * 255.f) & 255u;
            for (k = 0; k < 4; k++) {
                prim |= (unsigned)(in.prim[k] * 255.f + .5f) << (24 - 8 * k);
                env |= (unsigned)(in.env[k] * 255.f + .5f) << (24 - 8 * k);
            }
            /* the reference reads the same quantized values the constants hold */
            for (k = 0; k < 4; k++) { in.prim[k] = ((prim >> (24 - 8 * k)) & 255u) / 255.f; in.env[k] = ((env >> (24 - 8 * k)) & 255u) / 255.f; }
            in.plod = plod / 255.f;
            pm_comb_set_k4((unsigned)(in.k4 * 255.f + .5f));
            in.k4 = (float)((unsigned)(in.k4 * 255.f + .5f) & 255u) / 255.f;
            pm_comb_colours(&p, prim, env, plod, 0, 0);
        }
        {
            float out[4];
            float t0[4], t1[4], sh[4];
            memcpy(t0, in.t0, sizeof t0); memcpy(t1, in.t1, sizeof t1); memcpy(sh, in.sh, sizeof sh);
            pm_comb_eval(&p, t0, t1, sh, out);
            /* reference per channel, colour uses the RGB selectors, alpha the alpha selectors */
            float ref[4]; int neg = 0;
            for (k = 0; k < 3; k++) {
                float A = sel_rgb_chan(a, &in, k), B = sel_rgb_chan(b, &in, k);
                float C = c == 14 ? in.plod : sel_rgb_c(c, &in, k);
                float D = sel_rgb_chan(d, &in, k);
                if (A < B) neg = 1;
                ref[k] = rdp(A, B, C, D);
            }
            {
                float A = sel_alpha(Aa, &in), B = sel_alpha(Ab, &in), C = sel_alpha(Ac, &in), D = sel_alpha(Ad, &in);
                if (A < B) neg = 1;
                ref[3] = rdp(A, B, C, D);
            }
            cmp++;
            {
                int diff = 0;
                for (k = 0; k < 4; k++) { float e = fabsf(out[k] - ref[k]); if (e > maxe_other && !neg) maxe_other = e; if (e > 2.f / 255.f) diff = 1; }
                if (diff) { if (neg) known++; else { bad++; hist_a[a]++; hist_b[b]++; hist_c[c]++; hist_d[d]++; hist_aa[Aa]++; if (bad <= 3) printf("  comb mismatch a%u b%u c%u d%u Aa%u Ab%u Ac%u Ad%u out %.4f %.4f %.4f %.4f ref %.4f %.4f %.4f %.4f\n", a, b, c, d, Aa, Ab, Ac, Ad, out[0], out[1], out[2], out[3], ref[0], ref[1], ref[2], ref[3]); } }
            }
        }
    }
    { int q; printf("  other-differs by A:"); for (q = 0; q < 8; q++) printf(" %d", hist_a[q]); printf("\n  by B:"); for (q = 0; q < 8; q++) printf(" %d", hist_b[q]); printf("\n  by C:"); for (q = 0; q < 16; q++) printf(" %d", hist_c[q]); printf("\n  by D:"); for (q = 0; q < 8; q++) printf(" %d", hist_d[q]); printf("\n  by Aa:"); for (q = 0; q < 8; q++) printf(" %d", hist_aa[q]); printf("\n"); }
    printf("combiner: max difference with A >= B %.2f LSB (1 LSB = 1/255)\n", maxe_other * 255.f);
    printf("combiner: %lu samples, built %d, refused %d, compared %d, differs with A<B (known PICA clamp) %d, other differs %d\n",
        n, built, refused, cmp, known, bad);
    return bad == 0 && built > 0 && maxe_other * 255.f <= 2.f; /* gate: 2 LSB (the 8 bit output and one stage rounding) */
}

/* ================= 2 matrix ================= */
static void mat_mul(float o[4][4], const float a[4][4], const float b[4][4]) {
    int i, j, k;
    for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) {
        float s = 0.f;
        for (k = 0; k < 4; k++) s += a[i][k] * b[k][j];
        o[i][j] = s;
    }
}
static void mat_rand(float m[4][4]) { int i, j; for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) m[i][j] = rndf() * 2.f - 1.f; }

static int test_matrix(void) {
    float P[4][4], G[4][4], A[4][4], B[4][4], R[4][4];
    int t, diffs = 0, nonzero_rev = 0, det_bad = 0;
    for (t = 0; t < 2000; t++) {
        mat_rand(P); mat_rand(G);
        mat_mul(A, P, G);
        mat_mul(B, P, G);
        if (memcmp(A, B, sizeof A) != 0) det_bad++;
        mat_mul(R, G, P);
        if (memcmp(A, R, sizeof A) != 0) nonzero_rev++;
        diffs += memcmp(A, R, sizeof A) != 0;
    }
    printf("matrix: 2000 products, repeated product bit-equal %s (%d differ), reversed order differs in %d\n",
        det_bad == 0 ? "yes" : "NO", det_bad, nonzero_rev);
    return det_bad == 0 && diffs > 0;
}

/* ================= 3 fp24 dp4 ================= */
/* fp24: sign, 7 bit exponent, 16 bit mantissa. Round to nearest even on the float32 mantissa (drop 7 bits). */
static float fp24(float x) {
    union { float f; u32 u; } v; u32 lsb;
    v.f = x;
    if ((v.u & 0x7fffffffu) == 0) return x;
    lsb = (v.u >> 7) & 1u;
    v.u += 0x3fu + lsb;
    v.u &= ~0x7fu;
    return v.f;
}
static float dp4_fp24(const float row[4], const float v[4]) {
    float acc = 0.f; int k;
    for (k = 0; k < 4; k++) acc = fp24(acc + fp24(fp24(row[k]) * fp24(v[k])));
    return acc;
}
static int test_fp24(void) {
    float G[4][4], P[4][4], MA[4][4], MB[4][4], MR[4][4];
    int t, i, bitdiff_same = 0, seam_diff = 0, n_edge = 0;
    for (t = 0; t < 4000; t++) {
        float v1[4], v2[4]; /* two vertices of a shared edge, s16 integers */
        float o1a, o1b, o2a, o2b;
        int r;
        mat_rand(P); mat_rand(G);
        mat_mul(MA, P, G);
        memcpy(MB, MA, sizeof MA);
        for (i = 0; i < 4; i++) { v1[i] = (float)((int)(rnd() % 65536) - 32768); v2[i] = (float)((int)(rnd() % 65536) - 32768); }
        v1[3] = v2[3] = 1.f;
        for (r = 0; r < 4; r++) {
            o1a = dp4_fp24(MA[r], v1); o1b = dp4_fp24(MB[r], v1);
            if (memcmp(&o1a, &o1b, 4) != 0) bitdiff_same++;
            o2a = dp4_fp24(MA[r], v2); o2b = dp4_fp24(MB[r], v2);
            if (memcmp(&o2a, &o2b, 4) != 0) bitdiff_same++;
            n_edge++;
        }
        /* the seam risk: the same transform as two products in two orders */
        mat_mul(MR, G, P);
        for (r = 0; r < 4; r++) {
            if (dp4_fp24(MA[r], v1) != dp4_fp24(MR[r], v1)) seam_diff++;
            if (dp4_fp24(MA[r], v2) != dp4_fp24(MR[r], v2)) seam_diff++;
        }
    }
    printf("fp24 dp4: %d shared-edge results, equal matrix bits differ %d (must be 0), two product orders differ %d\n",
        n_edge, bitdiff_same, seam_diff);
    return bitdiff_same == 0;
}

/* ================= 4 record packing ================= */
/* copied from 3ds/src/pm_gbi.c:3476-3480 (the fields after the UC2 group words are checked by offset) */
typedef struct { u32 ver; u32 gen; u16 hgen; u16 tgen0; u16 tgen1; u16 off; u16 len; u8 state; u8 nb; u16 s0; u16 s1; u32 grp[7];
    u32 io, ic;
    u32 gin, gout, view; u16 alen; u8 npt, nd, fl;
    u32 gmc, gdl;
} __attribute__((aligned(32))) RecKey;
static int test_record(void) {
    int ok = sizeof(RecKey) == 96 && _Alignof(RecKey) == 32 && offsetof(RecKey, grp) == 24 && offsetof(RecKey, gmc) == 80;
    printf("record: sizeof RecKey %zu, align %zu, grp at %zu, gmc at %zu (design note: 96 B, aligned 32): %s\n",
        sizeof(RecKey), _Alignof(RecKey), offsetof(RecKey, grp), offsetof(RecKey, gmc), ok ? "PASS" : "FAIL");
    return ok;
}

int main(void) {
    int r1 = test_combiner();
    int r2 = test_matrix();
    int r3 = test_fp24();
    int r4 = test_record();
    printf("NDT2 host tests: combiner %s, matrix %s, fp24 %s, record %s\n",
        r1 ? "PASS" : "FAIL", r2 ? "PASS" : "FAIL", r3 ? "PASS" : "FAIL", r4 ? "PASS" : "FAIL");
    return r1 && r2 && r3 && r4 ? 0 : 1;
}
