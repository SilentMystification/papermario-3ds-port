/* nd_xf.c: the native transform calls (nd_xf.h). Each call records one NDC_NAPI record holding one typed op unit (nd_op.h). The runner
 * hands it to nd_walk_state, which runs the unit with the walker's own matrix code (NDO_MTX: load_mtx, NDO_POPMTX: gbi_popmtx). */
#include <stdint.h>
#include "ultra64.h"
#include "nd_op.h"
#include "nd_api.h"
#include "nd_xf.h"

void nd_xf_run(const uint32_t* rec) { nd_walk_state(rec, 1u); }

void nd_xf_push(const void* m, unsigned p) {
    const uint32_t u[2] = { NDO_W0(NDO_MTX, 0x380000u | ((p ^ 1u) & 0xffu)), (uint32_t)(uintptr_t)m };
    nd_rec_napi(nd_xf_run, u, 1u);
}

void nd_xf_pop(unsigned which) {
    (void)which;
    const uint32_t u[2] = { NDO_W0(NDO_POPMTX, 0x380002u), 64u };
    nd_rec_napi(nd_xf_run, u, 1u);
}
