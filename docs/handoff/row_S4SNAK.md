# Row S4SNAK: snaking_static on the native particle path (NATIVEAPI S4)

(Model: Haiku: the S4EMB pattern (tile sizes in the pre units), one loop, no design choice. Runs in parallel with S4E3D, S4WBLK,
FSELCEN: different files and different insert points in the two JSON files.)

## Design
snaking_static_appendGfx (src/effects/snaking_static.c:161-207): after `gSPDisplayList(D_09001000_3B3D90)` (:185), per part
(:188-204) `gDPSetPrimColor`, the float matrix arithmetic, `gSPMatrix(PUSH|MUL|MODELVIEW)`, `gDPSetTileSize` tile 0, `gDPSetTileSize`
tile 1, `gSPDisplayList(D_090010F8_3B3E88)`, `gSPPopMatrix`. Same macros into a local Gfx[6], nd_parts_add with pre 4 (prim, matrix,
tile 0, tile 1), post 1 (pop).

## Start
`sh 3ds/tools/wt_new.sh S4SNAK`. PM_ROW=S4SNAK PM_LANE=S4SNAK on every build and bench call. Base: `git merge-base HEAD main` (main is
20d613506 at writing).

## Touch only
src/effects/snaking_static.c (snaking_static_appendGfx, the include list); 3ds/tools/azt_scenarios.json and 3ds/tools/bench_defs.json:
`fxpic_81` inserted right after the `fxpic_50` entry, `fxfncov_81` right after the `fxfncov_51` entry (both files).

## Exact edits
1. `#include "nd_api.h"` after the includes.
2. After `gSPDisplayList(gMainGfxPos++, D_09001000_3B3D90);` (:185): `#ifdef TARGET_3DS` `NdParts pb; nd_parts_begin(&pb, 4, 1);` `#endif`
   (declare `pb` at the top of the function if the compiler rejects a declaration after code).
3. In the loop keep the matrix arithmetic (:190-195). On 3DS the prim line (:189) goes inside `#ifndef TARGET_3DS`; the lines
   :197-203 become `#ifdef TARGET_3DS` `{ Gfx g[6]; gDPSetPrimColor(&g[0], 0, 0, primR, primG, primB, (data->unk_24 * unk_24) / 255);
   gSPMatrix(&g[1], &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW); gDPSetTileSize(&g[2],
   G_TX_RENDERTILE, data->unk_40 * 64, 0, (data->unk_40 * 64) | 0x3C, 0x007C); gDPSetTileSize(&g[3], 1, (lifeTime * 12), 0,
   ((lifeTime * 3) + 0x1F) * 4, 0x007C); gSPDisplayList(&g[4], D_090010F8_3B3E88); gSPPopMatrix(&g[5], G_MTX_MODELVIEW);
   nd_parts_add(&pb, (const uint32_t*)g); }` `#else` the old lines `#endif`.
4. After the loop, before `gDPSetAlphaCompare` (:205): `#ifdef TARGET_3DS` `nd_parts_end(&pb);` `#endif`.
5. Scenario and bench `fxpic_81` (copy of `fxpic_50`, `fxtest=81`) and `fxfncov_81` (copy of `fxfncov_50`, `fxtest=81`).

## Gate
- Build, memcheck OK; nd_host PASS.
- `bench.py base fxpic_81 --commit <merge-base>`; `bench.py run fxpic_81 --row S4SNAK --why "S4: snaking_static parts as one NDC_NAPI
  record"`; `bench.py cmp fxpic_81 --row S4SNAK --commit <merge-base>`: every shot byte-equal.
- Speed gate (census): `PM_FNCOV=1 PM_ROW=S4SNAK PM_LANE=S4SNAK sh 3ds/build.sh`; `bench.py run fxfncov_base,fxfncov_81 --row S4SNAK
  --lane S4SNAK_fncov --why "S4 speed gate"`; `hotset.py --fncov <log> --fncovfn --lane S4SNAK` on both logs. Pass (effect minus
  base): nd_part_draw > 0; gv_tri, draw_tri, nd_geom_vtx, nd_ops_run and ent_gdl each < 20. Hand back the 15 largest deltas.
- Budget: 5 Azahar starts (1 base, 1 byte run, 2 census, 1 spare). Console: none here; the S4 batch console run after the merge.

## Stop
A failed gate or an edit that does not fit: stop, commit, hand back the output. Delete this row file in the completing commit.
