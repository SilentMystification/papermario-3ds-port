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

## State 4 (2026-11-07, MOVE4 agent)
Branch move4 from nds1 a682f24be, worktree build3ds/wt/move4. MOVE-4 done: suntile_on, suntile_t0, suntile_tri, pm_sun_spawn, pm_sun_draw, pm_sun_base (and their calls in src/effects/sun.c, nd_geom.c, nd_comb.c, nd_tex.c), cc07_dbg, tritrace_on, tritrace_tri, tritrace_seq and the 5 tritrace lines in draw_tri_cpu deleted. Scenarios nok02_tri_dd and nok02_tri (tritrace only) deleted. sun_sbk: suntile word removed. New scenario and bench spot_sbk56_det (cold det, sbk_56 sun scene).
Gates: spot_mac01_det cold det 12 of 12 shots byte-equal to nds1 a682f24be (runs 15571 base, 15708 head). spot_sbk56_det cold det 12 of 12 byte-equal (one run each). spot_sbk56_pic (no det) differs in 2 of 3 shots between two nds1 runs, so it is noise and not a gate. Link map (pm_3ds.map, pm_gbi.o sections) has no suntile, tritrace, cc07 or pm_sun names.
Left in pm_gbi.c (live link sections of pm_gbi.o): rec_op, rec_fogpos, rec_fog, rec_dl, rec_aux, rec_matrix, rec_apply, rec_run: LS-3 (MDL-5 edits rec_run now). sync_legacy, cc_legacy, set_leg: ST-3 (after RECT-3). draw_tri_cpu, gbi_vtx_cpu1, fold_axis, pack_color, lit_color: GM-3 (after TRI-4). box_pattern_rect, rect_to_image: RECT-3. pm_gbi_bind_tile1: named by row_TX and row_MDL5, keep until they decide.
The row file stays until LS-3, ST-3, GM-3 and RECT-3 land.

## State 5 (2026-11-07, MOVE5 agent)
Branch move5 from nds1 9d978d836, worktree build3ds/wt/move5. Done, pure cuts and prototypes:
- pm_gbi_combined_ptr to nd_mtx_set.c (next to combined, its only data). Caller: pm_gpu.c native draw.
- dual_boost_on, combchk_run with cc_runs, cc_diff, cc_last_b to nd_comb.c after combchk_on. combchk kept (S3N rows name it). dual_boost_on is live (pm_gpu.c rgb_scale).
- lcg_s, lcg_f to nd_comb.c. Live, not debug: the 24-input plan decision (sync work, seed 777) calls lcg_f. Not deleted (plan_s10 table said delete; corrected there).
- nd_tex.c split for the host link: descriptor table, nd_tex_get, unit words stay in nd_tex.c (host-tested). Walker ops, ensure_tile, tex_op and their globals to nd_tex_set.c (target only, CMake). Same pattern as nd_mtx_set.c.
- nd_tex -Werror fixes (op_loadblock and op_loadtlut (void)w0, tlut_v20 sign compare); now in nd_tex_set.c.
- nd_host: ND HOST TESTS PASS. ver/us/build/include and two mac shape bins copied into the worktree (untracked, build-at copies them too).
Gates: spot_mac01_det cold det, nds1 9d978d836 run 6911 vs head 4fd23d765 run 7139: 12 of 12 shots byte-equal (md5). Log checks pass; the picture is the same bytes as the base, which shows mac_01 at pos 0 0 200.
Link map (head build, pm_3ds.map): pm_gbi_combined_ptr in nd_mtx_set.o; dual_boost_on, lcg_f, lcg_s in nd_comb.o; combchk_run in nd_comb.o (unlikely section); op_loadblock, tlut_v20 in nd_tex_set.o; nd_tex_get in nd_tex.o; nd_model_draw in nd_model_set.o. No moved name in pm_gbi.o.
Starts used this row: 2 of 8.
The row file stays until LS-3, ST-3, GM-3 and RECT-3 land.

