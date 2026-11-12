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

## Stop
A failed gate or an edit that does not fit: stop, commit, hand back the output. Delete this row file in the completing commit.
