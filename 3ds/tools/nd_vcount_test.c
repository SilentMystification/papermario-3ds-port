/* VCOUNT host test: nd_vcount (nd_vcount.h) counts the vertices of a list: n units at most, else up to ND_FS_END. */
#include <stdio.h>
#include <stdint.h>
#include "nd_vcount.h"

#define A 0x00100000u

/* case 0: n 3 stops before the END unit and the VTX unit with the far address (unit 3). */
static const uint32_t c0[] = { ND_NOP_A, 0, ND_OP_VTX_W0(4, 0), A, ND_FS_W0(ND_FS_TRI2, 0), 0, ND_OP_VTX_W0(8, 0), 0x00be0000u, ND_FS_W0(ND_FS_END, 0), 0 };
/* case 1: no bound; the END unit ends the walk. */
static const uint32_t c1[] = { ND_OP_VTX_W0(3, 0), A, ND_OP_VTX_W0(2, 0), A - 64, ND_FS_W0(ND_FS_END, 0), 0 };
/* case 2: END at once. */
static const uint32_t c2[] = { ND_FS_W0(ND_FS_END, 0), 0 };

static int check(const uint32_t* u, unsigned n, unsigned wk, uint32_t wlo, uint32_t whi) {
    uint32_t lo = 0, hi = 0;
    unsigned k = nd_vcount(u, n, &lo, &hi);
    return k == wk && lo == wlo && hi == whi;
}

int main(void) {
    if (!check(c0, 3, 3, A, A + 64)) { printf("vcount FAIL case 0\n"); return 1; }
    if (!check(c1, 0xFFFFFFFFu, 3, A - 32, A + 48)) { printf("vcount FAIL case 1\n"); return 1; }
    if (!check(c2, 0xFFFFFFFFu, 1, 0, 0)) { printf("vcount FAIL case 2\n"); return 1; }
    printf("vcount ok\n");
    return 0;
}
