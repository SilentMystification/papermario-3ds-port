# Row S4EMB: embers on the native particle path (NATIVEAPI S4)

(Model: Haiku. Starts after the S4P branch (rows S4P, S4SNOW, S4SNOWG) is merged; runs in parallel with S4PINK and S4SHIM: different files.)

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
- Speed gate (census, Azahar counts; needs the hotset `calls` line of S4SNOWG): `PM_FNCOV=1 PM_ROW=S4EMB PM_LANE=S4EMB sh 3ds/build.sh`;
  `bench.py run fxfncov_base,fxfncov_105 --row S4EMB --lane S4EMB_fncov --why "S4 speed gate: class calls"`; `hotset.py --fncov <log> --lane S4EMB`
  for each; delta per class = effect calls minus base calls; base within 5 % of the stored base (fxfncov_classes.txt) else tool
  fault. Pass: stored deltas F +2594, I +808, A +1166: F < 1945 (75 %), I < 202 (25 %), A < 1049 (90 %: three pre ops per part);
  D report only. Fail: stop, commit, hand back both files.
- The pass numbers above wait for S4SNOWG step B (per-function census of snowfall); the oracle restates them before this row starts.
- Console: none in this row. One XL run `fxtest=all` after the batch merge (orchestrator), compared with xl4a walk per effect.

## Stop / At the end
A failed gate or an edit that does not fit: commit, hand back the output (CLAUDE.md 4-5). Delete this row file in the completing commit.
