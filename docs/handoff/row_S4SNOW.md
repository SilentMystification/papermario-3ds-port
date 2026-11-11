# Row S4SNOW: snowfall on the native particle path (NATIVEAPI S4 part 2; continues S4P in build3ds/wt/S4P, branch S4P)

(Model: Haiku. Do row S4P first; this row is its caller and its gates. PM_ROW=S4SNOW PM_LANE=S4P.)

## Design
snowfall_appendGfx (src/effects/snowfall.c:207-218): per part `gSPMatrix(T, PUSH|LOAD|MODELVIEW)`, `gSPMatrix(mtx, NOPUSH|MUL|MODELVIEW)`,
`gSPDisplayList(dlist)`, `gSPPopMatrix(MODELVIEW)`. The same macros write the same typed units into a local Gfx[4] (nd_gx_w writes the
unit when the packet is not gMainGfxPos, nd_rec.c:377); nd_parts_add takes them (pre 2, post 1). Everything else in the function stays.

## Touch only
src/effects/snowfall.c (snowfall_appendGfx), 3ds/tools/azt_scenarios.json and 3ds/tools/bench_defs.json (4 new entries each).

## Exact edits
1. snowfall.c: `#include "nd_api.h"` after the includes. In snowfall_appendGfx, `#ifdef TARGET_3DS NdParts pb; nd_parts_begin(&pb, 2, 1);
   #endif` before the part loop (after `data++;`). Inside `if (data->unk_30 <= 0 && data->unk_28 != 0) {` keep guTranslateF and
   guMtxF2L; replace the 4 gbi lines by `#ifdef TARGET_3DS` `{ Gfx g[4]; gSPMatrix(&g[0], &gDisplayContext->matrixStack[gMatrixListPos++],
   G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW); gSPMatrix(&g[1], mtx, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
   gSPDisplayList(&g[2], dlist); gSPPopMatrix(&g[3], G_MTX_MODELVIEW); nd_parts_add(&pb, (const uint32_t*)g); }` `#else` the 4 old
   lines `#endif`. After the loop, before the closing `gDPPipeSync`: `#ifdef TARGET_3DS nd_parts_end(&pb); #endif`.
2. Scenarios fxpic_63, fxpic_124, fxpic_50, fxpic_105 (snowfall, pink_sparkles, shimmer_burst, embers; rows S4PINK, S4SHIM, S4EMB use
   the last three): form of spot_mac01_det; debug `warp=mac_01:0 pos=0:0:200 fxtest=<i> shots shotwin=412:4 loglevel=debug det`,
   until `shot \d+ frame 415\b`, cap 40, deadline 35, first_marker `warp: mac_01 entry 0`, expects warp, needs_picture true.
   Benches of the same names (azahar_pic, normal, default false, tags azahar fxcen, until as the scenario). `bench.py rebuild`.
   Commit scenarios and benches together.

## Gate (CLAUDE.md 2-3)
- Build lane S4P, memcheck OK; nd_host PASS.
- `PM_ROW=S4SNOW PM_LANE=S4P python 3ds/tools/bench.py base fxpic_63` and `... base spot_mac01_det`; one list:
  `python 3ds/tools/bench.py run fxpic_63,spot_mac01_det --row S4SNOW --lane S4P --why "S4: snowfall parts as one
  NDC_NAPI record"`; `bench.py cmp fxpic_63 --row S4SNOW --commit $(git merge-base HEAD
  main)` and the same for spot_mac01_det: every shot byte-equal. Budget: at most 4 Azahar starts.
- Speed gate: row S4SNOWG (fncov class calls of fxfncov_63 on the census lane against FXFNCOV). Console: none in this row; one XL
  run `fxtest=all` after the batch merge (orchestrator), compared with xl4a (snowfall walk 18099 us above base).

## Stop
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, then hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash (branch S4P), the byte gate results, memcheck.
