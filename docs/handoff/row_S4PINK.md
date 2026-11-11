# Row S4PINK: pink_sparkles on the native particle path (NATIVEAPI S4)

(Model: Haiku. Starts after the S4P branch (rows S4P + S4SNOW) is merged; runs in parallel with S4SHIM and S4EMB: different files.)

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
  `bench.py cmp fxpic_124 --row S4PINK --commit $(git merge-base HEAD main)`: every shot byte-equal. Budget: 3 Azahar starts.
- Console gate (orchestrator, before the merge): `warp=mac_01:0 pos=0:0:200 fxtest=124,124,124,124` on the merge-base build and on
  lane S4PINK, two runs each; fxcen_report.py. Pass: pink_sparkles walk lower on S4PINK in both runs, runs of one build within 15 %.

## Stop / At the end
A failed gate or an edit that does not fit: commit, hand back the output (CLAUDE.md 4-5). Delete this row file in the completing commit.
