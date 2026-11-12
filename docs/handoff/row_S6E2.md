# Row S6E2: entity setupGfx state lines as S6T compiled ranges (5 src/entity files)

(Model: Haiku. One pattern, five files: the S6S2 form, a def of static units with value placeholders and a local value packet.)

## Blocker and base
SCOMP0 merged (a PRIM unit after the last static unit is a value unit). `sh 3ds/tools/wt_new.sh S6E2`; PM_ROW=S6E2 PM_LANE=S6E2.

## Code read (oracle 4, main f4234c4a9)
The state lines recorded by these functions on core 1 (gMainGfxPos, typed ops). The gSPDisplayList and gSPPopMatrix lines stay.
- src/entity/Block.c entity_base_block_setupGfx :36-44 and src/entity/ItemBlock.c entity_ItemBlock_setupGfx :228-236 (same code):
  TextureLUT(G_TT_NONE), Texture(-1, -1, 0, G_TX_RENDERTILE, G_ON); alpha >= 255: RenderMode(G_RM_AA_ZB_OPA_SURF pair),
  CombineMode(G_CC_MODULATEIA pair); else CombineMode(PM_CC_01, PM_CC_02), PrimColor(0, 0, 0, 0, 0, entity->alpha).
- src/entity/SaveBlock.c entity_SaveBlock_setupGfx :38-41 and :63-66: Matrix(&matrixStack[gMatrixListPos++], PUSH | MUL |
  MODELVIEW), RenderMode (:39 G_RM_ZB_CLD_SURF pair; :64 `G_RM_AA_XLU_SURF | Z_CMP, G_RM_AA_XLU_SURF2 | Z_CMP`),
  CombineMode(PM_CC_01, PM_CC_02), PrimColor(0, 0, 0, 0, 0, alpha). The PipeSync at :60 is a no-op unit (dropped by the recorder).
- src/entity/WoodenCrate.c :193-197 (in the fragment loop) and src/entity/ShatteringBlock_common.c :174-178: opaque
  RenderMode(G_RM_AA_ZB_OPA_SURF pair), CombineMode(G_CC_MODULATEIA pair); else CombineMode(PM_CC_11, PM_CC_12), PrimColor(0, 0,
  0, 0, 0, <alpha expression of that line>).
Kinds: OMH, TEXTURE, OML, COMB are static; MTX is a value; PRIM after the last static is an ordered value (SCOMP0).

## Touch only
The five functions named above, a def block above each function, `#include "nd_scomp.h"` under `#ifdef TARGET_3DS` after the
includes of each file. 3ds/tools/azt_scenarios.json (one scenario inserted right after ent_kmr03_pic, :8142-8161) and
3ds/tools/bench_defs.json (one bench right after ent_kmr03_pic, :2823-2858). Insert there, never at the file end.

## Exact edits
1. Per file, above the function, under `#ifdef TARGET_3DS`: one `static const Gfx <file>_st_o[]` (opaque) and `<file>_st_t[]`
   (translucent) with the gs forms of the lines above in their order, a value slot written as its gs form with 0
   (`gsSPMatrix(0, G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW)`, `gsDPSetPrimColor(0, 0, 0, 0, 0, 0)`), and
   `static NdSDef <file>_def_o = { <file>_st_o, sizeof <file>_st_o / sizeof <file>_st_o[0] };` and the same for _t.
   SaveBlock has two defs: _a (:38-41) and _b (:63-66), both with the matrix and the prim slots.
2. In the function, under `#ifdef TARGET_3DS`, in place of the lines (the `#else` keeps them):
   - opaque branch without values: `nd_scomp_rec(&<file>_def_o, NULL, 0);`;
   - a branch with a PrimColor: `{ Gfx v[1]; gDPSetPrimColor(&v[0], 0, 0, 0, 0, 0, <same alpha expression>); nd_scomp_rec(&<file>_def_t, v, 1); }`;
   - SaveBlock: `{ Gfx v[2]; gSPMatrix(&v[0], &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL |
     G_MTX_MODELVIEW); gDPSetPrimColor(&v[1], 0, 0, 0, 0, 0, alpha); nd_scomp_rec(&SaveBlock_def_a, v, 2); }` for :38-41, and the
     same with _def_b for :63-66 (drop the PipeSync line :60 only under TARGET_3DS).
   Block.c and ItemBlock.c: the TextureLUT and Texture units are the first units of both defs (one call per branch).
3. Scenario `ent_kmr03_det` (copy ent_kmr03_pic): debug `warp=kmr_03:0 shots shotwin=200:6 loglevel=debug det`, until
   `shot \\d+ frame 205\\b`, cap 25, deadline 20, expected_events: the warp line and the until line. Bench `ent_kmr03_det`: kind
   azahar_pic, scenario ent_kmr03_det, cap 25, the same until, tags azahar, entity, pic; no log_metrics, gates []. One commit.

## Gate (expected values)
- `PM_ROW=S6E2 PM_LANE=S6E2 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. `bench.py run nd_host --row S6E2 --why
  "S6E2"`: PASS.
- `PM_ROW=S6E2 PM_LANE=S6E2 python 3ds/tools/bench.py base ent_kmr03_det` and the same for spot_mac01_det (bases run your
  scenario file at the merge-base; 2 starts for the new bench). Expected: stored, 6 shots.
- One list: `bench.py run ent_kmr03_det,spot_mac01_det,s6chk_mac01 --row S6E2 --why "S6E2 entity state ranges"` (3 starts).
  `bench.py cmp` ent_kmr03_det and spot_mac01_det: every shot byte-equal. s6chk: `bad 0`. No `scomp: def` line. Budget 8.

## Stop and end
A byte difference, `bad` above 0, a scomp error line, a base that is not stored, or a test-tool fault: commit, hand back the exact
output. Delete this row file in the completing commit. Hand back: the ready hash, the gate values, the memcheck line.
