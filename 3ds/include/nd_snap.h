/* nd_snap.h: the snapchk switch (NDR0A, design_nstate.md section 6, stage S4). Core 1 takes the combine inputs of nd_ds at each mesh
 * triangle (nd_snap_mesh, nd_snap.c) and records them as one NDC_NAPI run. Core 0 runs the record (nd_snapchk_run), then compares its own
 * walker inputs at the next triangle draw (nd_snapchk_tri, nd_geom.c). nd_snap_in (nd_snap_in.c) is pure and host-built. */
#ifndef ND_SNAP_H
#define ND_SNAP_H
#include <stdint.h>
#include "nd.h"
#include "nd_ds.h"
#include "nd_api.h"

typedef struct NdSnapBind {
    NdTd td; unsigned wrap[4]; int sz[4]; unsigned shift[2]; /* NDR0G1: tile n as ensure_tile and dual_bind read it */
} NdSnapBind;
typedef struct NdSnapChk {
    NdCombIn in;
    int32_t key, leg, mode5, pat, plan_on;
    int32_t role; uint32_t oml, geom, fogc, blendc, pad;
    NdCombOut o; /* NDR0E1: the coloured combine decision of core 1 */
    uint32_t pad2; /* NDR0E1: size to a multiple of 8 */
    NdRender rs; /* NDR0F1: nd_render_state of the snapshot (core 1) */
    int32_t mixk; unsigned mixc; /* NDR0F1: nd_snap_mix of the snapshot (kind, colour) */
    uint32_t pad3; /* NDR0F1: size to a multiple of 8 (the rs and mix words) */
    NdSnapBind b[2]; /* NDR0G1: tiles 0 and 1 */
} NdSnapChk;
_Static_assert(sizeof(NdSnapChk) % 8u == 0u, "units");
/* NDR0G2: the tile words the bind and uv code read: NULL = the walker tiles */
extern const NdSnapBind* nd_bsrc;
/* NDR0G2: the blocks of the open epoch, NULL = walker */
extern const NdSnapBind* nd_ep_bs;
#define ND_BTD(n) (nd_bsrc ? &nd_bsrc[n].td : &nd_ws.td[n])
#define ND_BWR(n) (nd_bsrc ? nd_bsrc[n].wrap : nd_ws.tst1.tile_wrap[n])
#define ND_BSZ(n) (nd_bsrc ? nd_bsrc[n].sz : nd_ws.tst1.tile_sz[n])
#define ND_BSH(n) (nd_bsrc ? nd_bsrc[n].shift : nd_ws.tst1.tile_shift[n])
/* NDR0G2, nd_snap_in.c: bit 1 << 2n when the td of tile n differs, 2 << 2n when its wrap, sz or shift words differ. */
unsigned nd_snap_bdiff(const NdSnapBind* a, const NdSnapBind* b);
/* NDR0G2, nd_snap.c, core 0: 1 when the armed snapshot tile words differ from the blocks e (NULL = the walker tiles). */
int nd_snap_bnew(const NdSnapBind* e);
/* NDR0G2, nd_snap.c, core 0: the blocks of the armed snapshot for the open epoch (a copy kept until the next epoch). */
const NdSnapBind* nd_snap_eb(void);
/* NDR0G2, nd_snap.c, core 0: the bind compare of the armed snapshot at the triangle epoch (nd_geom.c gv_epoch_enter). */
void nd_snapchk_bind(void);

