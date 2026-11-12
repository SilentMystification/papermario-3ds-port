# Row S5TIMG: draw_img_util.c image draws through the NdTRect record (S5 call sites)

(Model: Haiku. A pattern edit of 4 load-plus-rect sites in one file. Blocker: S5TRECT merged.)

## Start
`sh 3ds/tools/wt_new.sh S5TIMG`; PM_ROW=S5TIMG PM_LANE=S5TIMG. Only src/draw_img_util.c is edited.

## Sites (src/draw_img_util.c at main ccdb7e3f5; re-grep `gDPLoadTextureTile\|gSPTextureRectangle` first, the lines may move)
- draw_image_with_clipping: the load switch :142-152 and the rect :155.
- draw_tiled_image: the loads :275-281 and the rect :284.
- draw_adjustable_tiled_image: the loads :461-475 and the rect :477.
- draw_ci_image_with_clipping (:8): it calls draw_image_with_clipping after its state lines :12-39 (check this by reading). Keep
  the state lines.

## Step 0 (oracle 4: a gap in nd_trect_run, 3ds/src/nd_direct.c after S5TRECT)
nd_trect_run passes r->prim to nd_direct_bind, which writes nd_ws.prim and the GPU prim (nd_direct.c nd_direct_bind). A site that
sets no prim would change the prim later walker draws inherit. In nd_trect_run, replace `nd_direct_bind(id, r->prim,
nd_direct_env())` with `nd_direct_bind(id, (r->pad & 1u) ? nd_direct_prim() : r->prim, nd_direct_env())`, and in nd_api.h change
the NdTRect comment to say `pad bit 0: keep the walker's prim`. This is an edit inside nd_trect_run, not an append.

## Exact edit per site (under `#ifdef TARGET_3DS`; `#else` keeps the old lines)
1. Keep every state op above the loop (PipeSync, cycle, filter, persp, render mode, combine, prim, TLUT, the LoadTLUT).
   The S5TRECT fallback (nd_trect_run, nd_direct_ready() != 1) replays only the texture load and the rectangle: it relies on the
   state ops and the LoadTLUT kept in the stream. Remove none of them; the record carries the palette only for the direct path.
2. Replace the load and the rect of one loop pass with one record: `{ NdTRect r; memset(&r, 0, sizeof r); ...;
   nd_rec_napi(nd_trect_run, &r, sizeof r / 8u); }`, filled as follows:
   - img, fmt, siz, width, height, uls, ult, lrs, lrt, cms, cmt, masks, maskt: the arguments of the load macro of that pass, in
     the same order as the macro call. siz is the literal of the call; for the _4b form, siz is G_IM_SIZ_4b.
   - pal and pal_n: the palette the function loaded with LoadTLUT. pal16 gives 16, pal256 gives 256, no TLUT gives NULL and 0.
     If the function only receives the palette from its caller, pass that pointer through.
   - w0, w1, hs, hd: `ND_W0_TR(G_TEXRECT, xh, yh)`, `ND_W1_TR(G_TX_RENDERTILE, xl, yl)`, `ND_TR_H1(s, t)` and
     `ND_TR_H2(dsdx, dtdy)`, with the exact argument expressions of that gSPTextureRectangle call (nd_gbi.h:201-204).
   - cw0 and cw1: the combine words the function set above. Build them as hud_element.c does with HUD_CW: copy its two macros
     into this file under other names.
   - oml: the render mode pair it set. omh: its TLUT mode (G_TT_RGBA16 or G_TT_NONE) plus its filter (G_TF_POINT here).
   - prim: the prim colour the function set before this pass (copy the expression). When no PrimColor of the function runs
     before the pass, set `r.pad = 1` (keep the walker's prim, step 0) and leave prim 0.
3. `#include "nd_api.h"` at the top, under `#ifdef TARGET_3DS`.

## Gate
- Build, memcheck OK; nd_host PASS.
- Callers: grep the 4 function names in src (20 call sites). Pick 2 det scenes that reach them: pause_tab_det_pic (pause
  images), and the title or file select det scene. Pre-flight `bench.py list --tag pic`. If no det scene of the title exists,
  use spot_mac01_det as the second scene and say so.
- `bench.py base` both; one list run (2 starts); `bench.py cmp` both: every shot byte-equal.
- No `trect fallback` lines in steady frames. Budget 5.

## Stop
A byte difference: stop, commit, hand back. Delete this row file in the completing commit.
