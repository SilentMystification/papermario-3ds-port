# Row S6E3: HeartBlock and SuperBlock content state lines as S6T compiled ranges

(Model: Haiku. The S6E2 pattern in two more src/entity files. Exact defs below.)

## Blocker and base
SCOMP0 merged (PRIM after the last static unit is a value unit). May run with S6E2 (other files). `sh 3ds/tools/wt_new.sh S6E3`;
PM_ROW=S6E3 PM_LANE=S6E3 on every build and bench call.

## Code read (oracle 4, main 2fdcf9a8b; re-grep, lines may move)
- src/entity/HeartBlock.c entity_HeartBlockContent__setupGfx (:28-71): CombineMode(PM_CC_01, PM_CC_02) + PrimColor(entity->alpha)
  (:37-38); then twice: Matrix(PUSH | MUL | MODELVIEW) + RenderMode(RM_HB, the long AA_EN | Z_CMP | IM_RD ... G_BL_1MA pair) (:48-49,
  :64-65), DisplayList(dlist), PipeSync, CombineMode(PM_CC_01, PM_CC_02) + PrimColor(alpha) (:52-53, :68-69), PopMatrix.
- src/entity/SuperBlock.c entity_SuperBlockContent_setupGfx (:113-181): Matrix + RenderMode(G_RM_AA_ZB_TEX_EDGE pair) +
  CombineMode(G_CC_MODULATEIA pair) (:142-144), LoadTLUT_pal16 (:145), DisplayList, PopMatrix; then twice: Matrix +
  RenderMode(RM_HB) + CombineMode(PM_CC_01, PM_CC_02) + PrimColor(alpha) (:158-161, :174-177), DisplayList, PopMatrix.
- The DisplayList, PopMatrix, PipeSync and LoadTLUT_pal16 lines stay ops (the TLUT stays after its record, in the same order).

## Touch only
The two functions, a def block above each, `#include "nd_scomp.h"` under `#ifdef TARGET_3DS` after each file's includes.

## Exact edits (under `#ifdef TARGET_3DS`; `#else` keeps the old lines)
1. HeartBlock.c, above the function: `hb_cp_u[] = { gsDPSetCombineMode(PM_CC_01, PM_CC_02), gsDPSetPrimColor(0, 0, 0, 0, 0, 0) }`
   and `hb_mr_u[] = { gsSPMatrix(0, G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW), gsDPSetRenderMode(<the :49 pair, copied exactly>) }`
   (static const Gfx), with `static NdSDef hb_cp = { hb_cp_u, 2 }, hb_mr = { hb_mr_u, 2 };`.
2. In the function: :37-38 -> `{ Gfx v[1]; gDPSetPrimColor(&v[0], 0, 0, 0, 0, 0, entity->alpha); nd_scomp_rec(&hb_cp, v, 1); }`;
   :48-49 and :64-65 -> `{ Gfx v[1]; gSPMatrix(&v[0], &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL |
   G_MTX_MODELVIEW); nd_scomp_rec(&hb_mr, v, 1); }`; :52-53 and :68-69 -> the hb_cp form with `alpha`.
3. SuperBlock.c, above the function: `sb_a_u[] = { the matrix unit, gsDPSetRenderMode(G_RM_AA_ZB_TEX_EDGE, G_RM_AA_ZB_TEX_EDGE2),
   gsDPSetCombineMode(G_CC_MODULATEIA, G_CC_MODULATEIA) }` and `sb_b_u[] = { the matrix unit, gsDPSetRenderMode(<the :159 pair>),
   gsDPSetCombineMode(PM_CC_01, PM_CC_02), gsDPSetPrimColor(0, 0, 0, 0, 0, 0) }`; `static NdSDef sb_a = { sb_a_u, 3 }, sb_b = {
   sb_b_u, 4 };`.
4. In the function: :142-144 -> `{ Gfx v[1]; gSPMatrix(&v[0], ...same args...); nd_scomp_rec(&sb_a, v, 1); }` (the LoadTLUT
   line :145 stays right after it); :158-161 and :174-177 -> `{ Gfx v[2]; gSPMatrix(&v[0], ...); gDPSetPrimColor(&v[1], 0, 0, 0,
   0, 0, alpha); nd_scomp_rec(&sb_b, v, 2); }`.
   Keep `gMatrixListPos++` exactly once per Matrix, inside the gSPMatrix call as before.

## Gate (expected values)
- `PM_ROW=S6E3 PM_LANE=S6E3 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. `bench.py run nd_host --row S6E3
  --why "S6E3"`: PASS.
- Try 2 (oracle 5): sbk56_orbdbg_det is not deterministic (two base runs differed at shot 0000 = frame 520, the padscript hit
  scene): it is no gate. The super block content (entity_SuperBlockContent_setupGfx, the orb inside the block) draws in the
  plain spot scene, which is det-proven: spot_sbk56_det (warp=sbk_56:0 pos=20:0:440 det, shots 200-205).
- New scenario and bench `s6chk_sbk56`, inserted right after `spot_sbk56_det` in azt_scenarios.json and bench_defs.json (one
  commit): debug `warp=sbk_56:0 pos=20:0:440 shots shotwin=200:1 loglevel=debug det s6chk`, until `s6chk f2\d\d ranges \d+ bad \d+`,
  cap 30, deadline 25, expected_events only `warp: sbk_56 entry 0` (both builds print it); bench as s6chk_mac01's.
- `bench.py base spot_sbk56_det` and `bench.py base s6chk_sbk56` (new bench: 2 runs). One list `bench.py run
  spot_sbk56_det,s6chk_sbk56 --row S6E3 --why "S6E3 try 2: block content ranges"` (2 starts). cmp spot_sbk56_det 6/6 byte-equal;
  s6chk_sbk56 head `bad 0` and `ranges` above the base log's value (the content records of the drawn block). Equal ranges mean
  the content is not drawn there: stop and hand back both values (then the gate is build, nd_host and s6chk, as row_S6M4.md).
- HeartBlock: no det scene draws a heart block; its defs are the unit kinds of SuperBlock's (proof: build and nd_host; say so).
- No `scomp: def` line. Budget 8 Azahar starts after the reset (row spec fault).

## Stop and end
A byte difference, `bad` above 0, a scomp error line, an edit that does not fit, or a test-tool fault: commit, hand back the exact
output. Delete this row file in the completing commit. Hand back: the ready hash, the gate values, the memcheck line.
