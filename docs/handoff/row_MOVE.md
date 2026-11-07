# Row MOVE: the move train (all remaining pm_gbi.c moves, one owner)

User decision (2026-11-07): one agent does every remaining pm_gbi.c move in sequence, so no other branch cuts pm_gbi.c at the same time. Plan: docs/handoff/plan_s10_path.md (What is left, Design, Rows WK-2, WK-3, GM-2, GM-4, LS-1, LS-2; Decisions D1 to D13). D12: pure cut and paste plus prototypes, no logic change.

## Order (one commit per move)
1. WK-2 census (group K) to nd_cen.c, by hand in sub-groups (os_*, pk_*, mdc_*), each with a hand-written list of its globals (no regex mover: it broke the build before).
2. WK-3 target (set_cimg/gbi_setcimg, img_role, grab_cur, off_screen_check, depth_of_word, word_5551, pm_depth_query, prim_ndc_z) to nd_target.c.
3. GM-2 G_VTX case, draw_tri, gv_tri, gv_*, tri/cull/clip helpers to nd_geom.c (the CPU-only functions stay where they are: GM-3 deletes them after TRI-4).
4. GM-4 sun: grep pm_sun_* callers, move to nd_geom.c or delete.
5. LS-1 map shapes (dlc_*, gp1_box, nd_env, pm_dlc_native_ranges) to nd_shape.c.
6. LS-2 entities (ent_*, the G_PM_ENT case) to nd_ent.c.
Not in this row: LS-3 (records; waits for SPR4 and MDL-4), GM-3/ST-3 deletes, WK-4 part 2 (needs an oracle decision).

## Gate
After every 2 or 3 moves (not per move): spot_mac01_pic byte-equal to nds1, gbi phase at mac_01 within +3 percent (opsplit), plus the move's named scene by eye (iwa_10, battle_start, pause_spin_grab_w300 for WK-3, sbk56 sun for GM-4, entcheck_mac01 diff 0 for LS-2). Grep proof that each moved definition left pm_gbi.c. Host tests link (nd_host_test.sh) after each gate.

## Rules
Own worktree build3ds/wt/move, branch move from nds1. Rebase on nds1 after each gate and hand the head to the orchestrator only at the end, unless a merge is asked for. Check `git grep '^<<<<<<<'` per commit. No attribution lines, never commit 3ds/src/map_restore_table.c, keep Azahar open. Two tries per named cause. Hand back once after move 6 (or a failed gate). Delete this file in the last commit.
