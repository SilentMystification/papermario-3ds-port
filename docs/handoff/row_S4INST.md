# Row S4INST: energy_shockwave draw list through a one-instance particle run (setter form)

(Model: Haiku. One function, one exact edit. Rewritten by ORACLE5 after the row spec fault of try 1: the old text named two
effects and predated S4MULTI. Budget reset once (row spec fault). Ring blast is row_S4INST2.md.)

## Decision (oracle 5)
- One run per appendGfx call (begin, add, end in energy_shockwave_appendGfx). No run across render tasks: with the setup list
  D_09000E60 per instance (D_09000DC0 clears G_SHADING_SMOOTH that E60 sets), a joined run must carry Segment and CALL E60
  in its units, which forces the slower nd_parts_run form (every pre unit through nd_walk_state). One NDC_NAPI header per task
  costs less. S7EFX replaces the setup list later.
- No change to nd_rec.c or nd_walk.c. PipeSync, Segment and CALL E60 stay ordinary ops before the run; the trailing PipeSync
  stays an op after it. The run units are PRIM, ENV, TILESIZE, TILESIZE, MTX, CALL DC0, POPMTX: every unit but the call is a
  setter id, so nd_parts_end (nd_rec.c:325-337) records nd_parts_run_v (nd_walk.c:469).
- DC0 (two vertex loads, 32 and 28 at slot 0, triangles) passes parts_ok since S4MULTI (nd_rec.c:283-306).

## Start
Worktree build3ds/wt/S4INST exists (branch S4INST). First `git merge main` in it. PM_ROW=S4INST PM_LANE=S4INST on every call.

## Touch only
src/effects/energy_shockwave.c (energy_shockwave_appendGfx), 3ds/tools/azt_scenarios.json and 3ds/tools/bench_defs.json
(new entry fxpic_51 after fxpic_50 in each).

## Exact edits
1. energy_shockwave.c: replace the 5 lines from `gDPSetPrimColor(...)` through `gDPSetEnvColor(...)` and the 2 gDPSetTileSize
   lines and the 3 lines gSPMatrix / gSPDisplayList(D_09000DC0_3762D0) / gSPPopMatrix with this form (the uls/ult arithmetic and
   the guTranslateF/guScaleF/guMtxCatF/guMtxF2L lines stay where they are, before the block; the block goes where gSPMatrix was):
   ```c
   #ifdef TARGET_3DS
       {
           NdParts pb;
           Gfx g[7];
           gDPSetPrimColor(&g[0], 0, 0, 255, 179, 30, data->unk_24);
           gDPSetEnvColor(&g[1], data->unk_50, data->unk_51, data->unk_52, 128);
           gDPSetTileSize(&g[2], G_TX_RENDERTILE, uls, ult, uls + 252, ult + 252);
           gDPSetTileSize(&g[3], 1, uls2, ult2, uls2 + 124, ult2 + 124);
           gSPMatrix(&g[4], &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
           gSPDisplayList(&g[5], D_09000DC0_3762D0);
           gSPPopMatrix(&g[6], G_MTX_MODELVIEW);
           nd_parts_begin(&pb, 5, 1);
           nd_parts_add(&pb, (const uint32_t*)g);
           nd_parts_end(&pb);
       }
   #else
       <the original prim, env, 2 tile size, matrix, display list and pop lines, unchanged, in their original order>
   #endif
   ```
   The original order puts prim and env before the uls arithmetic: under TARGET_3DS they move into the block (state setters before
   the same draw: no order change for the walker). Add `#include "nd_api.h"` as line 3 (after `#include "effects_internal.h"`),
   the form of src/effects/effect_3D.c:3.
2. azt_scenarios.json and bench_defs.json: copy the fxpic_50 entries to fxpic_51, insert after fxpic_50; change `fxtest=50` to
   `fxtest=51`, shimmer_burst to energy_shockwave, effect 50 to effect 51. Commit them with step 1 (one commit).

## Gate (expected values)
- `sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. `bench.py run nd_host --row S4INST --why "S4INST nd_host"`: PASS.
- `bench.py base fxpic_51` and `bench.py base spot_mac01_det` (merge-base of HEAD with main). One list:
  `bench.py run fxpic_51,spot_mac01_det --row S4INST --why "S4INST: shockwave list via nd_parts_run_v"` (2 starts).
  `bench.py cmp fxpic_51 --row S4INST`: 4/4 byte-equal. `bench.py cmp spot_mac01_det --row S4INST`: 6/6 byte-equal.
- Census in the fxpic_51 run log: the first `partsrun f4\d\d v N old M` line has N >= 1 and M = 0 (pr_cen, nd_walk.c:417).
- Budget 8 Azahar starts (reset once).

## Stop
A byte difference: stop, commit, hand back the cmp lines (2 tries per named cause). `old` above 0: the record took nd_parts_run:
hand back the unit ids of g[] (stop). Delete this row file in the completing commit.
