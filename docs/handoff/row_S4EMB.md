# Row S4EMB: embers on the native particle path (NATIVEAPI S4)

(Model: Haiku. Starts after the S4P branch (rows S4P, S4SNOW, S4SNOWG, with hotset --fncovfn) is merged; runs in parallel with S4PINK and S4SHIM: different files.)

## Design
embers_appendGfx (src/effects/embers.c): per part i >= 1 `gSPMatrix(&stack[pos++], PUSH|MUL|MODELVIEW)`, `gDPSetTileSize(RENDERTILE,
cell of i % 4)`, `gDPSetTileSize(1, ...)`, `gSPDisplayList(D_E00E0A40[0])` (D_090003B0: 4 vertices, 2 triangles), `gSPPopMatrix`.
Same macros into a local Gfx[5], nd_parts_add with pre 3 (matrix, two tile sizes), post 1. The tile sizes stay ops (exact); the uv
step (cell offset as a per-instance uv offset) follows after this row's console numbers.

## Start
`sh 3ds/tools/wt_new.sh S4EMB`. PM_ROW=S4EMB PM_LANE=S4EMB on every build and bench call.

## Touch only
src/effects/embers.c (embers_appendGfx, the include list).

## Exact edits
1. `#include "nd_api.h"` after the includes. Before the part loop (after `part++;`): `#ifdef TARGET_3DS NdParts pb; nd_parts_begin(&pb,
   3, 1); #endif`.
2. In the loop replace the 5 gbi lines by `#ifdef TARGET_3DS` `{ Gfx g[5]; gSPMatrix(&g[0], ...the same args...); gDPSetTileSize(&g[1],
   ...the same args...); gDPSetTileSize(&g[2], ...the same args...); gSPDisplayList(&g[3], D_E00E0A40[0]); gSPPopMatrix(&g[4],
   G_MTX_MODELVIEW); nd_parts_add(&pb, (const uint32_t*)g); }` `#else` the 5 old lines `#endif`. After the loop, before the final
   gSPPopMatrix: `#ifdef TARGET_3DS nd_parts_end(&pb); #endif`.

## Gate (CLAUDE.md 2-3)
- Build, memcheck OK; nd_host PASS.
- `bench.py base fxpic_105`; `bench.py run fxpic_105 --row S4EMB --why "S4: embers parts as one NDC_NAPI record"`;
  `bench.py cmp fxpic_105 --row S4EMB --commit $(git merge-base HEAD main)`: every shot byte-equal. Budget: 5 Azahar starts (1 base + 1 byte run + 2 census + 1 spare).
- Speed gate (census, Azahar counts): `PM_FNCOV=1 PM_ROW=S4EMB PM_LANE=S4EMB sh 3ds/build.sh`; `bench.py run fxfncov_base,fxfncov_105 --row S4EMB
  --lane S4EMB_fncov --why "S4 speed gate: per-function calls"`; `hotset.py --fncov <log> --fncovfn --lane S4EMB` on both logs. Pass
  (per function, effect minus base): nd_part_draw > 0; gv_tri, draw_tri, nd_geom_vtx, nd_ops_run and ent_gdl each < 20 (the old
  per-part path is gone). Class sums are not a gate: call counts include tiny helpers that the normal build inlines (elem,
  ptr_res). Hand back the 15 largest deltas. Fail: stop, commit, hand back both files.
- Console: none in this row. One XL run `fxtest=all` after the batch merge (orchestrator), compared with xl4a walk per effect.

## Stop / At the end
A failed gate or an edit that does not fit: commit, hand back the output (CLAUDE.md 4-5). Delete this row file in the completing commit.
