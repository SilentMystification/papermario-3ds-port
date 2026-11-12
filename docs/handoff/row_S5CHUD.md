# Row S5CHUD: complex HUD elements (mode 2 quads) drawn directly (NATIVEAPI E1)

Model: Sonnet (user decides).
- Why not Haiku: this row makes the first direct textured quad with a matrix (nd_direct_quad). NAPI2B has only screen rectangles.
  Every later quad producer uses it (S5ITEM, the sprite effect arm). It must keep the walker's matrix, clip and cull results.
- Why not Opus: one quad shape, and the decision and bind come from NAPI2B.

## Blocker and base
NAPI2B merged. Mode 1 (CI4 through imgfx) waits for S5IMGFX: not in this row. `sh 3ds/tools/wt_new.sh S5CHUD`; PM_ROW=S5CHUD
PM_LANE=S5CHUD.

## Code read (src/hud_element.c render_complex_hud_element :1537-)
- Every call does:
  - the filter op (:1561-1565);
  - the float matrix sp60 (pivot, translate, scale, rot Y, Z, X: :1594-1613), then guMtxF2L and gSPMatrix PUSH|LOAD (:1614-1616);
  - the mode from the format flags (:1618-1636).
- Mode 1: imgfx (out of scope).
- Mode 2 (IA8, the battle menu icons): HudElemTemplateQuad is copied into transform->quadBuffers[FrameQuadIndex] (:1690-), and
  the vertex positions and tex coords are set from width and height. Then come a texture load, the combine and render mode,
  gSPVertex and 2 triangles, and the pop. Read the rest of the case before the design: lines :1706 to the end of the function.
- napicen3 btl: hud_element.o 211 ops a frame (render_complex_hud_element is the largest part, 133 in NAPICEN2).

## Design
1. New helper in 3ds/src/nd_direct.c: `void nd_direct_quad(const float m[4][4], const Vtx v[4], const uint8_t idx[6])`.
   - It draws through the ring path as gSPVertex plus gSP2Triangles would: a matrix palette entry from m (the walker's load_mtx
     form with a float source), the 4 vertices copied to the ring with their matrix id, gv_tri for both triangles.
   - It keeps the walker's CPU clip and cull decisions (clip_bits4, cull_cross).
   - Read nd_geom.c nd_geom_vtx and gv_tri and nd_mtx_set.c load_mtx. Call the same functions with a local state; do not
     re-implement them.
   - The matrix stack: the old path pushes and pops. The record holds sp60 and the runner loads it as a pushed matrix, then pops.
2. Record NdCHudRec {sp60 as float[16], vtx[4], NdBindDesc fields of the IA8 load, pso fields (combine, render mode, filter),
   prim colour}. The record is built on core 1 in the mode 2 case under TARGET_3DS. Runner: nd_direct_ready, nd_direct_bind,
   nd_direct_tex, nd_direct_quad, nd_direct_end. The fallback is the old ops through nd_walk_state from a local list (as
   hud_rect_run does).
3. interp (fps60): HUD quads use the walker matrix path today. With fps60 the K matrix twin comes from nd_mtx_set.c. The record
   must give the same twin: load sp60 through the same load_mtx entry that the twin hooks (INTERPB1).

## Risks
- Matrix and twin path (fps60 K copy): gate with spot_mac01_fps60_det only if the battle HUD is in that scene. If not, say so.
  The battle gate covers the mono path.
- FrameQuadIndex double buffer: the vertices live in transform->quadBuffers. The record copies them, so core 1 may rewrite them
  in the next step.

## Files and places
src/hud_element.c (render_complex_hud_element mode 2 case only; hud_rect_direct stays), 3ds/src/nd_direct.c (nd_direct_quad at the
end), 3ds/include/nd_api.h (the declaration at the end).


## Merge rule (orchestrator, 2026-11-12)
Rows that append to the end of 3ds/include/nd_api.h, 3ds/src/nd_direct.c or 3ds/tools/nd_direct_test.c conflict when two merge in
sequence. Right before the gate runs: `git merge main`; at a conflict at those file ends keep both appends (main's first, then
yours), rebuild, nd_host; then take the bases at the new `git merge-base HEAD main` and run the gate.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS (add a host case: nd_direct_quad with the identity matrix gives the same ring vertices as the
  gSPVertex path).
- `bench.py base battle_wheel_nowide_det`; one list `bench.py run battle_wheel_nowide_det,spot_mac01_det,napicen_btl --row S5CHUD
  --why "E1: complex HUD quads direct"` (3 starts); cmp both det scenes: every shot byte-equal; napicen_btl: hud_element.o drops by
  the mode 2 ops (hand back the per-function numbers).
- Budget 8 Azahar starts.

## Stop
A byte difference: stop, commit, hand back (2 tries per named cause). Delete this row file in the completing commit.
