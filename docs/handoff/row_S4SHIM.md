# Row S4SHIM: shimmer_burst on the native particle path (NATIVEAPI S4)

(Model: Haiku. Starts after the S4P branch (rows S4P, S4SNOW, S4SNOWG) is merged; runs in parallel with S4PINK and S4EMB: different files.)

## Design
shimmer_burst_appendGfx (src/effects/shimmer_burst.c): inside the nd_sub range (replayed 3 times by nd_sub_call; an NDC_NAPI record in a
range replays with it, nd_rec.c stream_run), per part `gSPMatrix(&stack[pos++], PUSH|MUL|MODELVIEW)`, `gDPSetPrimColor(0, 0, unk_50,
unk_54, unk_58, part->unk_4C * temp_f20)`, `gSPDisplayList(D_E0070CD0[i & 7])` (4 vertices, 2 triangles), `gSPPopMatrix`. Same macros
into a local Gfx[4], nd_parts_add with pre 2 (matrix, prim), post 1. The prim stays an op (exact); the colour step follows.

## Start
`sh 3ds/tools/wt_new.sh S4SHIM`. PM_ROW=S4SHIM PM_LANE=S4SHIM on every build and bench call.

## Touch only
src/effects/shimmer_burst.c (shimmer_burst_appendGfx, the include list).

## Exact edits
1. `#include "nd_api.h"` after the includes. Right after `nd_sub_open(&sub);` (inside its `#ifdef TARGET_3DS`): `NdParts pb;
   nd_parts_begin(&pb, 2, 1);` (declare `NdParts pb;` next to `NdSub sub;`).
2. In the loop replace the 4 gbi lines by `#ifdef TARGET_3DS` `{ Gfx g[4]; gSPMatrix(&g[0], &gDisplayContext->matrixStack[gMatrixListPos++],
   G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW); gDPSetPrimColor(&g[1], 0, 0, unk_50, unk_54, unk_58, part->unk_4C * temp_f20);
   gSPDisplayList(&g[2], D_E0070CD0[i & 7]); gSPPopMatrix(&g[3], G_MTX_MODELVIEW); nd_parts_add(&pb, (const uint32_t*)g); }`
   `#else` the 4 old lines `#endif`. Before `nd_sub_close(&sub);`: `nd_parts_end(&pb);` (the record must be inside the range).

## Gate (CLAUDE.md 2-3)
- Build, memcheck OK; nd_host PASS.
- `bench.py base fxpic_50`; `bench.py run fxpic_50 --row S4SHIM --why "S4: shimmer_burst parts as one NDC_NAPI record"`;
  `bench.py cmp fxpic_50 --row S4SHIM --commit $(git merge-base HEAD main)`: every shot byte-equal. Budget: 5 Azahar starts (1 base + 1 byte run + 2 census + 1 spare).
- Speed gate (census, Azahar counts; needs the hotset `calls` line of S4SNOWG): `PM_FNCOV=1 PM_ROW=S4SHIM PM_LANE=S4SHIM sh 3ds/build.sh`;
  `bench.py run fxfncov_base,fxfncov_50 --row S4SHIM --lane S4SHIM_fncov --why "S4 speed gate: class calls"`; `hotset.py --fncov <log> --lane S4SHIM`
  for each; delta per class = effect calls minus base calls; base within 5 % of the stored base (fxfncov_classes.txt) else tool
  fault. Pass: stored deltas F +3596, I +1084, A +819: F < 2697 (75 %), I < 271 (25 %), A < 655 (80 %); D report only. Fail: stop, commit, hand back both files.
- The pass numbers above wait for S4SNOWG step B (per-function census of snowfall); the oracle restates them before this row starts.
- Console: none in this row. One XL run `fxtest=all` after the batch merge (orchestrator), compared with xl4a walk per effect.

## Stop / At the end
A failed gate or an edit that does not fit: commit, hand back the output (CLAUDE.md 4-5). Delete this row file in the completing commit.
