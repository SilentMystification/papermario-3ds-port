/* NDS1 phase A host tests for 3ds/src/pm_native.c. No emulator. Runs well under 1 s.
 *   1 record: sizeof and offsets of NatRec (16 bytes).
 *   2 template: nat_tmpl_add for the nine combine pairs used on Toad Town and census_yard (NDS1 step 1 dump). The plan
 *     stored in the table equals a direct pm_comb_build of the same pair, the same key gives the same id, a second key
 *     with another flag gets a new id.
 *   3 order: nat_order equals a reference stable sort by (tmpl, tex0) inside each run of unordered records. Ordered records keep their index.
 *   4 emit and eye copy: nat_emit for eye 0 equals a reference emitter. The eye 2 span made by nat_eye_copy from the eye 0
 *     span equals a direct nat_emit for eye 1, bit for bit. Words that differ between the eyes are only MTX patch words.
 * Build and run: sh 3ds/tools/nat_host_test.sh. Exit 0 when all gates pass. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "pm_native.h"

typedef uint32_t u32; typedef uint16_t u16; typedef uint8_t u8;
static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static u32 rng_s = 777u;
static u32 rnd(void) { rng_s = rng_s * 1664525u + 1013904223u; return rng_s >> 8; }

/* 1 record packing */
static void test_record(void) {
    CHECK(sizeof(NatRec) == 16, "NatRec size %zu, want 16", sizeof(NatRec));
    CHECK(offsetof(NatRec, tex0) == 1 && offsetof(NatRec, mtx) == 4 && offsetof(NatRec, nidx) == 6, "NatRec 16-bit offsets");
    CHECK(offsetof(NatRec, idx_off) == 8 && offsetof(NatRec, vtx_base) == 12, "NatRec 32-bit offsets");
    CHECK(offsetof(NatTmpl, blk) % 32 == 0, "NatTmpl block not 32-byte aligned");
    printf("record: sizeof NatRec %zu, NatTmpl %zu\n", sizeof(NatRec), sizeof(NatTmpl));
}

/* 2 template build: the nine pairs of the census (cw0, cw1) */
static const u32 census_pairs[9][2] = {
    {0xfc121824u, 0xff33ffffu}, {0xfcffffffu, 0xfffcf87cu}, {0xfc127e24u, 0xfffff3f9u},
    {0xfc26a004u, 0x1f1093ffu}, {0xfcffffffu, 0xfffcf279u}, {0xfcffb3ffu, 0xff2fffffu},
    {0xfcffffffu, 0xfffe793cu}, {0xfc20a204u, 0xff13ffffu}, {0xfc119623u, 0xff2fffffu},
};
static void test_template(void) {
    static NatTmpl save[NAT_TMPL_MAX];
    memset(nat_tmpl, 0, sizeof nat_tmpl);
    int ids[9], bad = 0, rc_bad = 0;
    for (int i = 0; i < 9; i++) {
        PmPlan a, b; int rc = -1;
        ids[i] = nat_tmpl_add(census_pairs[i][0], census_pairs[i][1], 0x00200405u, 0x0008ac00u, 0x00553078u, 1, 0, 0, 0, 0, &a, &rc);
        if (ids[i] < 0) { bad++; continue; }
        int rc2 = pm_comb_build(census_pairs[i][0], census_pairs[i][1], 0, 0, &b);
        if (rc2 != rc) rc_bad++;
        if (memcmp(&a, &b, sizeof a) != 0) bad++;
        printf("template: pair %08x %08x -> id %d, pm_comb rc %d\n", census_pairs[i][0], census_pairs[i][1], ids[i], rc);
    }
    CHECK(bad == 0, "template plan differs from pm_comb_build for %d pairs", bad);
    CHECK(rc_bad == 0, "pm_comb return code differs for %d pairs", rc_bad);
    for (int i = 0; i < 9; i++) {
        int again = nat_tmpl_add(census_pairs[i][0], census_pairs[i][1], 0x00200405u, 0x0008ac00u, 0x00553078u, 1, 0, 0, 0, 0, NULL, NULL);
        CHECK(again == ids[i], "same key gave id %d, want %d", again, ids[i]);
    }
    int blend_id = nat_tmpl_add(census_pairs[0][0], census_pairs[0][1], 0x00200405u, 0x0008ac00u, 0x00553078u, 1, 0, 1, 0, 0, NULL, NULL);
    CHECK(blend_id >= 0 && blend_id != ids[0], "blend flag gave the same id");
    CHECK(nat_tmpl[blend_id].ordered == 1 && nat_tmpl[ids[0]].ordered == 0, "ordered flag: blend 1, opaque 0");
    memcpy(save, nat_tmpl, sizeof nat_tmpl);
    /* a full table returns -1 */
    int full = 0;
    for (u32 k = 0; k < 100; k++) if (nat_tmpl_add(0xfc000000u + k, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL) < 0) full++;
    CHECK(full > 0, "table did not report full");
    memcpy(nat_tmpl, save, sizeof nat_tmpl);
    printf("template: %d pairs checked, table full at %d extra adds\n", 9, full);
}