## Oracle plan MOVE 7+ (empty pm_gbi.c)
Precondition grep fix: count definitions only, `grep -cE "^[a-z][^;]*\)\s*\{?$" 3ds/src/pm_gbi.c`, plus nm on pm_gbi.o (defined T/t symbols). S10 precondition 4: nm shows only the pm_gbi_run shim, or nothing.
Targets: walk core (is_host, ptr_of, pm_gbi_ptr_of, pm_gbi_run, pm_gbi_init, gbi_cold_modifyvtx, pm_gbi_fallback, lt_mem_same) -> nd_walk.c (MV-7). Target/frame (pm_depth_query, pm_is_cfb, pm_cfb_capture_prev/_cpu/_cpu_for, pm_zbuf_need, pm_fb_native, rgba_from_5551, the definitions here only) -> nd_target.c (MV-8). Census (pm_gbi_flush_class, ph_pick, gv_streak_*, tex_streak_*, gp3_t0_*) -> nd_cen.c (MV-9). The debug deletes (tritrace, cc07_dbg, suntile) are MOVE-4. Combine (sync_combine, combchk_on moves, combchk is named by a row) -> nd_comb.c; shift_mul, pm_tex_set_filter -> nd_tex.c; pm_gbi_gv_at -> nd_geom.c; nd_light_count -> nd_mtx (MV-10).
Later rows (not this agent): legacy combine delete ST-3 after RECT-3; CPU vertex delete GM-3 after TRI-4; records to nd_recs.c LS-3 after MDL-4; sprite to nd_sprite.c LS-3 after SPR4 row 4.
Each MV row: pure cut and paste plus prototypes, about 150 lines. Gate: mac_01 byte-equal (det switch when DET is in nds1, else a warm A/B pair with the same harness), nm shows the row's symbols absent from pm_gbi.o, host tests link.
(a) Module column: static const uint8_t op_module[256] in nd_cen.c, updated in each move row; the gate checks the table against the switch arms. os_report adds the module column from the table. Bucket MOD_PM_GBI must reach 0 (S10 precondition 1). No per-arm store in the hot path.
(b) nd_mtx host link: split, no stubs. nd_mtx.c keeps pure math (nd_mtx_pcomp, pool, products). The GM-1 handlers that call walker or GPU symbols (G_MTX, popmtx, movemem lights, lit_freeze, lt/lk, g2_same/g2_note, gbi_moveword, pm_gpu_note_mvp, pm_gpu_set_ntab, pm_eye_e, ptr_of, pm_frame_count, pm_log, pm_gbi_ndref_frame) go to a new target-only nd_mtx_set.c. The nd_mtx host test links nd_mtx.c only. Add nd_mtx_set.c to the D5 module list.
(c) Census gate: wcen_mac01 (prof=4, det), scene frames 0-700 (two 300-frame windows). Pass: prod lines with the module column print for windows 1 and 2; per-producer op totals within 2% of the pre-WK-4b base.
(d) Phase: prof=4 with opsplit, cold, det, wcen_mac01 and wcen_iwa10; mean gbi-phase us per frame over scene frames 100-400, base before the train vs head after each MV row. Gate: at most +3% per row (Azahar direction only). Past +3%: put the hot handler pair back in one TU or static inline in nd_int.h, measure again.
Order: (b), MV-7, MV-8, MV-9, MV-10, (a), (c), (d). Delete this file in the last commit.

## MOVE-5 state (2026-11-07, MOVE5 agent)
Branch move5 head is the merge of nds1 1e915fa5b (MDL-5e) into 5c783aa14. Conflict in nd_model.c resolved: nds1 noise and tex1 split and the mf_bx census kept in nd_model_state_in; nd_model_draw stays in nd_model_set.c (body identical to nds1). Host harness fixed for MDL-5e: nd_host_test.sh stubs (pm_log_prof_line, pm_gpu_noise_ok and pm_gpu_noise_draw, gGameStatusPtr); nd_model_test.c custom gfx case uses a real list (rr[22]=1 read address 1 and crashed). nd_host PASS. No conflict markers.
Gate spot_mac01_det: move5 lane (PM_LANE=move5) shots 200 to 205 equal nds1 1e915fa5b (build-at, same options): 6 of 6 byte-equal (run 24090). Earlier move5 runs gave 12 shots each because the master options.ini had fps60=1 (f60, two draws per game frame). Not a code difference.
MOVE-5 step 1: passed. Orchestrator sign-off pending.
Build rule: always PM_LANE=<lane>. A worktree build without it uses the shared main volume pm3ds-src (the first merge build did this; its shots are not used). A worktree needs the untracked ver/us/build and assets/us copied from the main repo (done in move5). Build: copy map_restore_table.c to scratch, python 3ds/tools/gen_map_restore.py, PM_LANE=move5 sh 3ds/build.sh, restore the table file.
LS-3 (step 2): NOT STARTED. Next: move group P (rec_run, rec_apply, rec_op, rec_fog, rec_fogpos, rec_dl, rec_aux, rec_matrix) from pm_gbi.c to nd_recs.c (pure cut and paste plus prototypes, CMake line, op_module update). Gate: build, mac_01 and iwa_10 cold det byte-equal to nds1, nd_host PASS, nm shows the symbols in nd_recs.o.
Next command: cd build3ds/wt/move5, then start LS-3 with PM_LANE=move5 (see Build rule). Keep options.ini fps60=0 for det gates.
Cosmetic debt: nd_comb.c has the SPR4-3 dual_boost_on comment twice; fix in the LS-3 commit.
