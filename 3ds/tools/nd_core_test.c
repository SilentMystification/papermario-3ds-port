/* nd_core_test.c: host test of the S1-2 frame driver (plan_native_slices S1-2, host test "legacy only").
 * A frame of 3 LEGACY records must give 3 walker calls in record order, with the list pointer and the byte count of each. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "nd.h"

/* pm_interp.c stubs (nd_mtx.c links them): no replacement table */
int pm_interp_repl;
const float (*pm_interp_find(const void* m))[4] { (void)m; return 0; }

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)
static uint32_t rng_s = 777u;
static uint32_t rnd(void) { rng_s = rng_s * 1664525u + 1013904223u; return rng_s >> 8; }

/* record layout (plan 2.2): one cache line, field offsets */
static void test_layout(void) {
    CHECK(sizeof(NdRec) == 32, "NdRec size %zu", sizeof(NdRec));
    CHECK(offsetof(NdRec, tmpl) == 1 && offsetof(NdRec, tex0) == 2 && offsetof(NdRec, tex1) == 3 && offsetof(NdRec, mtx) == 4 && offsetof(NdRec, unif) == 6, "NdRec u8 and u16 offsets");
    CHECK(offsetof(NdRec, idx_word) == 8 && offsetof(NdRec, nidx) == 12 && offsetof(NdRec, vbuf) == 14 && offsetof(NdRec, sort) == 16 && offsetof(NdRec, seq) == 20 &&
          offsetof(NdRec, aux) == 24 && offsetof(NdRec, aux2) == 28, "NdRec 32 bit offsets");
}

/* draw order: reference is a stable insertion sort by (tmpl, tex0) inside each run of unordered records. Ordered records keep their index. */
static void test_order(void) {
    for (int trial = 0; trial < 200; trial++) {
        NdRec rec[40]; unsigned n = 1 + rnd() % 40;
        for (unsigned k = 0; k < n; k++) {
            memset(&rec[k], 0, sizeof rec[k]);
            rec[k].tmpl = (uint8_t)(rnd() % 4); rec[k].tex0 = (uint8_t)(rnd() % 6); rec[k].mtx = (uint16_t)k; rec[k].nidx = (uint16_t)(k + 1);
            if (rec[k].tmpl == 2 || ((trial & 1) && rec[k].tmpl == 3)) rec[k].seq = ND_SEQ_ORDERED | k;
        }
        uint16_t ord[ND_REC_MAX]; nd_order(rec, n, ord);
        uint16_t ref[40]; for (unsigned k = 0; k < n; k++) ref[k] = (uint16_t)k;
        unsigned i = 0;
        while (i < n) {
            if (rec[i].seq & ND_SEQ_ORDERED) { i++; continue; }
            unsigned j = i; while (j < n && !(rec[j].seq & ND_SEQ_ORDERED)) j++;
            for (unsigned a = i + 1; a < j; a++) {
                uint16_t v = ref[a]; unsigned b = a;
                while (b > i) {
                    const NdRec* p = &rec[ref[b - 1]]; const NdRec* q = &rec[v];
                    if (p->tmpl < q->tmpl || (p->tmpl == q->tmpl && p->tex0 <= q->tex0)) break;
                    ref[b] = ref[b - 1]; b--;
                }
                ref[b] = v;
            }
            i = j;
        }
        for (unsigned k = 0; k < n; k++) if (ord[k] != ref[k]) { CHECK(0, "order trial %d pos %u: got %u want %u", trial, k, ord[k], ref[k]); break; }
    }
}

/* eye copy: the eye 1 span from nd_eye_copy equals a direct emission for eye 1 (matrix words at the logged places), and only those words differ */
static unsigned emit_span(uint32_t* out, unsigned eye, NdMtxLog* log) {
    unsigned o = 0;
    for (unsigned d = 0; d < 3; d++) {
        for (unsigned k = 0; k < 6; k++) out[o + k] = 0xA0000000u + d * 16 + k; /* template and state words */
        memcpy(out + o + 6, nd_mtx_pool[eye][d], 16 * sizeof(uint32_t));
        log[d].at = o + 6; log[d].slot = (uint16_t)d; log[d].pad = 0;
        o += 6 + 16;
        for (unsigned k = 0; k < 3; k++) out[o++] = 0xB0000000u + d * 8 + k; /* index words and the trigger */
    }
    return o;
}
static void test_eye(void) {
    static uint32_t a[200], b[200], c[200];
    NdMtxLog la[3], lb[3];
    for (unsigned e = 0; e < 2; e++) for (unsigned d = 0; d < 3; d++) for (unsigned k = 0; k < 16; k++) nd_mtx_pool[e][d][k] = 0.f, ((uint32_t*)nd_mtx_pool[e][d])[k] = rnd() | (e << 28);
    unsigned na = emit_span(a, 0, la), nb = emit_span(b, 1, lb);
    nd_eye_copy(c, a, na, la, 3, 1);
    CHECK(na == nb && memcmp(b, c, na * 4) == 0, "eye copy differs from the direct eye 1 span");
    unsigned diff = 0;
    for (unsigned k = 0; k < na; k++) diff += a[k] != c[k];
    CHECK(diff == 48, "differing words %u, want 48", diff);
}

/* walker stub: records each call */
static int calls;
static void* call_list[8];
static unsigned call_bytes[8];
void pm_gbi_run(void* dl, unsigned nbytes) {
    call_list[calls] = dl;
    call_bytes[calls] = nbytes;
    calls++;
}

/* phase hooks: the test runs with phase off, so the tick and the record are not read */
int pm_ph_on;
void pm_pf_nd_add(unsigned long long ticks, unsigned legacy) { (void)ticks; (void)legacy; }
uint64_t svcGetSystemTick(void) { return 0; }

int main(void) {
    static uint32_t a[4], b[8], c[2];
    nd_frame_begin();
    nd_bridge_cut(a, 16);
    nd_bridge_cut(b, 32);
    nd_bridge_cut(c, 8);
    nd_emit();
    assert(calls == 3);
    assert(call_list[0] == a && call_bytes[0] == 16);
    assert(call_list[1] == b && call_bytes[1] == 32);
    assert(call_list[2] == c && call_bytes[2] == 8);
    /* a new frame with no records runs nothing */
    calls = 0;
    nd_frame_begin();
    nd_emit();
    assert(calls == 0);
    puts("nd_core legacy only: PASS");
    test_layout();
    test_order();
    test_eye();
    printf("nd_core order, record layout, eye copy: %s\n", fails ? "FAIL" : "PASS");
    return fails != 0;
}