/* 3 draw order: reference is a stable insertion sort by (tmpl, tex0) inside each run of unordered records */
static void test_order(void) {
    for (int trial = 0; trial < 200; trial++) {
        NatRec rec[40]; unsigned n = 1 + rnd() % 40;
        memset(nat_tmpl, 0, sizeof nat_tmpl);
        for (int t = 0; t < 4; t++) { nat_tmpl[t].used = 1; nat_tmpl[t].ordered = (t == 2 || (trial & 1 && t == 3)); }
        for (unsigned k = 0; k < n; k++) {
            memset(&rec[k], 0, sizeof rec[k]);
            rec[k].tmpl = rnd() % 4; rec[k].tex0 = rnd() % 6; rec[k].mtx = k; rec[k].nidx = k + 1;
        }
        u16 ord[NAT_REC_MAX]; nat_order(rec, n, ord);
        /* reference */
        u16 ref[40]; for (unsigned k = 0; k < n; k++) ref[k] = (u16)k;
        unsigned i = 0;
        while (i < n) {
            if (nat_tmpl[rec[i].tmpl].ordered) { i++; continue; }
            unsigned j = i; while (j < n && !nat_tmpl[rec[j].tmpl].ordered) j++;
            for (unsigned a = i + 1; a < j; a++) {
                u16 v = ref[a]; unsigned b = a;
                while (b > i) {
                    const NatRec* p = &rec[ref[b - 1]]; const NatRec* q = &rec[v];
                    if (p->tmpl < q->tmpl || (p->tmpl == q->tmpl && p->tex0 <= q->tex0)) break;
                    ref[b] = ref[b - 1]; b--;
                }
                ref[b] = v;
            }
            i = j;
        }
        for (unsigned k = 0; k < n; k++) {
            if (ord[k] != ref[k]) { CHECK(0, "order trial %d pos %u: got %u want %u", trial, k, ord[k], ref[k]); break; }
        }
    }
    printf("order: 200 random sets checked against the reference\n");
}

