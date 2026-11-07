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

## Oracle decision WK-4b
Rule (D9): a switch survives only if an open row names it, or it is a user option. A deleted switch keeps its default branch (the switch-off path) and loses the other one.
- nofog: keep. It is a user option (options.ini, the "options: ... nofog=0 nolight=0" line), like nolight, so it is not a debug switch.
- rmoff: delete, and keep the render-mode path. Delete its XOR term in the state hash too; with the switch off the term contributes 0, so the hash value is unchanged. Check that the term really is 0 when off before deleting.
- nocull: delete. cull_off() becomes 0, so its 5 sites collapse to the cull path.
- nooff: delete; the off-screen path stays.
- t1foldall: delete; the default t1_fold rule stays.
- dlccheck: delete, together with the dlc_verify_skip path (verification only); the normal skip stays.
- behroute, nobox, noboxint, skipdump: delete; the default branches of pm_dlc_native_ranges and dlc_static_group stay.
- gv1check: delete its three blocks; the GV path and its cull checks as they run with the switch off stay.
- dd_on, xt_on, rmlog_on: delete, with the drawlog and rmlog code they guard (dd_* functions, the verts[] copy in G_VTX, tri_dd_log, tri_rm_log) and their call sites in nd_walk.c and the GV path. The ndref compare is the debug tool that stays.
- gbiprof / gp_*: convert, do not delete. The counters move to nd_cen.c (cold). They are fed only when pm_ph_on (prof) is set, the census K report reads them there, and the separate gbiprof switch is gone. The hot path keeps a single `if (pm_ph_on)` test, which is no extra test where one already exists.
- Kept because rows name them: combchk, entcheck, r13off, waterlog, orbdbg, texdiff.
Commits, in the MOVE train after its move 6; each gated by spot_mac01_pic byte-equal, plus the extra noted:
1. Render, cull, off: rmoff, nocull, nooff, t1foldall. Also battle_start byte-equal.
2. DLC: dlccheck, behroute, nobox, noboxint, skipdump. Also iwa_10 byte-equal, and the dlc report lines equal.
3. GV and drawlog: gv1check, dd_on, xt_on, rmlog_on. Also iwa_10 by eye.
4. gp_* into the cold census. Also one prof=4 run shows the same census and opsplit lines.
Proof per commit: `grep -n 'PMDL("<word>")\|pm_debug_has("<word>")'` is empty for each deleted word.

These 4 commits run after move 6, in this row.

## Gate
After every 2 or 3 moves (not per move): spot_mac01_pic byte-equal to nds1, gbi phase at mac_01 within +3 percent (opsplit), plus the move's named scene by eye (iwa_10, battle_start, pause_spin_grab_w300 for WK-3, sbk56 sun for GM-4, entcheck_mac01 diff 0 for LS-2). Grep proof that each moved definition left pm_gbi.c. Host tests link (nd_host_test.sh) after each gate.

## Rules
Own worktree build3ds/wt/move, branch move from nds1. Rebase on nds1 after each gate and hand the head to the orchestrator only at the end, unless a merge is asked for. Check `git grep '^<<<<<<<'` per commit. No attribution lines, never commit 3ds/src/map_restore_table.c, keep Azahar open. Two tries per named cause. Hand back once after move 6 (or a failed gate). Delete this file in the last commit.

## State (2026-11-07, handback of the first MOVE agent)
Branch move head cc035ecd0 (base 52374b549, not rebased), worktree build3ds/wt/move. Done: moves 1-6 (a0af182d3 WK-2, cb41ff6af WK-3+GM-2 in one commit, ffbe556ac GM-4 sun live, c552bafbc LS-1, 35c544069 LS-2), WK-4b-1 898efe249, WK-4b-2 b84f53e7f. Gates: spot_mac01 md5 equal through every step; entcheck_mac01 diff 0; iwa_10 and sbk_56 by eye.
Open, in this order:
1. Rebase move on nds1. Conflicts: pm_gbi.c (os_class_of drops G_PM_TEXI, removed G_PM_TEXI comment in the census block) and nd_walk.c (case G_PM_TEXI deleted in nds1). Rule: drop G_PM_TEXI everywhere, keep the move cuts. Check markers in every commit. Rerun spot_mac01_pic bytes once (compare with the md5 of the nds1 build at the same time, cold).
2. WK-4b-3 (gv1check, dd_on, xt_on, rmlog_on and the drawlog code; sites: nd_geom.c 172/211/221/247/324/329/336, nd_shape.c 50, nd_walk.c 100/101/143/185, pm_gbi.c 82/133-198/648/690/709/820/826/828/1050; recheck by grep after rebase). Gate mac01 bytes, iwa_10 picture.
3. WK-4b-4 (gp_* into the cold census, gbiprof switch removed, one pm_ph_on test). Gate prof=4 census line, mac01 bytes.
4. WK-2 module column in os_report (per-module op counts).
5. Fix 3ds/tools/nd_host_test.sh include path (nd_int.h includes ultra64.h) so nd_host passes.
6. gbi-phase within +3%: one phase run of mac_01 against the nds1 build.
Note: all pic runs logged rc "crash symbols" (also base). Check whether this is the sprite zeroing bug (row_SPR4) after sprfix lands, else report the PC.
