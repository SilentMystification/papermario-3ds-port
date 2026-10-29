# agent_modelops: MODELOPS epic (plan_model_ops R0 to R3)

## State
- R0 done. Commit 25d191c70. Counters (mops_count, mops_report in 3ds/src/pm_gbi.c) and scenario mac01 (azt_scenarios.json, catalog row).
- R1, R2, R3 not implemented. Reason: the R0 counts give a ceiling below each gate (below). A row that cannot reach its gate is not coded.
- R4: gate not met. 0a per bind is 27 us (Azahar, 2121 us / 78 binds), gate needs > 40 us.
- R5: gate open. lt_same 160 per frame, gate needs > 60. Not started.

## R0 baseline (mac01, lane mops, build 25d191c70, phase line f299, Azahar, per frame)
- 0a (G_PM_TEX) 2121 us, 78 binds. cc 571 us, geo 570 us (cc+geo 1141 us). mtx 481 us, 110 ops. tex class 2529 us.
- Counts: bind_w1 7 (w1 equal to the previous bind, no tex_gen check, so an upper bound), cc_same 35, tex_same 15, geo_same 12, mtx_same 1, lt 160, lt_same 160.
- Two runs of mac01 give byte-identical shots and identical log counts (deterministic).
- Picture: Toad Town post office, Mario and Toads (both shots). Baseline shots: build3ds/azt/shots/mac01/0000_1_2.ppm, 0001_1_2.ppm.
- Note: the until line is the first mops line. The regex cannot count to the second report.

## Gate ceilings from R0 (upper bounds, Azahar us per frame)
- R1 (gate 0a down >= 35 x bind_same): at most 7 skips, each at most the 27 us average. Max 190 us. Gate 245 us. Fails. R1 also needs every tex_gen writer site (plan section R1).
- R2 (gate cc+geo down >= 500): cc_same 35 x 8.9 us + geo_same 12 x 1.8 us. Max about 335 us. Gate 500 us. Fails.
- R2 exactness: sync_combine skips only when the sig matches (line ~1051), and box_fold_sync rewrites comb_aa..ad (lines ~562 to 571). A skip needs both checks. Not written.
- R3 (gate mtx down >= 175): mtx_same 1 per frame, about 4 us. Fails.

## Open items
- Decide: R1 to R3 are closed for mac01 by the counts. Do a new plan for the mac01 per-model cost (dlc replay ff 1081 us, 0a, vtx/tri) or a different scene.
- R5: lt_same 160 per frame passes the R0 gate. Needs a row plan (MOVEMEM light skip in g2_same, guard lt[] writes).
- Console check is not run. No push.

## Commands
- Build: python 3ds/tools/gen_map_restore.py, then PM_LANE=mops sh 3ds/build.sh, then restore 3ds/src/map_restore_table.c from the user copy.
- Test: PM_ROW=MODELOPS_R0 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 mac01 -Row MODELOPS_R0 -Build mops -Try
- Azahar starts used on MODELOPS_R0: 2 of 8.
