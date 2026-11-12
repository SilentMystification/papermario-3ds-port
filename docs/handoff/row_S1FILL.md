# Row S1FILL: screen colour overlays as a native fill record (NATIVEAPI B1)

Model: Haiku (one record, one runner that calls two existing functions; no design choice).

## Blocker and base
NAPI2B merged (nd_direct.c: nd_pso_decide, nd_direct_bind, nd_direct_end). `sh 3ds/tools/wt_new.sh S1FILL` after that merge;
PM_ROW=S1FILL PM_LANE=S1FILL. Base `git merge-base HEAD main`.

## Touch only
src/screen_overlays.c (_render_transition_stencil, the cases OVERLAY_SCREEN_COLOR and OVERLAY_VIEWPORT_COLOR at :209-246 only).

## Exact edits
1. `#include "nd_api.h"`. Above _render_transition_stencil, under `#ifdef TARGET_3DS`: `typedef struct { s32 x0, y0, x1, y1; u32 rgba,
   opaque; } NdFillRec;` (24 bytes: 3 units) and `static void fill_run(const uint32_t* u)`: build an NdPsoN64 with cw0/cw1 of
   `G_CC_PRIMITIVE, G_CC_PRIMITIVE` and oml of `CVG_DST_SAVE | G_RM_OPA_SURF, ...2` when opaque else `G_RM_CLD_SURF, G_RM_CLD_SURF2`
   (take the words from the macros into a local Gfx[2], as hud_rect_run does in NAPI2B), `nd_direct_bind(nd_pso_n64(&d), rgba, 0)`,
   `pm_gpu_fill_rect(x0, y0, x1, y1, rgba)`, `nd_direct_end()`.
2. In both cases, under `#ifdef TARGET_3DS`, replace the gDP lines from gDPPipeSync to gDPSetColorDither(G_CD_DISABLE) by one
   `nd_rec_napi(fill_run, &rec, 3)` with: SCREEN_COLOR x0 0, y0 0, x1 SCREEN_WIDTH, y1 SCREEN_HEIGHT (the 3DS line :224 keeps the
   exclusive lower right); VIEWPORT_COLOR the camera viewport rectangle of :243-244; rgba = colR << 24 | colG << 16 | colB << 8 |
   (u8)progress; opaque = progress == 255.0f. The `#else` keeps the old lines.
3. Dither: the old lines set G_CD_MAGICSQ and G_AD_PATTERN around the fill. The 3DS path has no dither (pre-flight: grep
   `G_CD_MAGICSQ` in 3ds/src: no reader). No edit for it.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- `bench.py base xfer_map_shots --commit <merge-base>`; `bench.py run xfer_map_shots,spot_mac01_det --row S1FILL --why "B1: overlay
  fill native"` (base spot_mac01_det too); `bench.py cmp` each: every shot byte-equal (the map fade draws the SCREEN_COLOR fill).
- Budget 5 Azahar starts. Console: none.

## Stop
A byte difference: stop, commit, hand back the shot list and the cmp output. Delete this row file in the completing commit.
