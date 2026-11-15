/* LKEY: the typed units a list reader walks. NDN(x) (nd_gtab.h) and the v1 table of a moved list (NDG_F_MOVED: list = &ndd_<name>) point to an
 * NdNDev, not to list units: a reader that walks that pointer reads the struct as words (the VCOUNT crash: mdl_get_vertex_count). e is
 * nd_ntab_find(list) (nd_tab_init sorts the index before the logic thread starts: any thread may call it). */
#ifndef ND_LKEY_H
#define ND_LKEY_H
#include "nd_op.h"
#include "nd_gtab.h"
/* A list with a device entry e: e->u and at most e->nu units (no end unit). Any other list: the list itself and max (the reader stops at NDO_END). */
static inline const uint32_t* nd_lkey_units(const void* list, const NdNDev* e, unsigned max, unsigned* n) {
    if (e) {
        *n = e->nu < max ? (unsigned)e->nu : max;
        return e->u;
    }
    *n = max;
    return (const uint32_t*)list;
}
/* A v1 table (NdGTab): the units of its list, as nd_lkey_units. */
static inline const uint32_t* nd_lkey_tab(const NdGTab* t, unsigned max, unsigned* n) {
    return nd_lkey_units(t->list, (t->flags & NDG_F_MOVED) ? (const NdNDev*)(const void*)t->list : 0, max, n);
}
/* For a reader that needs the end unit (nd_dlc_tex_decode, nd_ent_segments): a list with a device entry is copied into buf (at most max - 1
 * units) and an NDO_END unit follows; buf has 2 * max words. Any other list: the list itself. */
static inline const uint32_t* nd_lkey_end(const void* list, const NdNDev* e, uint32_t* buf, unsigned max) {
    unsigned n, k;
    if (!e) return (const uint32_t*)list;
    const uint32_t* u = nd_lkey_units(list, e, max - 1u, &n);
    for (k = 0; k < n; k++) { buf[2u * k] = u[2u * k]; buf[2u * k + 1u] = u[2u * k + 1u]; }
    buf[2u * k] = NDO_W0(NDO_END, 0);
    buf[2u * k + 1u] = 0;
    return buf;
}
#endif
