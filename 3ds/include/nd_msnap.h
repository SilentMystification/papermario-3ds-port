/* nd_msnap.h: the model snap (NREC1A, design_nstate.md section 6). Core 1 copies the render inputs of nd_ds into the stream after each
 * model record (flag bit 0 of the NDC_MODEL arg). Core 0 compares them with the walker inputs of model_draw (nd_recs.c, switch snapchk). */
#ifndef ND_MSNAP_H
#define ND_MSNAP_H
#include <stdint.h>
#include "nd_snap.h"
#include "nd_ds.h"
#define ND_MSNAP_MAGIC 0x4d534e50u
typedef struct NdMSnap {
    uint32_t magic, prim, prim_lod, omh, oml, kh, kl, tss, geom, cw0, cw1, fogc, blendc, envc;
    NdSnapBind b[2];
} NdMSnap;
_Static_assert(sizeof(NdMSnap) % 8u == 0u, "units");
void nd_msnap_take(const NdDs* s, NdMSnap* m);
unsigned nd_msnap_diff(const NdMSnap* a, const NdMSnap* b, const char** nm, int max, int* nb);
void nd_msnap_rec(uint32_t* p, unsigned units);
void nd_msnap_cmp(const uint32_t* q);
void nd_msnap_tick(void);
#endif
