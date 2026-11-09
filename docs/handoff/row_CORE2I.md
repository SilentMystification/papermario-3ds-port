# Row CORE2I: 60 fps interpolation on the logic thread

## Goal
With fps60 on, lp_serial (pm_main.c:391, term `pm_interp_on > 0`) runs every step serial: N3DS core2 `ser 300 n 0`, core 2 idle.
Make the logic build step N+1 while core 0 walks lerp N and key N. Delete the serial interp path (det/phase/census/prof samples stay serial).

## Start state
Worktree build3ds/wt/core2i, branch CORE2I from main (hash in the first commit message). PM_ROW=CORE2I, PM_LANE=core2i.

## Premise and checks
Hazards (oracle): H1 rtab/repl (pm_interp.c:44-45) written by pm_interp_lerp (pm_nusys.c:151, before fr_push :153) while the walker reads
them via pm_interp_find (nd_mtx.c:42, nd_mtx_set.c:95). H2 key_list/nkey (pm_nusys.c:90-93) shared. H3 pm_interp_key_draw from the game
thread idle pass (pm_main.c:503) posts via wq_main_direct -> pm_lg_quiesce each pass. H4 2 display contexts (main_loop.c:15/324): N+2
may rebuild N's context while key N walks. The prof `fps` field does not count key frames (pm_phase.c:497): dro_01 with iv 0 300 0 0
is probably already at 60 presents. Step 1 (kf) checks this before the threading.

## Plan
1. kf field (prof only): pm_logic.c counters lg_kwalk, lg_kdue (walker), lg_kseen (game); pm_lg_pf out kf; pm_phase.c core2 line adds
   `kf %u` (window difference); bench_defs.json kf metric on spot_mac01_prof. Commit.
2. LgCmd (pm_port.h) gains `unsigned flags`, LGF_ISKIP 1, LGF_NOKEY 2. pm_main.c:573 puts iskip in flags; NOKEY when fa_state != 0
   (replaces the fa_state/last_step_skipped tests at :503). pm_step_run sets pm_interp_skip from the flag.
3. Key message: delete pm_interp_key_draw; wq_key_draw becomes pm_key_walk(const PmKey*) (PmKey {PmTask t[4];} 32 B = WQ_ARG);
   add pm_interp_key_take(PmKey*) (copy key_list, nkey = 0). pm_interp_drop_keys stays.
4. nd_wq.c: pm_wq_key (logic: wq_fr_push++, wq_put(wq_key_run); game thread: wq_fr_push++, wq_main_direct). wq_key_run: pm_key_walk,
   barrier, wq_fr_walked++, pm_lg_key_walked(). pm_wq_frame_slot(): on the logic, if (wq_fr_walked != wq_fr_push) wq_block(c_walked).
   wq_block takes a kind (2 = c_walked); pm_wq_pump_blocked for kind 2 runs only while (wq_fr_walked != wq_fr_push && pm_wq_pump_one())
   (a full pump would run begin N+1 and pull an extra vblank: iv 3, interp lost). Update the header comment.
5. pm_nusys.c:149-153 else branch: pm_wq_frame_slot(); pm_interp_lerp(0.5f); fr.flags |= PMF_REPL.
6. pm_step_run: key = drew && pm_interp_on > 0 && !(flags & LGF_NOKEY) && pm_interp_key_take(&k) (the take clears nkey always);
   wq_step_end arg {int drew, key}: key -> pm_lg_key_due(), then pm_lg_end_walked(); after the tail: if (key) pm_wq_key(&k).
7. pm_logic.c pm_lg_show_keys(): while ((int)(lg_kwalk - lg_kdue) < 0) { if (!pm_wq_pump_one()) pm_wq_wait_walker(LG_WAIT_NS); }
   return lg_kwalk - lg_kseen; lg_kseen = lg_kwalk.
8. pm_main.c:501-507 after the LG_IDLE hand: k = pm_lg_show_keys(); if (k) { drawn += k; pm_pace_drawn += k; det_keys += k; continue; }
   Keep the interp vblank wait at :523 for passes with no key.
9. Delete `|| pm_interp_on > 0` in lp_serial (:391) and the declaration at :330; fix comments pm_logic.c:6, nd_wq.c:4, pm_main.c:390;
   put the rtab/slot rule into plan_core2.md (replace the old interp sentence).
10. Commit; message states: no heap growth, static about 16 B, 32 B message, no division in hot paths, memcheck line.

## Done criteria
1. nd_host PASS.
2. gameover_fps60 byte-equal to the base main; spot_mac01_det and battle_start_f182_cold byte-equal (nok11_det, mdl5_iwa10_det,
   tour10_det if budget allows).
3. spot_mac01_fps60_pic shots 200-205: mac_01 visible, no tear, no pop, no missing actors (lerp and key shots).
4. btrans_stand_enc: caplog `interp 1`, `cpu-after-split 0`.
5. New c2haz_mac01_thr60 and c2haz_battle_thr60 (the _thr strings plus fps60): no new c2haz addresses at steady state beyond the 2
   known port-heap ones; `logic: thread on core` line present.
6. New console bench dro01_fps60_prof_n3ds (console_prof, host n3ds, warp=dro_01:0 pos=-256:0:9 prof=30 fps60, until
   `prof f\d+ w\d+ core2`), plus the same recipe on the heaviest N3DS scene (bench.py where --host n3ds). User pushes. Pass: ser about
   10, n about 290, kf about 300, iv mostly 2, o34 below about 80.
Affected scenarios (no others): the ones above. Order: gameover_fps60, fps60_pic, c2haz60 x2, btrans_stand, spot_mac01_det,
battle_start_f182_cold. At the cap: INCOMPLETE, -ResetBudget CORE2I, continue.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=core2i on every build.sh and bench call. No git stash. Commit after each passed step.

## At the end
Delete this row file in the commit that completes the row. SendMessage main "CORE2I ready <hash>".
