# Row S4E3D: effect_3D on the native particle path (NATIVEAPI S4)

(Model: Haiku: the S4SHIM/S4EMB pattern, one loop, no design choice. Runs in parallel with S4SNAK, S4WBLK, FSELCEN: different files
and different insert points in the two JSON files.)

## Design
effect_3D_appendGfx (src/effects/effect_3D.c:217-252): after the outer `gSPMatrix(... G_MTX_PUSH | G_MTX_LOAD ...)` (:230), per part
(:233-248) `gDPSetPrimColor(0, 0, 200, 255, 255, part->unk_58)`, the float matrix arithmetic, `gSPMatrix(&stack[pos++], PUSH|MUL|
MODELVIEW)`, `gSPDisplayList(D_090002E8_37D468)`, `gSPPopMatrix`. Same macros into a local Gfx[4], nd_parts_add with pre 2 (prim,
matrix), post 1 (pop). No nd_sub range in this function: begin and end bracket the loop (as embers.c).

## Start
`sh 3ds/tools/wt_new.sh S4E3D`. PM_ROW=S4E3D PM_LANE=S4E3D on every build and bench call. Base: `git merge-base HEAD main` (main is
20d613506 at writing).

## Touch only
src/effects/effect_3D.c (effect_3D_appendGfx, the include list); 3ds/tools/azt_scenarios.json and 3ds/tools/bench_defs.json: one new
entry each, inserted right after the `fxpic_124` entry.

## Exact edits
1. `#include "nd_api.h"` after the includes.
2. After the `gSPMatrix(... G_MTX_LOAD ...)` line (:230): `#ifdef TARGET_3DS` `NdParts pb; nd_parts_begin(&pb, 2, 1);` `#endif`.
3. In the loop: keep the matrix arithmetic. Replace the prim line by nothing on 3DS and the 3 lines at :245-247 by
   `#ifdef TARGET_3DS` `{ Gfx g[4]; gDPSetPrimColor(&g[0], 0, 0, 200, 255, 255, part->unk_58); gSPMatrix(&g[1],
   &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW); gSPDisplayList(&g[2], D_090002E8_37D468);
   gSPPopMatrix(&g[3], G_MTX_MODELVIEW); nd_parts_add(&pb, (const uint32_t*)g); }` `#else` the old 3 lines `#endif`. The prim line at
   :234 goes inside `#ifndef TARGET_3DS` ... `#endif` (the 3DS prim is g[0]; the order prim, matrix, list, pop is unchanged).
4. After the loop, before the final `gSPPopMatrix` (:250): `#ifdef TARGET_3DS` `nd_parts_end(&pb);` `#endif`.
5. Scenario and bench `fxpic_55`: copies of `fxpic_50` with `fxtest=55` and purpose "S4 byte gate: effect_3D (effect 55) ...".
   `fxfncov_55` exists.

## Gate
- Build, memcheck OK; nd_host PASS.
- `bench.py base fxpic_55 --commit <merge-base>`; `bench.py run fxpic_55 --row S4E3D --why "S4: effect_3D parts as one NDC_NAPI record"`;
  `bench.py cmp fxpic_55 --row S4E3D --commit <merge-base>`: every shot byte-equal.
- Speed gate (census): `PM_FNCOV=1 PM_ROW=S4E3D PM_LANE=S4E3D sh 3ds/build.sh`; `bench.py run fxfncov_base,fxfncov_55 --row S4E3D
  --lane S4E3D_fncov --why "S4 speed gate"`; `hotset.py --fncov <log> --fncovfn --lane S4E3D` on both logs. Pass (effect minus base):
  nd_part_draw > 0; gv_tri, draw_tri, nd_geom_vtx, nd_ops_run and ent_gdl each < 20. Hand back the 15 largest deltas.
- Budget: 5 Azahar starts (1 base, 1 byte run, 2 census, 1 spare). Console: none here; the S4 batch console run after the merge.

## Stop
A failed gate or an edit that does not fit: stop, commit, hand back the output. Delete this row file in the completing commit.
