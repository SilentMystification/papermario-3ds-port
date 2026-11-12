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

## Gate (expected values; oracle 5 pre-flight, main 5c3865487)
- Blockers: S2W (nd_direct_tex1) and S5SPR2/S5SPR3 merged. Neither S2W nor S1PREV is merged at this writing.
- Build, memcheck OK; nd_host PASS.
- `bench.py base sprshade_hos01_pic` (hos_01 shaded sprites, det, shots 200-205) and `bench.py base spot_mac01_det`; one list
  `bench.py run sprshade_hos01_pic,spot_mac01_det --row S5SHADE --why "S5SHADE: shaded sprites in the record"` (2 starts).
  cmp 6/6 and 6/6 byte-equal.
- Census (rule of 2026-11-12: it counts only what this row moves): `bench.py run bombette_mim11 --row S5SHADE` (1 start; the
  shadecen switch, line `shadecen f<n> m1 <a> ... nat <n> fb <b> ...` of nd_tex_set.c:736): `m1` (the walker's set_tilesize
  shaded arm, shc[0]) drops to 0 and `fb` to 0 for the sprites this row moves; hand back the line of main (runs.jsonl) and of the
  row. Budget 6.

## Stop
A byte difference: stop, commit, hand back. Delete this row file in the completing commit.
