# Row PM5: lit and fogged (program 5) draws go native

## Goal
Battle effect_renderUI draws 1800 walker batches per 300 frames (wcen_battle, why pm5: g_use5 batches, program 5 with lighting and fog; graw_is_pm4 refuses them in nd_raw_native). Make them native, then delete the citro3d pm5 path. Plan: docs/handoff/plan_walker_end.md rows PM5-1 to PM5-4 and decisions D7 (reuse the prog5 register layout, no new shader), D9.

## Start state
nds1 at 113f8a243 (RECT-1b and RECT-2 census merged). Work in worktree build3ds/wt/rect, branch rect: git rebase nds1 first (or a new worktree build3ds/wt/pm5 -b pm5 nds1). Copy assets/us and ver/us/build if new. Build there, azt -Build <worktree 3dsx>.
The super block agent edits nd_tmpl.c tev_op_* in the shared folder: you own only the program/attr/uniform build parts of nd_tmpl.c, nd_emit.c, and in pm_gpu.c graw_is_pm4 and the g_use5 block of flush_draw. Do not change tev_op_*.

## Rows (one commit each)
PM5-1 TEMPLATE: NdTmplKey gets use5; nd_tmpl.c builds prog5, attr4 and the pm5 uniform block (lflag, fogu, light table, 7 registers per palette entry), same layout as the citro3d g_use5 path. Host test first: nat_host_test emitted words vs an ndref capture of one pm5 batch (wcen_battle effect_renderUI).
PM5-2 RAW: graw_is_pm4 accepts g_use5 for the native close. Gate: wcen_battle effect_renderUI wd 1800 to 0 (why pm5 0); battle shots right by eye (battle_start, battle_menu_bar); spot_mac01_pic byte-equal.
PM5-3 MODEL: nd_model_state drops why 4 (gv->fxon to the pm5 template). Gate: ndm why 4 = 0 at iwa_10 (one ndmcheck run), by eye, spot_mac01_pic byte-equal.
PM5-4 DELETE: the g_use5 citro3d block of flush_draw, sh5_* and u5_* without callers.
Hand back with the branch head after PM5-4 or a failed gate.

## Rules
CLAUDE.md rules 5, 7, 9. No toggles (debug switches allowed, removed in PM5-4), no stubs, no push, no attribution lines, never commit 3ds/src/map_restore_table.c. Keep Azahar open (no -NoKeep). Status line in 3ds/logs/perf/status_PM5.txt before each start. Budget: reset and continue. Two tries per named cause, then the next code point. Delete this file in the PM5-4 commit.
