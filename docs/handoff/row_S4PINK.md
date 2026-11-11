# Row S4PINK: pink_sparkles on the native particle path (NATIVEAPI S4)

(Model: Haiku. Starts after the S4P branch (rows S4P, S4SNOW, S4SNOWG) is merged; runs in parallel with S4SHIM and S4EMB: different files.)

## Design
pink_sparkles_appendGfx (src/effects/pink_sparkles.c:283-286): per part `gDPSetPrimColor(0, 0, r, g, b, 255)`, `gSPMatrix(&stack[pos++],
PUSH|MUL|MODELVIEW)`, `gSPDisplayList(D_E01248A0[unk_28 & 7])` (a 4-vertex, 2-triangle list: nd_parts_add accepts it), `gSPPopMatrix`.
The same macros into a local Gfx[4], nd_parts_add with pre 2 (prim, matrix), post 1. The prim op stays an op in this row (exact):
it still makes a new combine plan per part (PBFLUSH run 504363: pm_gpu_set_plan 53.8 flushes per frame). The colour step (prim as a
per-instance vertex colour) is the next S4 row after this one's console numbers.

## Start
`sh 3ds/tools/wt_new.sh S4PINK`. PM_ROW=S4PINK PM_LANE=S4PINK on every build and bench call.

## Touch only
src/effects/pink_sparkles.c (pink_sparkles_appendGfx, the include list).

## Exact edits
1. `#include "nd_api.h"` after the includes. Before the part loop: `#ifdef TARGET_3DS NdParts pb; nd_parts_begin(&pb, 2, 1); #endif`.
2. In the loop replace the 4 gbi lines by `#ifdef TARGET_3DS` `{ Gfx g[4]; gDPSetPrimColor(&g[0], 0, 0, primR, primG, primB, 255);
   gSPMatrix(&g[1], &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
   gSPDisplayList(&g[2], D_E01248A0[unk_28 & 7]); gSPPopMatrix(&g[3], G_MTX_MODELVIEW); nd_parts_add(&pb, (const uint32_t*)g); }`
   `#else` the 4 old lines `#endif`. After the loop: `#ifdef TARGET_3DS nd_parts_end(&pb); #endif`.

## Gate (CLAUDE.md 2-3)
- Build, memcheck OK; nd_host PASS.
- `bench.py base fxpic_124`; `bench.py run fxpic_124 --row S4PINK --why "S4: pink_sparkles parts as one NDC_NAPI record"`;
  `bench.py cmp fxpic_124 --row S4PINK --commit $(git merge-base HEAD main)`: every shot byte-equal. Budget: 5 Azahar starts (1 base + 1 byte run + 2 census + 1 spare).
- Speed gate (census, Azahar counts; needs the hotset `calls` line of S4SNOWG): `PM_FNCOV=1 PM_ROW=S4PINK PM_LANE=S4PINK sh 3ds/build.sh`;
  `bench.py run fxfncov_base,fxfncov_124 --row S4PINK --lane S4PINK_fncov --why "S4 speed gate: class calls"`; `hotset.py --fncov <log> --lane S4PINK`
  for each; delta per class = effect calls minus base calls; base within 5 % of the stored base (fxfncov_classes.txt) else tool
  fault. Pass: stored deltas (FXFNCOV) F +1743, I +696, A +419, D +5306, E +1373: F < 1307 (75 %), I < 174 (25 %), A < 335
  (80 %); D and E report only (the plan per part stays until S4COL). Fail: stop, commit, hand back both files.
- The pass numbers above wait for S4SNOWG step B (per-function census of snowfall); the oracle restates them before this row starts.
- Console: none in this row. One XL run `fxtest=all` after the batch merge (orchestrator), compared with xl4a walk per effect.

## Stop / At the end
A failed gate or an edit that does not fit: commit, hand back the output (CLAUDE.md 4-5). Delete this row file in the completing commit.
