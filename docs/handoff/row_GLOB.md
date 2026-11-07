# Row GLOB: pm_gbi.c globals to their owners, then END-1 (dead walker cases)

## Goal
Two commits in one worktree. GLOB: pm_gbi.c ends with only includes (S10-1 then deletes it). END-1: walker case labels with no producer are deleted. Pixels: layout only.

## Start state
Worktree build3ds/wt/glob, branch glob from nds1 (after the RS3 merge fa6406ee7 and this row file). Copy assets/us and ver/us/build into it.

## GLOB (oracle plan 2026-11-07)
Premise: pm_gbi.c holds only global definitions plus fold_axis and pm_gbi_bind_tile1.
Check first: `grep -nE "^[a-zA-Z].*\(.*\) *\{" 3ds/src/pm_gbi.c` lists only fold_axis and pm_gbi_bind_tile1. Otherwise stop and report.
Rule D10: the writing module owns a global. Find writers per global: `grep -nE "\b<name>\b *(=|\+\+|--|\+=|\|=|\[.*\] *=)" 3ds/src/*.c`. If a global has two writing modules, stop and report it (do not pick an owner).
Expected owners: vert_*, gv_fb/gv_*_n, gp1_*, tri_since_vtx -> nd_geom.c. lt, lt_n, lk, ident_m -> nd_mtx_set.c. seen, op_cnt, n_cmd..have_sample, stats_line, mm_cnt, rm_frame, plan_draws, plan_fails -> nd_cen.c. comb_*, dual_*, box_*, aux_key, tst_f, ls_*, pm_nolight, tex_gen -> the writer (nd_comb.c or nd_tex*.c). fold_axis -> nd_rect.c (keep its prototype in nd_int.h). pm_gbi_bind_tile1 -> nd_tex_set.c.
Pure cut and paste plus extern fixes (D12). Same initializers, same types.
Gate: build; nd_host PASS; `grep -cE "^[a-zA-Z]" 3ds/src/pm_gbi.c` shows only #include lines; byte gates below; gbi-phase us at wcen_mac01 prof=4 opsplit within +3% of base.

## END-1
Premise: a walker case with no macro producer in src or include is dead.
Check first (0 starts): for each case label in nd_walk.c find its gbi.h macro and grep src and include. Expected candidate: G_TEXRECTFLIP only (used only in include/PR/gbi.h).
Runtime proof: add a cold `ophit` line in nd_cen.c (debug tool, stays): sum os_op_n[256] over the whole run and print the non-zero ops at the end. Run tour10_det, wcen_golden, wcen_battle, wcen_nok11 once each. Delete a case only when it has 0 hits AND no macro caller.
Gate: build; nd_host PASS; byte gates below.

## Byte gates (cold det, cmp against base shots you take at the start commit on lane glob)
spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015, pause_spin_grab_det_w300. Not gates: spot_mac00_*, super_text_pic, nok11 shot 0016.

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. PM_ROW=GLOB PM_LANE=glob on every build.sh and bench call. Budget 8 starts, reset when needed (powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget GLOB). No git stash. Never commit runs.jsonl or 3ds/src/map_restore_table.c. Wait for runs in the foreground.
Build: python 3ds/tools/gen_map_restore.py; PM_ROW=GLOB PM_LANE=glob sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.

## At the end
Delete this row file and docs/handoff/plan_walker_end.md (its last row is END-1; remove its INDEX.md line) in the END-1 commit. Report in 5 lines.