/* nd_snap_in.c: the combine inputs of an NdDs (leg and pat are the walker's leg and box_pat of the snapshot). */
void nd_snap_in(const NdDs* s, int leg, int pat, NdCombIn* in);
/* nd_snap_in.c: the render inputs of an NdDs (oml, geom, fogc, blendc) into the snapshot (role left 0). */
void nd_snap_rnd(const NdDs* s, NdSnapChk* r);
/* NDR0G1, nd_snap_in.c: the bind block of tile n (td zero when not valid, and the tile n words of t). */
void nd_snap_bind(const NdTd* td, const NdTst* t, int n, NdSnapBind* o);
/* nd_snap_in.c: 1 when every texel of the tile d is 1.0 (tile0_white of nd_comb.c on d). */
int nd_td_white(const NdTd* d);
/* nd_snap.c, core 1: the target record of set_cimg (w0 = siz << 12 | width - 1, w1 = the address; nd_target_beginw). */
void nd_snap_tgt(uint32_t w0, uint32_t w1);
/* nd_snap.c, core 1: the snapshot of the triangle about to be recorded (PMDL("snapchk") only). */
void nd_snap_mesh(void);
/* nd_snap.c, core 0: the NDC_NAPI runner of the snapshot record; sets nd_snap_arm. */
void nd_snapchk_run(const uint32_t* rec);
/* NDR0F1, nd_snap_in.c: the mix of sync_render (nd_set.c) on the arguments: 1 colour mix, 2 fog shade alpha mix (col), 0 none (col 0). */
int nd_snap_mix(unsigned ol, unsigned fogc, unsigned blendc, int nofog, unsigned* col);
/* NDR0F1, nd_snap.c, core 0: the compare of the render block and the mix with the walker decision, at its triangle epoch (nd_geom.c).
 * NDR0F2: called by nd_snap_render and by draw_tri (no new epoch). */
void nd_snapchk_rnd(void);
/* NDR0F2, nd_snap.c, core 0: the render state and mix of a triangle from its armed snapshot (nd_geom.c gv_epoch_enter); the GPU state if the epoch changed. */
void nd_snap_render(void);
extern unsigned char nd_snap_rpend; /* NDR0F1: set by draw_tri for its own triangle, cleared after gv_tri and by nd_snapchk_rnd */
/* nd_snap.c, core 0: the compare of the armed snapshot at a triangle draw (nd_geom.c). */
void nd_snapchk_tri(void);
/* NDR0E2, nd_snap.c, core 0: the armed snapshot at a texture rectangle (nd_rect.c): the compare only. */
void nd_snapchk_rect(void);
/* NDR2A, nd_snap.c, core 1: the snap of a direct draw: nd_ds with the pso words (nd_direct_bind order), prim and env. */
void nd_snap_pso(const NdPsoN64* p, uint32_t prim, uint32_t env);
/* NDR2A, nd_snap.c, core 0: the armed pso snap against the decision of nd_direct_bind (log tag dir). */
void nd_snapchk_dir(const NdCombOut* w);
/* NDR1C, nd_snap.c, core 1: the snap of a texture rectangle (nd_rec_texrect); zcol of nd_rect.c:236 from the target. */
void nd_snap_rect(void);
/* NDR1C, nd_snap.c, core 0: 1 while the rectangle run has a live snap (a command clears it). */
extern unsigned char nd_snap_live;
/* NDR1C, nd_snap.c, core 0: the tile words of the armed snap, for the rectangle binds (nd_rect.c nd_bsrc). */
const NdSnapBind* nd_snap_cur_b(void);
/* NDR1C, nd_snap.c, core 0: the combine of the armed snap (nd_comb_apply on its inputs). */
void nd_snap_comb(void);
/* NDR1C, nd_snap.c, core 0: the drec log line `rectsnap f<n>` for a rectangle whose snap differs from the walker. */
void nd_snap_rlog(void);
/* nd_snap.c, core 0: the 100-frame counter line (PMDL("snapchk")), called at each frame stream end (nd_rec.c). */
void nd_snapchk_tick(void);
/* nd_rec.c: the count of the commands of every kind (the snapshot is taken when it changes). */
unsigned nd_rec_cmds(void);
/* nd_rec.c clears it at each command but the snapshot run; nd_snap.c sets it. */
extern unsigned char nd_snap_arm;
#endif
