/* nd_snap.h: the snapchk switch (NDR0A, design_nstate.md section 6, stage S4). Core 1 takes the combine inputs of nd_ds at each mesh
 * triangle (nd_snap_mesh, nd_snap.c) and records them as one NDC_NAPI run. Core 0 runs the record (nd_snapchk_run), then compares its own
 * walker inputs at the next triangle draw (nd_snapchk_tri, nd_geom.c). nd_snap_in (nd_snap_in.c) is pure and host-built. */
#ifndef ND_SNAP_H
#define ND_SNAP_H
#include <stdint.h>
#include "nd.h"
#include "nd_ds.h"
#include "nd_api.h"
#include "nd_apw.h" /* NWS4B */

typedef struct NdSnapBind {
    NdTd td; unsigned wrap[4]; int sz[4]; unsigned shift[2]; /* NDR0G1: tile n as nd_tile_bind and dual_bind read it */
} NdSnapBind;
typedef struct NdSnapChk {
    NdCombIn in;
    int32_t key, leg, mode5, pat, plan_on;
    int32_t role; uint32_t oml, geom, fogc, blendc, tss; /* NCONV2A: tex_ss << 16 | tex_ts (was pad) */
    NdCombOut o; /* NDR0E1: the coloured combine decision of core 1 */
    uint32_t plod; /* NCONV2D: the 16-bit prim LOD word (was pad2) */
    NdRender rs; /* NDR0F1: nd_render_state of the snapshot (core 1) */
    int32_t mixk; unsigned mixc; /* NDR0F1: nd_snap_mix of the snapshot (kind, colour) */
    uint32_t pad3; /* NDR0F1: size to a multiple of 8 (the rs and mix words) */
    uint32_t fill, primz; /* NAT4H: nd_c0.fill and prim_z of core 1 */
    NdSnapBind b[2]; /* NDR0G1: tiles 0 and 1 */
} NdSnapChk;
_Static_assert(sizeof(NdSnapChk) % 8u == 0u, "units");
/* NDR0G2: the tile words the bind and uv code read: NULL = the walker tiles */
extern const NdSnapBind* nd_bsrc;
/* NDR0G2: the blocks of the open epoch, NULL = walker */
extern const NdSnapBind* nd_ep_bs;
#define ND_BTD(n) (nd_bsrc ? &nd_bsrc[n].td : &nd_c0.td[n])
#define ND_BWR(n) (nd_bsrc ? nd_bsrc[n].wrap : nd_c0.tst1.tile_wrap[n])
#define ND_BSZ(n) (nd_bsrc ? nd_bsrc[n].sz : nd_c0.tst1.tile_sz[n])
#define ND_BSH(n) (nd_bsrc ? nd_bsrc[n].shift : nd_c0.tst1.tile_shift[n])
/* NDR0G2, nd_snap_in.c: bit 1 << 2n when the td of tile n differs, 2 << 2n when its wrap, sz or shift words differ. */
unsigned nd_snap_bdiff(const NdSnapBind* a, const NdSnapBind* b);
unsigned nd_snap_fill(void); /* NAT4H */
/* NDR0G2, nd_snap.c, core 0: 1 when the armed snapshot tile words differ from the blocks e (NULL = the walker tiles). */
int nd_snap_bnew(const NdSnapBind* e);
/* NDR0G2, nd_snap.c, core 0: the blocks of the armed snapshot for the open epoch (a copy kept until the next epoch). */
const NdSnapBind* nd_snap_eb(void);

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
/* nd_snap.c, core 1: the snapshot of the triangle about to be recorded. */
void nd_snap_mesh(void);
/* nd_snap.c, core 0: the NDC_NAPI runner of the snapshot record; sets nd_snap_arm. */
void nd_snapchk_run(const uint32_t* rec);
/* NDR0F1, nd_snap_in.c: the mix of nd_rnd_sync (nd_set.c) on the arguments: 1 colour mix, 2 fog shade alpha mix (col), 0 none (col 0). */
int nd_snap_mix(unsigned ol, unsigned fogc, unsigned blendc, int nofog, unsigned* col);
/* NDR0F2, nd_snap.c, core 0: the render state and mix of a triangle from its armed snapshot (nd_geom.c gv_epoch_enter); the GPU state if the epoch changed. */
void nd_snap_render(void);
extern unsigned char nd_snap_rpend; /* NDR0F1: set by draw_tri for its own triangle, cleared after nd_gv_tri and by nd_snap_render */
/* nd_snap.c, core 0: the compare of the armed snapshot at a triangle draw (nd_geom.c). */
void nd_snapchk_tri(void);
/* NDR2A, nd_snap.c, core 1: the snap of a direct draw: nd_ds with the pso words (nd_direct_bind order), prim and env. */
void nd_snap_pso(const NdPsoN64* p, uint32_t prim, uint32_t env);
/* NDR2D, nd_snap.c, core 0: the decision of the live snap (the core 1 pso decision), NULL when no snap is live. */
const NdCombOut* nd_snap_dir_o(void);
/* NDR1C, nd_snap.c, core 1: the snap of a texture rectangle (nd_rec_texrect); zcol of nd_rect.c:236 from the target. */
void nd_snap_rect(void);
void nd_snap_alt(const NdDs* s, int rect); void nd_snapcp_run(const uint32_t* rec); /* NWS4A, nd_snap.c: the snap of a copy draw (core 1), its runner (core 0) */
/* NDR1C, nd_snap.c, core 0: 1 while the rectangle run has a live snap (a command clears it). */
extern unsigned char nd_snap_live;
/* NDR1C, nd_snap.c, core 0: the tile words of the armed snap, for the rectangle binds (nd_rect.c nd_bsrc). */
const NdSnapBind* nd_snap_cur_b(void);
const NdSnapChk* nd_snap_cur(void); /* NCS1, nd_snap.c, core 0: the last applied snap */
/* NDR1C, nd_snap.c, core 0: the combine of the armed snap (nd_comb_apply on its inputs). */
void nd_snap_comb(void);
/* NDR1C, nd_snap.c, core 0: the drec log line `rectsnap f<n>` for a rectangle whose snap differs from the walker. */
void nd_snap_rlog(void);
/* nd_rec.c: the count of the commands of every kind (the snapshot is taken when it changes). */
unsigned nd_rec_cmds(void);
/* nd_rec.c clears it at each command but the snapshot run; nd_snap.c sets it. */
extern unsigned char nd_snap_arm;
int nd_snap_spr(uint32_t o[9]); /* NDR4, nd_snap.c, core 0: the snap words of a sprite record (pm_gpu_spr_build); 0 when no snap */
/* NCONV1D1, nd_snap.c and nd_snap_in.c: the snap table of a list (rec[4] of a drawing list, nd_rec_dl). */
#define ND_LS_HDR(n) ((8u + 4u * (n) + 7u) & ~7u)
extern unsigned char nd_ls_on, nd_lsc_on; extern unsigned nd_ls_ord;
void nd_lsr_begin(const uint32_t* b); void nd_lsr_end(void); void nd_lsr_tri(unsigned ord); void nd_lsr_seg(unsigned ord); void nd_lsc_unit(uint32_t w0); void nd_lsc_tris(unsigned n);
unsigned nd_snap_lsr(const uint32_t* b, unsigned ord);
#define ND_LS_EP 0x80000000u /* NCONV2C: a core 0 cut comes before this range start */
#define ND_LS_DUP 0x40000000u /* NCONV2C: the range repeats the snap before it; core 0 does not arm it */
#define ND_LS_ORD(f) ((f) & 0xffffu) /* NAT2D: ord bits 0-15 */
#define ND_LS_SNAP(f) (((f) >> 16) & 0x3fffu) /* NAT2D: the snap index of a range */
#define ND_LS_DSB 32u /* PERF1: 8 end words */
void nd_lsr_geo(void);
extern const NdSnapChk* nd_ap; void nd_snap_hold(void); void nd_snap_ctx(void); /* NWS4B; PERF1 */
#endif
