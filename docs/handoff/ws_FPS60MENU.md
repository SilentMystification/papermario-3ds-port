# ws_FPS60MENU (Haiku): the "60 FPS" row comes back in the Select menu and the Options panel (revert of FSKIP2)
Designer: oracle NEXT3 (SendMessage). Worktree build3ds/wt/FPS60MENU, branch FPS60MENU from main, PM_ROW=FPS60MENU PM_LANE=FPS60MENU.
Edit only by `git revert` of 1e7ddd7ea1. It touches 3ds/include/pm_fpsmode.h, 3ds/include/pm_port.h, 3ds/src/pm_main.c, 3ds/src/pm_options.c,
3ds/tests/host.d/fskip.sh, 3ds/tools/pm_fpsmode_test.c. Shares no file with NST2A-F. Model: Haiku (one git command).
## Cause (FACT, code and git)
- Producer of the rows: tab_ids (pm_options.c:418-422). One table drives the Select menu (mode 1) and the Options panel (PT59, 0bef0fc8a8).
  Labels: pm_optui_label (pm_options.c:639, :675). Draw: filemenu_options.c:138 and pm_options_ui.c:46-58.
- Commit 1e7ddd7ea1 (FSKIP2, 2026-11-16 03:47, before the session start 002498da73) changed tab 0 from
  `ids[n++] = RW_DBG + 7; ids[n++] = RW_WIDE; ids[n++] = RW_FPS;` with `case RW_FPS: return "60 FPS";`
  to `ids[n++] = RW_FPS; ids[n++] = RW_WIDE;` with `case RW_FPS: return "Frame skip";` (pm_options.c:421, :675 now).
  It also removed the fskip boolean (PmOpt.fskip, row 9, pm_fskip_set) and folded FS1 into the fps60 mode (pm_main.c:688, :719).
- No hide condition exists: tab 0 has no O3DS or mode test (pm_options.c:420-422). No merge since 002498da73 touches the 6 files (git log).
- FACT: `git apply --check -R` of 1e7ddd7ea1 succeeds on main 87bb30efff.
## Edit
1. `git revert --no-edit 1e7ddd7ea1`. Then amend the message to `FPS60MENU 1: revert FSKIP2 (1e7ddd7ea1): 60 FPS row and Frame skip row back (user 2026-11-17)`.
   Result: tab 0 rows "Frame skip" (On/Off, options.ini fskip), "Resolution Mode", "60 FPS" (Off/On/Dynamic, options.ini fps60).
## Loop (each output to build3ds/out/, read the last 20 lines)
1. `git diff 1e7ddd7ea1^ HEAD -- 3ds/include/pm_fpsmode.h 3ds/include/pm_port.h 3ds/src/pm_main.c 3ds/src/pm_options.c 3ds/tests/host.d/fskip.sh
   3ds/tools/pm_fpsmode_test.c > build3ds/out/fps60menu_diff.txt`. The file must be empty (exact restore).
2. `python 3ds/tools/bench.py base spot_mac01_drec --row FPS60MENU` (base run, not charged).
3. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row FPS60MENU --bench spot_mac01_drec --why "FPS60MENU gate" > build3ds/out/fps60menu_loop.txt 2>&1`.
## Tests that decide
- Step 1 file empty. `LOOP PASS`: host PASS (fskip.sh of the old form passes), build rc 0, spot_mac01_drec byte-equal (det mode skips FS1:
  pm_main.c:688 has `!pm_det_on()`). Budget: 1 build, 2 Azahar starts.
## Fix rights: none beyond the revert. A revert conflict, a host FAIL, a compile error or a cmp DIFF: stop, send the path to the oracle, wait.
## Console check (user, add to verify_list.md at merge)
- Open the Select menu, General tab. Rows: Frame skip, Resolution Mode, 60 FPS. Set 60 FPS to On. The log shows
  `options: live row 0 value 1 (On)`. The game draws 60 frames a second (prof line).
## Stop and hand-back
Last commit `FPS60MENU 2: gate. revert exact; spot_mac01_drec byte-equal` deletes this page. One SubagentHandback: hashes, LOOP line.
