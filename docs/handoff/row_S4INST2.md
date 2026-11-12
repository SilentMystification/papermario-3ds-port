# Row S4INST2: ring_blast draw list through a one-instance particle run (setter form)

(Model: Haiku. The pattern of row_S4INST.md in one more effect. Blocker: S4INST merged (fxpic_51 entries sit next to the new
fxpic_31 entries in the same json files). Explosion and snowman_doll get no S4 row: explosion's drawn part lists
D_E00328B0[] start with a texture load and its D_090008F0 part is off in fxtest (type 0 sets unk_00 = -1, explosion.c:56);
snowman_doll's D_09003818 is a pure list call. Both go to S7EFX.)

## Code read
- ring_blast_appendGfx (src/effects/ring_blast.c:86-144): PipeSync, Segment 0x09, CALL dlist2 (D_09001040: setup), MTX (push
  mul), then for unk_00 == 0: PRIM, ENV, KEYR, KEYGB, COMBINE (PM_CC_4A, PM_CC_4B); else COMBINE (PM_CC_4A, PM_CC_4C), PRIM, ENV;
  then 2 TILESIZE, CALL dlist (D_090010F8: one load of 4 at slot 0, SP2Triangles, PipeSync, End: passes parts_ok), POPMTX.
- KEYR, KEYGB and COMBINE are not setter ids (nd_rec.c:334): they stay ops before the run, so the run is the setter form.
  The state setters all precede the one draw: moving MTX after them changes no walker result (no draw between).

## Start
`sh 3ds/tools/wt_new.sh S4INST2`. PM_ROW=S4INST2 PM_LANE=S4INST2 on every call.

## Touch only
src/effects/ring_blast.c (includes, ring_blast_appendGfx), 3ds/tools/azt_scenarios.json and 3ds/tools/bench_defs.json (new
entry fxpic_31 right before fxpic_50 in each).

## Exact edits
1. ring_blast.c: add `#include "nd_api.h"` as line 3.
2. ring_blast_appendGfx: the lines up to and including `guMtxF2L(sp20, ...)` stay. Replace every line from
   `gSPMatrix(gMainGfxPos++, ...)` to `gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);` (the end of the function) with
   ```c
   #ifdef TARGET_3DS
       {
           NdParts pb;
           Gfx g[7];
           s32 uls = unk_20_s32 * 32 * 4;
           if (unk_00 == 0) {
               f32 var_f4 = 1.0f;
               if (data->unk_20 > 5.0f) { var_f4 = 1.0f - (data->unk_20 - 5.0f); if (var_f4 < 0.0f) var_f4 = 0.0f; }
               gDPSetKeyR(gMainGfxPos++, var_f4 * 211.0f, 0, 0);
               gDPSetKeyGB(gMainGfxPos++, var_f4 * 255.0f, 0, 0, var_f4 * 216.0f, 0, 0);
               gDPSetCombineMode(gMainGfxPos++, PM_CC_4A, PM_CC_4B);
               gDPSetPrimColor(&g[1], 0, 0, var_f4 * 255.0f, var_f4 * 18.0f, var_f4 * 59.0f, 0);
               gDPSetEnvColor(&g[2], var_f4 * 255.0f, var_f4 * 53.0f, var_f4 * 24.0f, envAlpha);
           } else {
               gDPSetCombineMode(gMainGfxPos++, PM_CC_4A, PM_CC_4C);
               gDPSetPrimColor(&g[1], 0, 0, 255, 255, 40, 127);
               gDPSetEnvColor(&g[2], 255, 255, 139, envAlpha);
           }
           gSPMatrix(&g[0], &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
           gDPSetTileSize(&g[3], G_TX_RENDERTILE, uls, 0, uls + 32 * 4, 32 * 4);
           if (cond) gDPSetTileSize(&g[4], 1, uls + 32 * 4, 32 * 4, uls + 64 * 4, 64 * 4);
           else gDPSetTileSize(&g[4], 1, uls + 32 * 4, 0, uls + 64 * 4, 32 * 4);
           gSPDisplayList(&g[5], dlist);
           gSPPopMatrix(&g[6], G_MTX_MODELVIEW);
           nd_parts_begin(&pb, 5, 1);
           nd_parts_add(&pb, (const uint32_t*)g);
           nd_parts_end(&pb);
       }
   #else
       <the original lines from gSPMatrix to gSPPopMatrix, unchanged>
   #endif
   ```
   The tile size words equal the original ((unk_20_s32 * 32) * 4 = uls; (unk_20_s32 * 32 + 32) * 4 = uls + 128; + 64: uls + 256).
3. json: copy the fxpic_50 entries to fxpic_31 (fxtest=31, ring_blast, effect 31), insert before fxpic_50. Same commit.

## Gate (expected values)
- Build, memcheck OK; `bench.py run nd_host --row S4INST2 --why "S4INST2 nd_host"`: PASS.
- `bench.py base fxpic_31`, `bench.py base spot_mac01_det`; one list `bench.py run fxpic_31,spot_mac01_det --row S4INST2 --why
  "S4INST2: ring_blast list via nd_parts_run_v"` (2 starts). cmp fxpic_31 4/4 byte-equal, spot_mac01_det 6/6 byte-equal.
- fxpic_31 run log: the first `partsrun f4\d\d v N old M` line has N = 1, M = 0 (fx_ring_blast lives 24 frames from the spawn at
  about 402: shots 412-415 see it).
- Budget 8 Azahar starts.

## Stop
A byte difference: stop, commit, hand back the cmp lines (2 tries per named cause). Delete this row file in the completing commit.
