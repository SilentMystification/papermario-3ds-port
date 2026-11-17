/* VCOPY: the device entry of a local vertex copy list (src/model.c mdl_make_local_vertex_copy). The list is typed units (the copy of the
 * model node list, vertex loads moved to the copied vertices). nu = the units before NDO_END, ntri = the triangle count (not zero, so the
 * walker takes ent_gdl for the heap list as walk_call did; dlc_try refuses a heap list in both paths). */
#ifndef ND_VCOPY_H
#define ND_VCOPY_H
#include <stdint.h>
#include "nd_op.h"
#include "nd_gtab.h"
static inline void nd_vcopy_dev(NdNDev* d, const void* buf) {
    const uint32_t* u = (const uint32_t*)buf;
    unsigned ntri = 0;
    unsigned i = 0;
    for (;; i++) {
        const uint32_t op = u[2u * i] >> 24;
        if (op == NDO_END) {
            break;
        }
        if (op == NDO_TRI1) {
            ntri += 1;
        } else if (op == NDO_TRI2) {
            ntri += 2;
        }
    }
    d->list = buf;
    d->fl = 0;
    d->u = u;
    d->nu = i;
    d->ntri = ntri;
}
#endif
