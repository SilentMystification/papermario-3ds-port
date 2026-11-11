# Row S4EMB: embers on the native particle path (NATIVEAPI S4)

(Model: Haiku. Starts after the S4P branch (rows S4P + S4SNOW) is merged; runs in parallel with S4PINK and S4SHIM: different files.)

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
  `bench.py cmp fxpic_105 --row S4EMB --commit $(git merge-base HEAD main)`: every shot byte-equal. Budget: 3 Azahar starts.
- Console gate (orchestrator, before the merge): `warp=mac_01:0 pos=0:0:200 fxtest=105,105,105,105` on the merge-base build and on
  lane S4EMB, two runs each; fxcen_report.py. Pass: embers walk lower on S4EMB in both runs, runs of one build within 15 %.

## Stop / At the end
A failed gate or an edit that does not fit: commit, hand back the output (CLAUDE.md 4-5). Delete this row file in the completing commit.