/* 4 emit and eye copy */
static void emit_ref(const NatRec* rec, unsigned n, const u16* ord, u32* out, unsigned eye, u32* nmt, unsigned* nw) {
    unsigned o = 0; int cur = -1; *nmt = 0;
    for (unsigned k = 0; k < n; k++) {
        const NatRec* r = &rec[ord[k]];
        if (r->tmpl != cur) { memcpy(out + o, nat_tmpl[r->tmpl].blk, NAT_TMPL_WORDS * 4); o += NAT_TMPL_WORDS; cur = r->tmpl; }
        u32* d = out + o; memcpy(d, nat_drw, NAT_DRAW_WORDS * 4);
        d[3] = nat_tex_word[r->tex0]; d[4] = nat_tex_word[r->tex1]; d[5] = nat_idx_base + r->idx_off; d[6] = r->nidx;
        memcpy(d + 8, nat_mtx[eye][r->mtx], 64); (*nmt)++;
        o += NAT_DRAW_WORDS;
    }
    *nw = o;
}
static void test_emit(void) {
    static NatRec rec[NAT_REC_MAX]; static u16 ord[NAT_REC_MAX];
    static u32 out0[8192], out1[8192], ref0[8192], copy1[8192];
    static NatMtxLog log0[NAT_REC_MAX];
    NatPatch pt[5] = { {3, NAT_P_TEX0, 0}, {4, NAT_P_TEX1, 0}, {5, NAT_P_IDX, 0}, {6, NAT_P_NIDX, 0}, {8, NAT_P_MTX, 0} };
    u32 drw[NAT_DRAW_WORDS];
    for (int i = 0; i < NAT_DRAW_WORDS; i++) drw[i] = 0xA5A50000u + i;
    nat_layout_set(pt, 5, drw);
    memset(nat_tmpl, 0, sizeof nat_tmpl);
    for (int t = 0; t < 4; t++) {
        nat_tmpl[t].used = 1; nat_tmpl[t].ordered = (t == 2);
        for (int i = 0; i < NAT_TMPL_WORDS; i++) nat_tmpl[t].blk[i] = 0x10000u + t * 1000u + i;
    }
    for (int i = 0; i < NAT_TEX_MAX; i++) nat_tex_word[i] = 0x20000u + i * 8u;
    nat_idx_base = 0x08000000u;
    for (int e = 0; e < 2; e++) for (int s = 0; s < NAT_MTX_MAX; s++) for (int j = 0; j < 16; j++) nat_mtx[e][s][j] = rnd() * 2654435761u;
    unsigned n = 60;
    for (unsigned k = 0; k < n; k++) {
        memset(&rec[k], 0, sizeof rec[k]);
        rec[k].tmpl = rnd() % 4; rec[k].tex0 = rnd() % 20; rec[k].tex1 = rnd() % 3; rec[k].mtx = rnd() % NAT_MTX_MAX;
        rec[k].nidx = 3 + rnd() % 300; rec[k].idx_off = (rnd() % 4096) * 2;
    }
    nat_order(rec, n, ord);
    unsigned nlog = 0;
    unsigned w0 = nat_emit(rec, n, ord, out0, 8192, 0, log0, &nlog);
    unsigned rw; u32 rnm;
    emit_ref(rec, n, ord, ref0, 0, &rnm, &rw);
    CHECK(w0 == rw, "eye 0 word count %u, reference %u", w0, rw);
    CHECK(memcmp(out0, ref0, w0 * 4) == 0, "eye 0 span differs from the reference emitter");
    CHECK(nlog == n, "MTX log count %u, want %u", nlog, n);
    /* eye 2: copy of the eye 0 span, patched at the logged MTX positions */
    static NatMtxLog log1[NAT_REC_MAX];
    unsigned nlog1 = 0;
    unsigned w1 = nat_emit(rec, n, ord, out1, 8192, 1, log1, &nlog1);
    CHECK(nlog1 == nlog, "eye 1 MTX log count %u, want %u", nlog1, nlog);
    nat_eye_copy(copy1, out0, w0, log0, nlog, 1);
    CHECK(w1 == w0, "eye 1 word count differs");
    CHECK(memcmp(copy1, out1, w1 * 4) == 0, "eye copy with patch differs from direct eye 1 emission");
    /* the words that differ between the eyes must sit inside the MTX patches */
    unsigned diff = 0, outside = 0;
    for (unsigned k = 0; k < w0; k++) {
        if (out0[k] == out1[k]) continue;
        diff++;
        int inside = 0;
        for (unsigned m = 0; m < nlog; m++) if (k >= log0[m].at && k < log0[m].at + 16) { inside = 1; break; }
        if (!inside) outside++;
    }
    CHECK(outside == 0, "%u differing words outside the MTX patches", outside);
    CHECK(diff == 16 * n, "differing words %u, want %u", diff, 16 * n);
    /* a span that is too small returns 0 */
    CHECK(nat_emit(rec, n, ord, out1, 100, 0, log0, &nlog) == 0, "small output did not return 0");
    printf("emit: %u draws, span %u words, MTX patches %u, eye 2 words patched %u\n", n, w0, nlog, diff);
}

int main(void) {
    test_record();
    test_template();
    test_order();
    test_emit();
    if (fails) { printf("NAT HOST TESTS: %d FAIL\n", fails); return 1; }
    printf("NAT HOST TESTS: all pass\n");
    return 0;
}
