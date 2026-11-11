# Row WALLSHADE: the KZN04 sprite shade colour leaks into later model and dlc draws

(Model: Haiku. 60 lines or fewer. The agent reads only this file, CLAUDE.md and what this file names.)

## User facts (bugs only)
User (2026-11-11, O3DS XL, a build with HC4f3, HC4e4, HC8, KZN04 flat shade, SPLASHPREV): "The walls in Shooting Star Summit are
missing." Shooting Star Summit is area_hos (hos_00 Shooting Star Path, hos_01 Shooting Star Summit), not area_sam (Shiver Mountain).

## Cause (oracle code read; this row fixes one named path, row_BATUI-style questions for the rest are in the hand-back)
- 3ds/src/nd_geom.c:88 (KZN04 TEV step, 16a4fdd4f): pm_gbi_gv_at multiplies the draw's cm and ca by dual_f[0..3] whenever the walker
  global dual_on is 1. dual_on is set only by a shaded sprite (nd_tex_set.c:581 tex_shaded0) and cleared only at a tile 0 SETTILESIZE
  (nd_tex_set.c:196). Every draw after a shaded sprite and before the next tile 0 SETTILESIZE gets the sprite's shade colour: the native
  model ranges (nd_model_set.c:37), the dlc static groups (nd_shape.c:203 gv_params_calc) and the ring triangles (nd_geom.c:223).
- Untextured model records and records whose R13 ops hold no tile 0 SETTILESIZE (nd_recs.c:81 rec_aux) do not clear dual_on, and the
  model cache stores the scaled cm/ca (nd_model.c:382) under a key with no dual_on or dual_f field, so a hit replays the wrong colour.
- hos_01 sets SetSpriteShading(SHADING_HOS_01) (src/world/area_hos/hos_01/main.c:117): Mario, the spirits and the NPCs are shaded sprites.
- User facts 2: hos_01, all walls. The hos_01 shape (romfs/shape/hos_01_shape.nds) names 11 textures (ground, grass, flowers, star
  gate, frame, door side) and no wall texture: the walls are untextured records, so no bind clears dual_on before them, and dual_on is
  never reset between frames (a shaded sprite of frame N reaches the first models of frame N+1). The why-6 drops (nd_model.c:350-351)
  need a TEXEL1 plan: an untextured wall cannot reach them. This row alone is the fix by code read.
- The shade colour is meant for the shaded sprite only (nd_tex_set.c:646-649 comment): pm_gpu_spr_build (pm_gpu.c:3869) and the walker
  fallback of a sprite (nd_walk.c:152 nd_ops_run of the NDC_SPR words).

## Start
`sh 3ds/tools/wt_new.sh WALLSHADE` (build3ds/wt/WALLSHADE, branch WALLSHADE from main). PM_ROW=WALLSHADE PM_LANE=WALLSHADE on every call.

## Touch only
3ds/src/nd_geom.c: pm_gbi_gv_at (:88), gv_params_calc (:61), one new global above gv_params_calc. 3ds/src/nd_walk.c: nd_walk_spr (:152).
3ds/src/pm_gpu.c: pm_gpu_spr_build, the line after `pm_gbi_gv_at(mode.has_tx ? mode.tw1 : sw_tss, geo, tw, th, &gv);` (:3869) only.
Other rows: HC6 (pm_gpu.c model key/hit/submit, nd_recs.c, nd_model.c, nd_emit.c, nd_rec.c), UB1 (pm_gpu.c :3750, :3787-3788, :3796):
do not edit those lines. `git merge main` before the completing commit.

## Exact edits
1. nd_geom.c:88: delete the whole line `if (dual_on) for (int q = 0; q < 4; q++) { o->cm[q] *= dual_f[q]; o->ca[q] *= dual_f[q]; } ...`.
2. nd_geom.c, directly above gv_params_calc (:61) add:
   `int nd_spr_walk; /* WALLSHADE: 1 while nd_walk_spr runs the walker words of a sprite (the shaded sprite fallback) */`
   and replace the body of gv_params_calc with:
   `{ int tw = 1, th = 1; pm_tex_size(&tw, &th); pm_gbi_gv_at(tss, nd_ws.geom, tw, th, o); if (dual_on && nd_spr_walk) for (int q = 0; q < 4; q++) { o->cm[q] *= dual_f[q]; o->ca[q] *= dual_f[q]; } } /* KZN04: the shade colour of a shaded sprite, only for the sprite (WALLSHADE) */`
3. nd_walk.c:152: `nd_ops_run(q + G_PM_SPR_WORDS, 2u, 1);` becomes
   `{ extern int nd_spr_walk; nd_spr_walk = 1; nd_ops_run(q + G_PM_SPR_WORDS, 2u, 1); nd_spr_walk = 0; } /* WALLSHADE */`
4. pm_gpu.c, after the pm_gbi_gv_at line in pm_gpu_spr_build (:3869) add:
   `if (shaded) { extern float dual_f[4]; for (int q = 0; q < 4; q++) { gv.cm[q] *= dual_f[q]; gv.ca[q] *= dual_f[q]; } } /* WALLSHADE: the KZN04 shade colour, the shaded sprite only */`

## Gate (CLAUDE.md 2: nd_host and byte compare only, no picture)
- `PM_ROW=WALLSHADE PM_LANE=WALLSHADE sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row WALLSHADE --why "WALLSHADE gv_at"`: PASS.
- `PM_ROW=WALLSHADE PM_LANE=WALLSHADE python 3ds/tools/bench.py base spot_mac01_det`, the same for sprshade_hos01_pic. Then one list: `PM_ROW=WALLSHADE PM_LANE=WALLSHADE python 3ds/tools/bench.py run spot_mac01_det,sprshade_hos01_pic --row WALLSHADE --why "dual_f only on shaded sprites: no shaded sprite in mac_01 (equal), hos_01 shaded (draws after sprites change)"`.
  Then `bench.py cmp spot_mac01_det --row WALLSHADE`: 6/6 byte-equal (pass gate). `bench.py cmp sprshade_hos01_pic --row WALLSHADE`:
  write the per-shot diff pixels in the commit (a change is expected; no pass/fail, the look is the user's console check in hos_01).

## Stop
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, then hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash, the gate results, the memcheck line.
