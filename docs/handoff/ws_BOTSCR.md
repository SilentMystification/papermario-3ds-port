# ws_BOTSCR (Haiku): the bottom screen HUD, footer and header draw also while logging is off (bug fix, no counter)
Designer: oracle NEXT3 (SendMessage). Worktree build3ds/wt/BOTSCR, branch BOTSCR from main, PM_ROW=BOTSCR PM_LANE=BOTSCR.
Edit only 3ds/src/pm_log.c, 3ds/src/pm_gpu_ui.c, new 3ds/tests/host.d/botscr.sh. Shares no file with FPS60MENU (pm_port.h, pm_main.c,
pm_options.c stay) or NST2A-F. No wait. Model: Haiku (1 new function, 3 call lines, 1 grep test).
## Cause (FACT, code)
- The bottom console text is drawn only by pm_gpu_hud_flush (pm_gpu_ui.c:156). Only con_flush calls it (pm_log.c:896), on the sink thread.
- sink_main (pm_log.c:899-905): with the log queue off, `qon()` is 0 (pm_log.c:74: `log_file_on || prof_mode`), and the thread waits on
  ev_sink with NO timeout. A signal comes only at a gate change (:390) and at console ready (:536).
- So with Logging off and Profiling off, the game thread writes the HUD rows (pm_main.c:848 -> pm_gpu_hud, pm_gpu_ui.c:122 `hud_new = 1`)
  and the footer (:152 `foot_new = 1`), but nothing draws them. After a menu close, pm_gpu_ui.c:64 clears the screen and :68 sets the flags,
  and nothing draws them: the bottom screen stays blank.
- Logging off comes from options.ini `log_on=0` (pm_options.c:141-146, :230), the Logging row, or the switch `log=off` (pm_log.c:363).
  The prof profile (prof=30) keeps prof_mode 1 and the log profile keeps log on: both draw. conlevel, livelevel, disklevel only select log
  lines (pm_log.c:353-383); they do not stop the HUD.
- Other blank states (by design, no edit): before pm_log_console_ready (con_up 0, pm_log.c:874), pm_io_quiet (sleep and exit, :907),
  C3D_Init failure (pm_gpu.c:1799), HUD option off (pm_main.c:844 `if (pm_opt.dbg_hud)`; the footer still draws).
- The cause is LOG4 0c1f761665 (2026-11-06), before the session. No merge since 002498da73 touches pm_log.c, pm_gpu_ui.c (git log).
  GPOS1 (27b9c6467a) changed only PmTask.n, a log-only field (pm_port.h:122-125). DRECTEX touched pm_gpu.c drec code only.
## Edits
1. pm_log.c, after pm_log_console_ready (:536 block end), add:
   `void pm_log_sink_kick(void) { if (con_up) LightEvent_Signal(&ev_sink); } /* BOTSCR: bottom screen work (HUD, footer, menu close) wakes the sink */`
2. pm_gpu_ui.c: after the includes add `void pm_log_sink_kick(void); /* pm_log.c */`.
   :122 `    hud_new = 1;` -> `    hud_new = 1;` and the next line `    pm_log_sink_kick(); /* BOTSCR */`. The same after :152 `foot_new = 1;`.
   :68 add `pm_log_sink_kick(); /* BOTSCR */` after `frame_key = -1;` (before the comment).
   The event is RESET_ONESHOT (pm_log.c:1347): with the queue on, the 100 ms sleep path is unchanged.
3. host.d/botscr.sh (frame of host.d/vcopy.sh, no gcc): `grep -c 'pm_log_sink_kick();' 3ds/src/pm_gpu_ui.c` must print 3, and
   `grep -q 'void pm_log_sink_kick(void) { if (con_up) LightEvent_Signal(&ev_sink); }' 3ds/src/pm_log.c`. Print `botscr ok`.
   Seeded fault: a sed copy of pm_gpu_ui.c without the :122 kick must give 2: else `botscr seeded fault not seen`, exit 1.
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edit. Commit `BOTSCR 1: HUD, footer and menu close wake the log sink (bottom screen draws with logging off)`.
2. `python 3ds/tools/bench.py base spot_mac01_drec --row BOTSCR` (base run, not charged).
3. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row BOTSCR --bench spot_mac01_drec --why "BOTSCR gate" > build3ds/out/botscr_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS with `botscr ok`, build rc 0, spot_mac01_drec byte-equal (the bottom console is not in drec). Budget: 1 build, 2 starts.
## Fix rights: only the 3 files. A compile error, a host FAIL or a cmp DIFF: stop, send the path to the oracle, wait.
## Console check (user, add to verify_list.md at merge)
- Select menu > Debug > Logging page: set Logging Off. Close the menu. The bottom screen shows the HUD (fps row updates) and the footer.
## Stop and hand-back
Last commit `BOTSCR 2: gate. botscr ok; spot_mac01_drec byte-equal` deletes this page. One SubagentHandback: hashes, LOOP line.
