/* nd_xf.c: the native transform calls (nd_xf.h). Each call records one typed matrix op unit (nd_op.h) as one native transform record
 * (nd_rec_xf, NDR5): core 1 applies the unit to nd_ds (nd_ds_unit), core 0 runs the matrix stack (nd_xf_run: the walker's MTX and
 * POPMTX arms: load_mtx, gbi_popmtx). The record replaces the unit in the state run. */
#include <stdint.h>
#include "ultra64.h"
#include "nd_op.h"
#include "nd_cmd.h"
#include "nd_nr.h"
#include "nd_at.h"
#include "nd_xf.h"
#include "nd_api.h"
#include "nd_ds.h"
#include "nd_int.h"
#include "nd_fs.h"

extern unsigned pm_gbi_cur_op; /* nd_walk.c */

/* core 0: op_geo's MTX and POPMTX arms for one record unit */
static void nd_xf_run(const uint32_t* r) {
    const unsigned id = r[0] >> 24;
    pm_gbi_cur_op = id;
    if (id == ND_FS_MTX) load_mtx(r[0] & 0xffffffu, r[1], 0);
    else gbi_popmtx(r[1]);
}

/* core 1: the state and ep rule of the unit (as nd_rec_nr did), then the record */
static void xf_rec(uint32_t w0, uint32_t w1) {
    const uint32_t r[2] = { w0, w1 };
    nd_ds_unit(&nd_ds, w0, w1);
    nd_rec_xf(nd_xf_run, r);
}

void nd_xf_push(const void* m, unsigned p) {
    NDXF_PUSH(xf_rec, m, p);
}

void nd_xf_pop(unsigned which) {
    (void)which;
    NDXF_POP(xf_rec);
}
