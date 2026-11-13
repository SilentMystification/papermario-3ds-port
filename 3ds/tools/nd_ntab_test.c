/* CONV2A: host test of the native table form v2 (NdNOp, NdNTab, nd_gtab.h). Prints one line. */
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include "nd_gtab.h"

_Static_assert(sizeof(NdNOp) == 16, "NdNOp size");
_Static_assert(offsetof(NdNOp, v) == 4, "NdNOp v offset");
_Static_assert(offsetof(NdNOp, p) == 8, "NdNOp p offset");
_Static_assert(sizeof(NdNTab) == 16, "NdNTab size");

static const uint16_t fx_idx[] = {0, 1, 2};
static const NdNOp fx_ops[] = {
    {NDN_PSO, 0, 3, 0, NULL, 0},
    {NDN_COL, 0, 0, 0xFF8000FFu, NULL, 0},
    {NDN_DRAW, 0, 3, 0, NULL, 0},
    {NDN_END, 0, 0, 0, NULL, 0},
};
static const NdNTab fx_tab = {NULL, 3, 4, fx_ops, fx_idx};

int main(void) {
    int ops = 0, draws = 0, idx = 0;
    const NdNOp* o = fx_tab.ops;
    for (;;) {
        ops++;
        if (o->op == NDN_END) break;
        if (o->op == NDN_DRAW) {
            draws++;
            idx += o->b;
        }
        o++;
    }
    if (ops != fx_tab.nops || draws != 1 || idx != 3) {
        printf("ntab FAIL ops %d draws %d idx %d\n", ops, draws, idx);
        return 1;
    }
    printf("ntab ok ops %d draws %d idx %d\n", ops, draws, idx);
    return 0;
}
