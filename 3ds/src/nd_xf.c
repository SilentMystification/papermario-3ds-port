/* nd_xf.c: the native transform calls (nd_xf.h). Each call records one typed matrix op unit (nd_op.h) into the open native record run
 * (nd_rec_nr), the unit the nd_gbi.h redirect records for gSPMatrix and gSPPopMatrix, so the frame stream is the one the macro gave.
 * The state run is executed by nd_walk_state with the walker's own matrix code (NDO_MTX: load_mtx, NDO_POPMTX: gbi_popmtx). */
#include <stdint.h>
#include "ultra64.h"
#include "nd_op.h"
#include "nd_cmd.h"
#include "nd_nr.h"
#include "nd_at.h"
#include "nd_xf.h"

void nd_xf_push(const void* m, unsigned p) {
    NDXF_PUSH(nd_nr, m, p);
}

void nd_xf_pop(unsigned which) {
    (void)which;
    NDXF_POP(nd_nr);
}
