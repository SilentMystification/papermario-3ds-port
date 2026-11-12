# Row S4WBLK: water_block components on the native particle path (NATIVEAPI S4)

(Model: Haiku: the S4EMB pattern, one loop, no design choice. Runs in parallel with S4E3D, S4SNAK, FSELCEN: different files and
different insert points in the two JSON files.)

## Design
water_block_appendGfx (src/effects/water_block.c:216-304): after `gSPDisplayList(D_090003B0_3B6FA0)` (:278), per component with
`data->unk_88[i] >= 0` (:280-288): the float matrix (guPositionF), `gSPMatrix(PUSH|MUL|MODELVIEW)`, `gSPDisplayList(D_090004A0_3B7090)`,
`gSPPopMatrix`. Same macros into a local Gfx[3], nd_parts_add with pre 1 (matrix), post 1 (pop). The rest of the function (the
vertex mesh at :290-303) stays as it is. The previous-frame refraction is water_splash, not this effect.

## Start
`sh 3ds/tools/wt_new.sh S4WBLK`. PM_ROW=S4WBLK PM_LANE=S4WBLK on every build and bench call. Base: `git merge-base HEAD main` (main is
20d613506 at writing).

## Touch only
src/effects/water_block.c (water_block_appendGfx, the include list); 3ds/tools/azt_scenarios.json and 3ds/tools/bench_defs.json:
`fxpic_84` inserted right after the `fxpic_105` entry, `fxfncov_84` right after the `fxfncov_55` entry (both files).

## Exact edits
1. `#include "nd_api.h"` after the includes. Declare `NdParts pb;` under `#ifdef TARGET_3DS` with the other locals.
2. After `gSPDisplayList(gMainGfxPos++, D_090003B0_3B6FA0);` (:278): `#ifdef TARGET_3DS` `nd_parts_begin(&pb, 1, 1);` `#endif`.
3. Inside `if (data->unk_88[i] >= 0)` keep guPositionF and guMtxF2L; the 3 gbi lines (:284-286) become `#ifdef TARGET_3DS`
   `{ Gfx g[3]; gSPMatrix(&g[0], &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
   gSPDisplayList(&g[1], D_090004A0_3B7090); gSPPopMatrix(&g[2], G_MTX_MODELVIEW); nd_parts_add(&pb, (const uint32_t*)g); }` `#else`
   the old 3 lines `#endif`.
4. After the loop, before `gSPDisplayList(gMainGfxPos++, D_09000300_3B6EF0);` (:290): `#ifdef TARGET_3DS` `nd_parts_end(&pb);` `#endif`.
5. Scenario and bench `fxpic_84` (copy of `fxpic_50`, `fxtest=84`) and `fxfncov_84` (copy of `fxfncov_50`, `fxtest=84`).

## Gate
- Build, memcheck OK; nd_host PASS.
- `bench.py base fxpic_84 --commit <merge-base>`; `bench.py run fxpic_84 --row S4WBLK --why "S4: water_block components as one
  NDC_NAPI record"`; `bench.py cmp fxpic_84 --row S4WBLK --commit <merge-base>`: every shot byte-equal.
- Speed gate (census): `PM_FNCOV=1 PM_ROW=S4WBLK PM_LANE=S4WBLK sh 3ds/build.sh`; `bench.py run fxfncov_base,fxfncov_84 --row S4WBLK
  --lane S4WBLK_fncov --why "S4 speed gate"`; `hotset.py --fncov <log> --fncovfn --lane S4WBLK` on both logs. Pass (effect minus
  base): nd_part_draw > 0; gv_tri, draw_tri, nd_geom_vtx, nd_ops_run and ent_gdl each < 20 (the mesh at :295-301 is not a part:
  if only its calls remain above 20, hand back the deltas; do not change the mesh). Hand back the 15 largest deltas.
- Budget: 5 Azahar starts (1 base, 1 byte run, 2 census, 1 spare). Console: none here; the S4 batch console run after the merge.

## Try 2 (oracle code read; the code edit 12cf5c703 stays, no code change)
- Facts of try 1: fxpic_84 6/6 byte-equal; census at fxfncov_84 f420: nd_part_draw 0, gv_tri +80, ent_gdl +42, rec_open +448.
- Cause: the census frame is after the water block is gone. fxtest spawns at slot frame 2 (pm_fxtest.c:179-180, :263-275: START 400,
  so spawn at f402) with `fx_water_block(FX_WATER_BLOCK_DESTROY, X, Y, Z, 1.5f, 10)` (pm_fxtest.c:116). duration 10 sets timeLeft 10
  (water_block.c:91); at timeLeft < 0 the DESTROY type starts water_splash and removes the block (water_block.c:145-153), about f413.
  At f420 only the splash draws: its drops (water_splash.c:193-228, a LoadMultiTile and a list per drop) are the deltas. No component
  draw happens in that frame, so nd_part_draw 0 is not a shape fallback. The list D_090004A0_3B7090 (VTX 4 at slot 0, TRI2, PipeSync,
  End) has the same shape as embers' D_090003B0_3DC2F0, which passes parts_ok (nd_rec.c:283-299). Components are live from the first
  update: unk_88 starts at -13, -9, ..., +111 (water_block.c:105-107).
- The base frame f396 of fxfncov_base is by design: it is before the spawn (f402), so it is the frame with no effect.
- Scenario edits (one commit): `fxfncov_84` gets `fncov=406` (4 frames after the spawn, inside the block's life). `fxpic_84` gets
  `shotwin=404:4` and until `shot \d+ frame 407\b` (the old shots 412-415 straddle the destroy at about f413).
- Runs (4 starts; budget 3 used): `bench.py base fxpic_84 --commit <merge-base>`; `bench.py run fxpic_84 --row S4WBLK --why "S4WBLK
  try 2: shots inside the block life"`; `bench.py cmp fxpic_84 --row S4WBLK --commit <merge-base>`. Then the census lane:
  `bench.py run fxfncov_base,fxfncov_84 --row S4WBLK --lane S4WBLK_fncov --why "S4WBLK census at f406"`; hotset with `--lane S4WBLK`.
- Pass: every fxpic_84 shot byte-equal. Census (effect minus base, per frame): nd_part_draw above 0; nd_geom_vtx at most 2 per live
  instance (the mesh loads at water_block.c:295 and :299); gv_tri and draw_tri at most 28 per instance plus 20 (the mesh lists
  D_090004D8 and D_09000538 hold 10 + 4 TRI2 = 28 triangles); ent_gdl and nd_ops_run each below 20 per instance plus 20. Expected:
  nd_part_draw equals the number of live components. The fxtest spawns one instance: the per-instance limits are the numbers.

## Stop
A failed gate or an edit that does not fit: stop, commit, hand back the output. Delete this row file in the completing commit.
