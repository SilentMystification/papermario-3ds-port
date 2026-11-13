/* nd_vslot_test.c: host test of nd_vslot_resolve (CONV2B). Prints "vslot ok 6/6", or "vslot FAIL <case>" and exits 1. */
#include <stdio.h>
#include <string.h>
#include "nd_vslot.h"

static NdVsOp vtx(uint32_t arr, uint32_t first, unsigned n, unsigned v0)
{
    NdVsOp o;
    memset(&o, 0, sizeof o);
    o.kind = ND_VS_VTX;
    o.arr = arr;
    o.first = first;
    o.n = (uint8_t)n;
    o.v0 = (uint8_t)v0;
    return o;
}

static NdVsOp tri1(unsigned a, unsigned b, unsigned c)
{
    NdVsOp o;
    memset(&o, 0, sizeof o);
    o.kind = ND_VS_TRI1;
    o.s[0] = (uint8_t)a;
    o.s[1] = (uint8_t)b;
    o.s[2] = (uint8_t)c;
    return o;
}

static NdVsOp tri2(unsigned a, unsigned b, unsigned c, unsigned d, unsigned e, unsigned f)
{
    NdVsOp o;
    memset(&o, 0, sizeof o);
    o.kind = ND_VS_TRI2;
    o.s[0] = (uint8_t)a;
    o.s[1] = (uint8_t)b;
    o.s[2] = (uint8_t)c;
    o.s[3] = (uint8_t)d;
    o.s[4] = (uint8_t)e;
    o.s[5] = (uint8_t)f;
    return o;
}

static void print_idx(const char* lbl, const uint16_t* v, unsigned n)
{
    unsigned k;
    printf("%s", lbl);
    for (k = 0; k < n; k++) printf(" %u", (unsigned)v[k]);
    printf("\n");
}

/* Runs one case. want_rc != 0: only the return value is checked. Returns 1 on pass, 0 on fail (after printing). */
static int run_case(const char* name, const NdVsOp* ops, unsigned nops, int want_rc, unsigned want_ndr,
                    const NdVsDraw* want_dr, unsigned want_nidx, const uint16_t* want_idx)
{
    uint16_t idx[64];
    NdVsDraw dr[8];
    unsigned ndr = 0, nidx = 0, k;
    int rc;

    memset(idx, 0, sizeof idx);
    memset(dr, 0, sizeof dr);
    rc = nd_vslot_resolve(ops, nops, idx, 64, dr, 8, &ndr, &nidx);
    if (rc != want_rc) {
        printf("vslot FAIL %s rc %d want %d\n", name, rc, want_rc);
        return 0;
    }
    if (want_rc != 0) return 1;
    if (ndr != want_ndr || nidx != want_nidx) {
        printf("vslot FAIL %s ndr %u nidx %u want ndr %u nidx %u\n", name, ndr, nidx, want_ndr, want_nidx);
        return 0;
    }
    for (k = 0; k < ndr; k++) {
        if (dr[k].arr != want_dr[k].arr || dr[k].idx0 != want_dr[k].idx0 || dr[k].nidx != want_dr[k].nidx) {
            printf("vslot FAIL %s draw %u got {%u, %u, %u} want {%u, %u, %u}\n", name, k, (unsigned)dr[k].arr,
                   (unsigned)dr[k].idx0, (unsigned)dr[k].nidx, (unsigned)want_dr[k].arr, (unsigned)want_dr[k].idx0,
                   (unsigned)want_dr[k].nidx);
            return 0;
        }
    }
    for (k = 0; k < nidx; k++) {
        if (idx[k] != want_idx[k]) {
            printf("vslot FAIL %s\n", name);
            print_idx("idx got  ", idx, nidx);
            print_idx("idx want ", want_idx, want_nidx);
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    int pass = 0;

    /* F1: VTX(7, 0, 4, 0), TRI1(0, 1, 2), TRI2(0, 2, 3, 3, 1, 0) */
    {
        NdVsOp ops[3] = { vtx(7, 0, 4, 0), tri1(0, 1, 2), tri2(0, 2, 3, 3, 1, 0) };
        NdVsDraw d[1] = { { 7, 0, 9 } };
        uint16_t x[9] = { 0, 1, 2, 0, 2, 3, 3, 1, 0 };
        if (!run_case("F1", ops, 3, 0, 1, d, 9, x)) return 1;
        pass++;
    }
    /* F2: VTX(7, 0, 3, 0), TRI1(0, 1, 2), VTX(7, 10, 3, 0), TRI1(2, 1, 0) */
    {
        NdVsOp ops[4] = { vtx(7, 0, 3, 0), tri1(0, 1, 2), vtx(7, 10, 3, 0), tri1(2, 1, 0) };
        NdVsDraw d[1] = { { 7, 0, 6 } };
        uint16_t x[6] = { 0, 1, 2, 12, 11, 10 };
        if (!run_case("F2", ops, 4, 0, 1, d, 6, x)) return 1;
        pass++;
    }
    /* F3: VTX(1, 0, 3, 0), TRI1(0, 1, 2), VTX(2, 5, 3, 0), TRI1(0, 1, 2) */
    {
        NdVsOp ops[4] = { vtx(1, 0, 3, 0), tri1(0, 1, 2), vtx(2, 5, 3, 0), tri1(0, 1, 2) };
        NdVsDraw d[2] = { { 1, 0, 3 }, { 2, 3, 3 } };
        uint16_t x[6] = { 0, 1, 2, 5, 6, 7 };
        if (!run_case("F3", ops, 4, 0, 2, d, 6, x)) return 1;
        pass++;
    }
    /* E1: TRI1(5, 6, 7) alone: slots not loaded */
    {
        NdVsOp ops[1] = { tri1(5, 6, 7) };
        if (!run_case("E1", ops, 1, -1, 0, NULL, 0, NULL)) return 1;
        pass++;
    }
    /* E2: VTX(1, 0, 2, 0), VTX(2, 0, 1, 2), TRI1(0, 1, 2): triangle mixes arrays 1 and 2 */
    {
        NdVsOp ops[3] = { vtx(1, 0, 2, 0), vtx(2, 0, 1, 2), tri1(0, 1, 2) };
        if (!run_case("E2", ops, 3, -3, 0, NULL, 0, NULL)) return 1;
        pass++;
    }
    /* E3: VTX(1, 0, 4, 30): slots 30..33 out of range */
    {
        NdVsOp ops[1] = { vtx(1, 0, 4, 30) };
        if (!run_case("E3", ops, 1, -1, 0, NULL, 0, NULL)) return 1;
        pass++;
    }

    if (pass != 6) {
        printf("vslot FAIL count %d\n", pass);
        return 1;
    }
    printf("vslot ok 6/6\n");
    return 0;
}
