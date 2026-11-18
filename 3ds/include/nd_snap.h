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
    int32_t role; uint32_t oml, geom, fogc, blendc, pad;
    NdCombOut o; /* NDR0E1: the coloured combine decision of core 1 */
    uint32_t pad2; /* NDR0E1: size to a multiple of 8 */
} NdSnapChk;
_Static_assert(sizeof(NdSnapChk) % 8u == 0u, "units");

/* nd_snap_in.c: the combine inputs of an NdDs (leg and pat are the walker's leg and box_pat of the snapshot). */
void nd_snap_in(const NdDs* s, int leg, int pat, NdCombIn* in);
/* nd_snap_in.c: the render inputs of an NdDs (oml, geom, fogc, blendc) into the snapshot (role left 0). */
void nd_snap_rnd(const NdDs* s, NdSnapChk* r);
/* nd_snap_in.c: 1 when every texel of the tile d is 1.0 (tile0_white of nd_comb.c on d). */
int nd_td_white(const NdTd* d);
/* nd_snap.c, core 1: the target record of set_cimg (w0 = siz << 12 | width - 1, w1 = the address; nd_target_beginw). */
void nd_snap_tgt(uint32_t w0, uint32_t w1);
/* nd_snap.c, core 1: the snapshot of the triangle about to be recorded (PMDL("snapchk") only). */
void nd_snap_mesh(void);
/* nd_snap.c, core 0: the NDC_NAPI runner of the snapshot record; sets nd_snap_arm. */
void nd_snapchk_run(const uint32_t* rec);
/* nd_snap.c, core 0: the compare of the armed snapshot at a triangle draw (nd_geom.c). */
void nd_snapchk_tri(void);
/* NDR0E2, nd_snap.c, core 0: the armed snapshot at a texture rectangle (nd_rect.c): the compare only. */
void nd_snapchk_rect(void);
/* nd_snap.c, core 0: the 100-frame counter line (PMDL("snapchk")), called at each frame stream end (nd_rec.c). */
void nd_snapchk_tick(void);
/* nd_rec.c: the count of the commands of every kind (the snapshot is taken when it changes). */
unsigned nd_rec_cmds(void);
/* nd_rec.c clears it at each command but the snapshot run; nd_snap.c sets it. */
extern unsigned char nd_snap_arm;
#endif
