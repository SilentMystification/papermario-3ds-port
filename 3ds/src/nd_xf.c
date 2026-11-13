/* nd_xf.c: the native transform calls (nd_xf.h). Each call records one typed matrix op unit (nd_op.h) into the open NDC_STATE run
 * (nd_rec_state), the unit the nd_gbi.h redirect records for gSPMatrix and gSPPopMatrix, so the frame stream is the one the macro gave.
 * The state run is executed by nd_walk_state with the walker's own matrix code (NDO_MTX: load_mtx, NDO_POPMTX: gbi_popmtx). */
#include <stdint.h>
#include "ultra64.h"
#include "nd_op.h"
#include "nd_cmd.h"
#include "nd_xf.h"

void nd_xf_push(const void* m, unsigned p) {
    nd_rec_state(NDO_W0(NDO_MTX, 0x380000u | ((p ^ 1u) & 0xffu)), (uint32_t)(uintptr_t)m);
}

void nd_xf_pop(unsigned which) {
    (void)which;
    nd_rec_state(NDO_W0(NDO_POPMTX, 0x380002u), 64u);
}
