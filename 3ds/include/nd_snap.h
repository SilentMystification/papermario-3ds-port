/* nd_snap.h: the snapchk switch (NDR0A, design_nstate.md section 6, stage S4). Core 1 takes the combine inputs of nd_ds at each mesh
 * triangle (nd_snap_mesh, nd_snap.c) and records them as one NDC_NAPI run. Core 0 runs the record (nd_snapchk_run), then compares its own
 * walker inputs at the next triangle draw (nd_snapchk_tri, nd_geom.c). nd_snap_in (nd_snap_in.c) is pure and host-built. */
#ifndef ND_SNAP_H
#define ND_SNAP_H
#include <stdint.h>
#include "nd.h"
#include "nd_ds.h"

typedef struct NdSnapChk {
    NdCombIn in;
    int32_t key, leg, mode5, pat, plan_on;
} NdSnapChk;
_Static_assert(sizeof(NdSnapChk) % 8u == 0u, "units");

/* nd_snap_in.c: the combine inputs of an NdDs (leg and pat are the walker's leg and box_pat of the snapshot). */
void nd_snap_in(const NdDs* s, int leg, int pat, NdCombIn* in);
/* nd_snap.c, core 1: the snapshot of the triangle about to be recorded (PMDL("snapchk") only). */
void nd_snap_mesh(void);
/* nd_snap.c, core 0: the NDC_NAPI runner of the snapshot record; sets nd_snap_arm. */
void nd_snapchk_run(const uint32_t* rec);
/* nd_snap.c, core 0: the compare of the armed snapshot at a triangle draw (nd_geom.c). */
void nd_snapchk_tri(void);
/* nd_rec.c: the count of the commands of every kind (the snapshot is taken when it changes). */
unsigned nd_rec_cmds(void);
/* nd_rec.c clears it at each command but the snapshot run; nd_snap.c sets it. */
extern unsigned char nd_snap_arm;
#endif
