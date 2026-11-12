# Row S5SHADE: sprite shading (appendGfx_shading_palette) as part of the NDC_SPR record (NATIVEAPI S5)

Model: Sonnet (user decides).
- Why not Haiku: shaded sprites read a second raster on tile 1 (the m1.valid path that nd_direct_ready refuses:
  `if (m1.valid) return 0`, nd_direct.c). The shading palette is computed per frame from the light. This row states tile 1 for
  the sprite record.
- Why not Opus: S5SPR sets the record form and S2W the tile-1 helper.

## Blocker and base
S5SPR and S2W merged. `sh 3ds/tools/wt_new.sh S5SHADE`; PM_ROW=S5SHADE PM_LANE=S5SHADE.

## Code read
src/sprite_shading.c: appendGfx_shading_palette 20 gbi calls, func_801491E4 3. The walker side of shaded sprites:
- m1 (M1Rec, 3ds/include/nd_int.h:186);
- set_tilesize's tex_shaded0;
- dual_on (nd_comb.c);
- the nd_direct_bind line `dual_on = 0`.
Read those before the design.

## Design
1. The shading palette and the tile-1 raster go into the NDC_SPR record's state block (S5SPR form) as a second NdBindDesc and
   the combine of the shaded pass.
2. Core 0 binds tile 1 through nd_direct_tex1 and sets dual_on as the walker's set_tilesize arm does for a shaded sprite.
3. nd_direct_ready's m1 refusal is lifted only for this record path. Other draws keep it.

## Files and places
src/sprite_shading.c (both functions), src/sprite.c (the shaded arm of the component functions only, after S5SPR), 3ds/src/nd_direct.c
(nd_direct_ready only, if the refusal must change).

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- One list with bases: a scene with shaded sprites, either a lit area or kmr_10 (pre-flight: grep the scenario list for
  "shade" or "BOMBLIT"; name it), plus spot_mac01_det. 2 starts. Every shot byte-equal.
- listcen down by the moved calls. Budget 6.

## Stop
A byte difference: stop, commit, hand back. Delete this row file in the completing commit.
