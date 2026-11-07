# Row GLOB: pm_gbi.c globals to their owners, then END-1 (dead walker cases)

## Goal
Two commits in one worktree. GLOB: pm_gbi.c ends with only includes (S10-1 then deletes it). END-1: walker case labels with no producer are deleted. Pixels: layout only.

## Start state
Worktree build3ds/wt/glob, branch glob from nds1 (after the RS3 merge fa6406ee7 and this row file). Copy assets/us and ver/us/build into it.

## GLOB (oracle plan 2, 2026-11-07; plan 1 premise "one writer per global" refuted by glob_writers.txt)
Premise: every extra writer of a multi-writer global is one of 4 kinds: save/restore of walker state (nd_comb.c comb_in/sv ~492-502, nd_tex_set.c ts_load ~590-593), reset at a list/window/frame start (nd_walk.c ~370, nd_cen.c ~536, pm_phase.c ~497/738), replay of the walk (nd_shape.c dlc groups, nd_ent.c ~374), generation bump (tex_gen).
Check first (no build): grep -n the extra writer lines in C:\Users\Andrew\AppData\Local\Temp\claude\c--Users-Andrew-Documents-GitHub-papermario-3ds-port\48214233-5ad6-4a37-b750-8bdf4cfcc789\scratchpad\glob_writers.txt and read them. Any other kind of write: stop and name it.
Owner rule (new D10): a global is defined in the module that sets it from the display-list words or by its own decision, else in its main hot reader. Other writers keep the extern in nd_int.h.
Commit 1, move (pure, D12; keep PM_HOTD on box_folded and any attribute; externs stay in nd_int.h sections):
- nd_geom.c: vert_live, vert_scr, vert_sx/sy, vert_ring/gmi/tss, tri_since_vtx, gv_fb, gv_load_n, gv_tri_n, gp1_*, n_cmd, n_in, n_drop, n_out, pm_nolight.
- nd_cen.c: n_cut, n_keep, sample_x, have_sample, stats_line, seen, op_cnt, rm_frame, pm_gbi_sync/gv/tex_ticks.
- nd_walk.c: pm_gbi_cur_op.
- nd_comb.c: dual_on, box_prod/folded/pat/save, comb_set, comb_gen, cmb_gen_seen, comb_alpha_shade, comb_dry, comb_mode5, leg_mode, sl_*, plan_active, cur_plan, sc_ok, sc_sig, ls_*, keyc, keys, conv_k4, aux_key, tst_f, dual_gain, cc_nm_*.
- nd_tex_set.c: tex_gen, ld_valid/x/y/h/w, ldrec_n, blk_n, m1, pm_gbi_bind_tile1.
- nd_target.c: cimg_stride. nd_mtx_set.c: lt, lt_n, lk. nd_rect.c: fold_axis.
- Delete dead (no reader, no outside writer): st_off, ident_m, mm_cnt, plan_draws, plan_fails.
- Keep sample_x, have_sample, n_keep, n_cut (the stats line reads them).
Commit 2: delete the write-only counters gv_xf_n, gv_rej_n, gp1_subset (nd_geom.c increments only, no reader).
Gate per commit: build; nd_host PASS; byte gates below; after commit 1 `grep -cE "^[a-zA-Z]" 3ds/src/pm_gbi.c` shows only #include lines; gbi-phase us at wcen_mac01 within +3% of base (C:\Users\Andrew\AppData\Local\Temp\claude\c--Users-Andrew-Documents-GitHub-papermario-3ds-port\48214233-5ad6-4a37-b750-8bdf4cfcc789\scratchpad\glob_base\gbi_phase_base.txt).
Base shots: already in C:\Users\Andrew\AppData\Local\Temp\claude\c--Users-Andrew-Documents-GitHub-papermario-3ds-port\48214233-5ad6-4a37-b750-8bdf4cfcc789\scratchpad\glob_base\ (taken at cf3098aef on lane glob). Do not take them again.

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
