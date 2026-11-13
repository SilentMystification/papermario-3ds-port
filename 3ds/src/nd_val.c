/* nd_val.c: the value calls of a call site (nd_val.h). Each call records one NDC_NAPI record holding one typed op unit (nd_op.h, the unit
 * nd_op_xlate gives for the matching F3DEX2 word). The runner hands it to nd_walk_state, the executor of the NDC_STATE runs: skip test, op
 * prologue and setter as the walker has them, so the prim and env colour, the tile size and the segment base reach the next draw as before. */
#include <stdint.h>
#include "ultra64.h"
#include "nd_op.h"
#include "nd_api.h"
#include "nd_val.h"

#define VAL_COL(r, g, b, a) (((uint32_t)(r) & 0xffu) << 24 | ((uint32_t)(g) & 0xffu) << 16 | ((uint32_t)(b) & 0xffu) << 8 | ((uint32_t)(a) & 0xffu))

static void val_rec(uint32_t id, uint32_t a, uint32_t b) {
    const uint32_t u[2] = { NDO_W0(id, a), b };
    nd_rec_napi(nd_val_run, u, 1u);
}
void nd_val_run(const uint32_t* rec) { nd_walk_state(rec, 1u); }

void nd_val_prim(unsigned minlod, unsigned lodfrac, unsigned r, unsigned g, unsigned b, unsigned a) {
    val_rec(NDO_PRIM, ((minlod & 0xffu) << 8) | (lodfrac & 0xffu), VAL_COL(r, g, b, a));
}
void nd_val_env(unsigned r, unsigned g, unsigned b, unsigned a) {
    val_rec(NDO_ENV, 0u, VAL_COL(r, g, b, a));
}
void nd_val_seg(unsigned seg, uint32_t base) {
    /* the typed form of G_MOVEWORD G_MW_SEGMENT at offset seg * 4 (ND_MW_A); a segment above 15 is not a segment op and is dropped like the macro's */
    if (seg < 16u) val_rec(NDO_MW, ((uint32_t)NDO_MW_SEG << 16) | seg, base);
}
void nd_val_tile_size(unsigned tile, unsigned uls, unsigned ult, unsigned lrs, unsigned lrt) {
    val_rec(NDO_TILESIZE, ((uls & 0xfffu) << 12) | (ult & 0xfffu), ((tile & 7u) << 24) | ((lrs & 0xfffu) << 12) | (lrt & 0xfffu));
}
