# agent_menu: Select menu epic (plan_menu.md sections 2 and 3)

STATE (MENU4, branch nds1, main folder; see section 11)
- MENU4 row 1 MN6 rest: PASS (21ba60d65). Row 2 MN7 rest: PASS (bc6b263a8). Row 3 MN3 check: PASS by eye (menu_shot, blank row under the tab line).
- Rows 4 (MENU-D cheats CH0 to CH8) and 5 (MENU-E) not started in this run: handback after row 3 as asked.

STATE (older, MENU2 to MENU3)
- MENU2 row 1 WARPY2: gate FAILED (Mario not in iwa_10 shot). Placement edit discarded. Debug floorscan + column log committed. Section 5.
- Epic A: MN2 open (grey top screen). Epic B: MN3b, MN4, MN5, MN6 (+ warp restore), MN7 open.
- Row 2 exit_armed (shotmenu, arm log line): PASS by eye (section 6). Epic B next.
- MENU-C (resolution) not started (out of scope, note in row_MENU).

## 1. Audit (HEAD of nds1 at start, commit 0fe0497e4 row file)

Evidence: grep of 3ds/src, 3ds/include, 3ds/tools, 3ds/CMakeLists.txt; git log --grep for row ids (no commit names MN/CH/BT/RES ids except row file).

| Row | Status | Evidence |
|---|---|---|
| MN0 | not in | no `shotmenu`, no bot_m writer; pm_main.c:553 bottom writer is inline (shotbottom=) not a function. menu_put_line pads `%-*.*s` by bytes (pm_gpu.c:1500-1508), no visible-width pad. |
| MN1 | not in | no `menu: game paused`, no freeze in pm_main.c loop (grep pm_menu_open in pm_main.c: none). Game runs while menu open; input cut only (pm_input.c:331). |
| MN2 | not in | no pm_gpu_menu_grey. |
| MN3 | partial | exit_armed + pm_optui_alert red value exists (pm_options.c, pm_options_ui.c:37). Value text "A again" (plan wants "Press A again"). No blank line after tab line (draw() pm_options_ui.c:37 writes "\n" then rows; VIS 21). Return to Title has no confirm (pm_options.c act RW_TITLE returns 4 at once). Scenario exit_armed exists (azt_scenarios.json:1659). |
| MN3b | not in | no "Close game" row. Exit row is "Exit to Homebrew Menu" (pm_options.c:333). |
| MN4 | partial | one-level submenu exists: PM_OPT_SUB tab with home_tab (pm_options_ui.c:17, :98-100). No page stack, no breadcrumb, no folder rows, Boss is a tab (tab 3) not under Debug. |
| MN5 | not in | src/filemenu/filemenu_options.c has 3 tabs, own code, no sub page. |
| MN6 | partial | PT01c Warp area submenu (commit 3fb4405be WARPMENU): presets then prefix groups, `< >` walks members (pm_options.c RW_SUB, warp_pos[]). Missing: warning page, B confirm, map rows with entries, entry move, area names table, restore of last selection (coordinator add). |
| MN7 | not in | no poslog / L+R+Y+B combo in 3ds/src (grep). poshud (Position HUD) exists, different. |
| MN8 | not in | pm_log.c has no `\x1b[1;31m` / colour codes for console lines. |
| MN9 | partial | CMakeLists.txt:499-501 NAME "Paper Mario 64 3DS Port", DESCRIPTION "Build ${PM_BUILD_NUM}", AUTHOR "Dadrewster569" (CONCOL). Plan default: description = number alone, title per plan. Open: title/description wording (plan says "Paper Mario 3DS Port"? plan 2.12 NAME "Paper Mario 64 3DS Port" kept). |
| RES1 | not in | no pm_view_m / pm_view_sx. Out of scope (MENU-C not now). |
| RES2 | not in | no `res=` key, no pm_view_* (out of scope). |
| RES3 | not in | no Video folder (out of scope). |
| CH0 | not in | no gen_item_names.py, no item_name_table.h. |
| CH1 | partial | stat rows in Debug tab (RW_STAT, Max HP, Max FP, BP, Level, Coins, Star points: pm_options.c, pm_dbg_game.c). No Cheats tab, no Set Stats folder. |
| CH2 | partial | "All badges" bulk give row (RW_GIVE+1). No per-badge folder, no unequip. |
| CH3 | partial | "All key items" bulk give row (RW_GIVE+2). No per-key-item rows. |
| CH4 | partial | "Enable partners" bulk give row (RW_GIVE+0). No per-partner rows. |
| CH5 | not in | no spawn item code. |
| CH6 | not in | no pm_cheat_tick, no moon jump. |
| CH7 | not in | no infinite HP/FP, no speed x2, no no-battles. |
| CH8 | not in | none of boots, hammer, star spirits, star pieces, story, full inventory, save/load pos, instant win, frame advance. |
| BT1 | not in | no pm_battle_table.h, no `battletable` mode in sweep_gen.py. |
| BT2 | not in | no Battles folder. |
| BT3 | not in | no "Battle KMR" HUD line, no `maphud: battle`. |

Summary: in = 0. partial = MN3, MN4, MN6, MN9, CH1, CH2, CH3, CH4. not in = the rest in epics A, B, D, E. MENU-C out of scope by row file.

## 2. Plan detail changes (plan default taken, noted here)
- MN1 present: a frozen frame with no draw (target kept, plan 2.2 facts) presents a menu change. Plan default kept.
- MN0 pad: plan said pad by visible width; done in menu_fit (pm_gpu.c), cut at 38 columns.

## 5. MENU2 row 1 WARPY2 (iwa_10), result: gate FAILED, placement code discarded

Starts used (MENU2): 2 of 8. Lines: 3ds/logs/perf/status_MENU2.txt.

Tried (binary 2252, not kept): walkable floor only (normal y above 0.5), cast again below a rejected hit, wait for curFloor (120 frames), place 1 unit below the floor (POS_SINK). Result: floor y 0 -> 0 as before, placed at -662 0 587, probe at placed+60 still `ground y 0 id -1`, shot at frame 265 (placed frame about 205, plus 60) shows rock, sky and a Mario text box ("Ah... Now... Confound it! Where could it be...?"), Mario not in the picture. Same picture as GATES1 gate 4. Two unchanged shots, so the placement edit is discarded (3ds/src/pm_warp.c back to HEAD placement code).

Kept (debug only, debug switch `floorscan`, scene frame 5 of a pos= run): the collider hits of the column under the pos= x z, and the floor grid below.

Column x -662 z 587 (run 2): one hit only. collider 28, y 0.000, normal y 1.000 (walkable). No hit below y 0.

Floorscan, map iwa_10 (run 2, same grid rule as the code: step 50, samples 700, standable 379):
- bounds x -1466..-100, z -587..613, y 0..365.
- Edge = largest z (z 613). Plan default. The side the camera looks from was NOT checked.
- The 10 standable points nearest that edge (all y 0.000):
  1. x -966 y 0 z 613
  2. x -916 y 0 z 613
  3. x -866 y 0 z 613
  4. x -816 y 0 z 613
  5. x -766 y 0 z 613
  6. x -716 y 0 z 613
  7. x -666 y 0 z 613
  8. x -616 y 0 z 613
  9. x -566 y 0 z 613
  10. x -516 y 0 z 613
- The spot -662 0 587 is 26 units from the edge row, at the edge column x -666.

Answer to the coordinator check (1): the column has no floor below y 0, so the spot is not over a lower floor. The failure is not the floor height. Next diagnosis (not run): the value of the Mario draw at this spot (model visible flag, cutscene or script flag set by the iwa_10 entry text, camera target). Not named yet.

Row 1: FAILED (gate 4). Commit: debug tool only (floorscan, column log), placement unchanged.

## 6. MENU2 row 2 exit_armed (menu shot), result: PASS

Starts used (MENU2): 3 of 8. Status: 3ds/logs/perf/status_MENU2.txt.
Change: exit_armed uses shotmenu (no frame window). Until is the new arm log line `menu: exit armed, press A again` (pm_options.c, one pm_log when row 3 is armed). The old armed-value line is printed with %s, so the static check of azt.ps1 cannot read it.
Shot bot_m0004 (after the arm, 3ds/... build3ds/azt/shots/exit_armed/bot_m0004.ppm): tab line, one blank row, rows 60 FPS, Widescreen, Return to Title Screen, selected row Exit to Homebrew Menu (label blue), value Press A again (red), Back. Read by eye.

## 7. MENU3 row 1 ENTRY TEST (iwa_10, pos -662:0:587), result: PASS (table complete, no entry holds the player)

Starts used (MENU3): 5 of 8 (row 2 has 3 left). Status lines: 3ds/logs/perf/status_MENU3.txt. Scenarios: iwa10_entry0_state, iwa10_entry1_state (azt_scenarios.json, run with -Try, status unverified).
Entries (src/world/area_iwa/iwa_10/settings.c): ENTRY_0 (-1250 30 0, yaw 90) runs EVS_ArriveFromToadTown (train arrival, DisablePlayerInput, train moves). ENTRY_1 (-445 215 -500, yaw 270) runs EnterWalk (normal walk-in from iwa_00).
Per-entry log at pos run frames 0 to 300 (state line, pm_warp.c pos_probe), pos placed at frame 30 (readback 40):

| Entry | Script | Mario alpha (drawn) | input lock / overrides | text box (pm_msg_active) | camera target | curFloor after place | camera eye (lookAt_eye) |
|---|---|---|---|---|---|---|---|
| 0 | EVS_ArriveFromToadTown | 255 | 0 / 00000000 at f30 to f300 | 0 at all probes | = Mario | 28 (f60 on) | not logged in starts 1 to 3 (field added later) |
| 1 | EnterWalk | 255 | 0 / 00000000 at f30 to f300 | 0 at all probes | = Mario | 28 (f60 on) | y -999.8 z 677 (f60), y -891 z 935 (f120 on, frozen) |

Findings:
- No entry script held input or opened a text box in the 300 frames at this spot. The entry wait (row 2) passes at once here.
- Mario is drawn (alpha 255) and stands on collider 28 (y 0, walkable). Both shots (frame 90 after the pos run starts, entry 0 and entry 1) show rock, sky and HUD only: no Mario.
- Cause of the empty picture: the camera eye sits under the map (y -891 .. -1000, Mario at y 0). Camera mode is CAM_UPDATE_FROM_ZONE with no lead (SetUP_CAMERA_ALT_NO_LEAD, include/script_api/macros.h:810). The eye stops moving at f120 on entry 1. The target follows Mario, the eye does not. Not an entry problem.
- MENU2 text box ("Confound it! Where could it be...?", shot at frame 265) did not repeat in MENU3 runs (no msg in any probe). Unexplained; not reproduced.
- First normal walk-in entry: ENTRY_1 (EnterWalk). ENTRY_0 is the train arrival scene.

Next (row 2): the entry wait is cheap and harmless here; it does not move the camera. The row 2 gate (Mario visible in the top shot) needs a camera fix, a separate diagnosis (camera zone at the placed spot).

## 8. MENU3 row 2 WARP WAIT (iwa_10 entry 1), result: FAILED (gate 1), wait edit discarded

Starts used (MENU3): 6 of 8. Status: 3ds/logs/perf/status_MENU3.txt.
Design (plan default, the row file's definition, taken without a question): entry done = no text box (pm_msg_active) and player input on (inputDisabledCount 0, PS_FLAG_INPUT_DISABLED clear, GLOBAL_OVERRIDES_40 clear). Wait max 300 frames after the pos run starts, then place with the WARPY floor snap. Log `poswarp: entry done after <n> frames`, or `poswarp: entry not done after <n> frames, placed anyway`.
Run: iwa10_entry1_state, pos -662:0:587, shot at pos run frame 90 (placed at 30, so 60 frames after placed).
- Log: `poswarp: entry done after 30 frames` (the condition was true at once: no script lock, no text box). Placed at -662 0 587, floor 28 (walkable).
- Picture at frame 90 (after 2 shots in this run, 0009 and 0014): rock, sky, HUD. No Mario. Same picture as the row 1 start without the wait.
- Gate needs Mario on visible ground in the top shot: FAILED. The wait changes no pixel (the entry is already done at frame 30). Edit discarded (pm_warp.c back to HEAD ed3ebdf82). Row 1 probe and posshot stay.
- Cause (row 1): camera eye sits under the map (y -891 .. -1000, Mario y 0). Camera mode CAM_UPDATE_FROM_ZONE, no lead (macros.h:810). The eye stops at f120 (entry 1). The camera target is correct. Next diagnosis, not started: why the zone camera of iwa_10 puts the eye under the floor at the placed spot (camera zone data for this x z, or the camera update being skipped after the warp). The next run that tests it must read the zone camera record and the eye position in the same line.
- Entry for the test spot (row 2 asked for it): ENTRY_1 (walk-in). Use it for the spot. The entry wait is not needed for the spot to work (nothing held the player).
- Note: shot log line `shot N frame F` is not printed in these runs (dump_shot path), so the picture is read from the PPM files of build3ds/azt/shots/<scenario>/.

Hand back after row 2 (the row file asks for it). Row 3 (MN2 and epic B) not started.

STATE (MENU3, branch nds1, main folder)
- Row 1 ENTRY TEST: PASS, commit ed3ebdf82. Table in section 7. First walk-in entry: 1.
- Row 2 WARP WAIT: FAILED gate (Mario not in top shot). Wait edit discarded. Camera under the map at the placed spot is the cause (section 8).
- Row 3 (MN2, epic B): not started. Next: camera diagnosis of iwa_10 (needs user OK: a new line in the next run).
- Starts MENU3: 6 of 8. Budget reset only with `run_azahar.ps1 -ResetBudget MENU3` (not used).

## 9. MENU3 camera fix (coordinator order, 2 starts max) and row 3 status

Camera fix: PASS by eye. Commit 8f65f970f. pm_warp.c: gCameras[CAM_DEFAULT].needsReinit = true after pm_pos_set (cam_mode_zone_interp.c re-init block); log `poswarp: camera reinit eye x y z mario x y z` one frame later. Gate run (iwa10_entry1_state, pos -662:0:587, shot at pos frame 90): eye 116.1 above Mario y 0.0 from f60 on (before: eye y -891 to -1000). Shot 0010 shows Mario standing on the ground. Starts MENU3: 7 of 8.
Spot default: spot_iwa10_pic and spot_iwa10_ndmcheck warp iwa_10:1 (commit 7ffba7623, index-only commit, DRAW2 hunks not in it).

Row 3 (MN2, then epic B): NOT DONE in this pass. Reasons:
- MN2 (grey top screen): the present path (pm_gpu_menu_present, pm_gpu.c:1588) runs C3D_FrameEnd, which puts the target on the top screen again on every menu text change. A grey pass made once at pause is lost at the next present. The grey pass must run after each present, after the GPU is idle, on the top back buffer (gfxGetFramebuffer(GFX_TOP)), then flush and swap. citro3d buffer order at C3D_FrameEnd is not verified in this repo. Needs a probe run, not done.
- MN3b, MN4, MN5, MN6, MN7: not started (each is a separate menu feature with its own gate).
- Starts: 1 left of 8 on MENU3. Reset needed for more starts (run_azahar.ps1 -ResetBudget MENU3, at cap only).

STATE (MENU3, branch nds1)
- Row 1 PASS (ed3ebdf82). Row 2 FAILED then superseded by camera fix (b56459102, 8f65f970f).
- Camera fix PASS by eye (8f65f970f). Spot default iwa_10:1 (7ffba7623).
- Row 3 MN2 and epic B: open. Next: MN2 grey probe (after-present grey, citro3d buffer order), then MN3b, MN4, MN5, MN6, MN7.

## 10. MENU3 epic B (rows MN2, MN3b, MN4, MN5, MN6 part, MN7), results

Design answers taken as plan default (no question asked): Q1 Close game = chainload to the HOME Menu (region title id 0004001000021000 + 0x100 per region, USA for other regions); Q3 breadcrumb on the tab line (blank line kept); Q4 file select mirrors the folder pages (no Boss, no cheats); Q2, Q5, Q6 not built (rows of MENU-C and MENU-D).

| Row | Result | Commit | Gate | Starts |
|---|---|---|---|---|
| MN2 grey top | PASS by eye | f0d537096 | menu_grey: top_grey0000 every pixel r = g = b (checked 400 x 240 with PIL, 0 non-grey samples), Toad Town visible | 1 (probe and gate in one start) |
| MN3b Close game | PASS (shot and log) | 7f41d6e1c | menu_close: armed row blue, Press A again red (bot_m0004); log: close path, chainload 0004001000021000 region 0, process end | 1 |
| MN4 folders | PASS (shot and log) | d6c750d4b | menu_folder: push log (page push 4 depth 1), shot tab 4 row 0 shows `Debug > Boss` and the boss rows, B pop | 2 valid (1 with no padscript file, 1 with) |
| MN5 file select mirror | PASS (shot and log) | 24e488671 | fs_folder: panel open (options: tab 2), page push 4, top shot shows `Debug > Warp area` and the warp rows | 1 (route found by this run: SELECT on file select, pad works there) |
| MN6 warp submenu, part | PASS (shot and log) for the warning and memory; NOT DONE for the map rows | 0da22de10 | menu_warp: warning, pop, restore log `menu: warp sub restore area -1 row 3`, shot bot_m0030 cursor on row 3 | 2 (1 with restore skipped presets, fixed) |
| MN7 position log | PASS (log) | 85604c04c | poslog_yard: `pos: kmr_20 area 0 map 11 entry 4 x 594.000 y 30.000 z -216.000 yaw 180.0` once per press | 1 |

Named gaps (not done, not dropped):
- MN6: map rows per area, entry shown and moved (left/right), A warps to the chosen entry, area name table. Current warp folder: presets then prefix rows with member walk (left/right), A warps to entry 0.
- MN7: the Debug row "Position log combo" (switch poslog only). Also the L+R+Y+B line is pm_log_w (warn level).
- MN2: grey pass is run at the pause and after each menu present (gspWaitForPPF, then gfxGetFramebuffer(GFX_TOP) back buffer, luma, GSPGPU_FlushDataCache, gfxScreenSwapBuffers). The order was confirmed by the shot (grey shown), not by a timing probe.
- MN3 blank line under the tab line: not seen in the menu_close shot (bot_m0004 has no blank row). Not re-checked here.

Shared-tree note: another agent's working copy had invalid JSON escapes (`\d` in entcheck_mac01 and battle_start_sprcheck) in 3ds/tools/azt_scenarios.json and bench_defs.json. azt.ps1 and bench.py refuse to run until fixed. Not touched by MENU3.

STATE (MENU3, branch nds1, after epic B)
- Row 1 ENTRY TEST: PASS (ed3ebdf82). Row 2 WAIT: gate failed, edit discarded (b56459102). Camera fix: PASS (8f65f970f). Spot: iwa_10:1 at -662:0:587, then moved to -586:1:530 (5159bece7).
- MN2, MN3b, MN4, MN5, MN7: PASS. MN6: part (warning, memory), map rows and entries open.
- Epic B hand back: after MN7 (this section).

## 11. MENU4 (rows 1 to 3 of row_MENU4.md)

Row 1 MN6 rest (warp area folder, one row per map with the entry): PASS (log and shot). Commit: see git log (MN6 rest).
- Folder level 1: presets (8 rows as before), then one row per area (`Goomba Village` .. `Test map`, area name table in pm_warp.c, areas with a non-test map only). A on an area opens the area page (log `menu: page push 4 depth 2`).
- Area page: one row per map (`machi`, `mac_00`, ...), value `< entry of count >`, left and right move the entry (wrap, RAM only), A warps to that map and entry (pm_warp_live). Breadcrumb `Debug > Warp area > Toad Town`. B goes back to the area list on the area row.
- Warp memory in RAM: last area row (warp_last_row, restored on the warning page), last map row of the last area (warp_mem_area, warp_mem_row, restored when that area page opens), entry per map (pm_warp_entry_get/set).
- Gate run menu_warp_entry: area page row, `< 1 of 8 >` on mac_00 (shot bot_m0036 read by eye), `warp: live to choice 27 entry 1`, `warpstat after live: slot 0 map 1:1 entry 1`. The picture after the warp is not read (gate is the warp log line).
- Regression menu_warp (MENU3 MN6 part): PASS (log, restore row 3).
- Entry count from MapSettings.entryCount (static data). Maps with no settings warp to entry 0 (plan default).
- Named gap: the page scrolls only within VIS rows (long area lists are cut by the menu height, no paging beyond the window). Entries per map are not shown as a window `0 1 [2] 3 4` (plan 2.6 shape); the value is `< n of N >`.
- Starts MENU4: 5 (2 lost to a shared Azahar log and the padscript file not staged in the sdmc PaperMario folder, 3 valid). Starts for the menu_warp_entry gate: 2 valid (start 3 and the CheckLog pass), menu_warp regression 1.

Row 2 MN7 rest (Debug row "Position log combo"): PASS (log and shot). Commit: see git log.
- Option poslog (pm_opt.poslog, options.ini poslog=0|1, option row 12). Debug row 22 (mode 1 and file select), after the Warp area and Boss folder rows so Profiling, Warp area and Boss keep their row numbers (no pad or scenario changed). Plan default "next to Position HUD" not taken: that would move rows 10 to 22 and five pads. Named choice, not a question.
- The combo (pm_input.c) fires on pm_opt.poslog or the switch poslog. The line is unchanged (pm_poslog_line, warn level).
- Gate menu_poslog_row (yard, no switch): `menu: tab 2 row 22 Position log combo = On` (shot bot_m0025 read: row On), `pos: kmr_20 area 0 map 11 entry 4 x 594.000 y 30.000 z -216.000 yaw 180.0`, Off again (last match), saved poslog=0.
- Starts MENU4 after row 2: 6 (of which 2 lost, see row 1).

Row 3 MN3 check (blank row under the tab line): PASS by eye. Scenario menu_shot (SELECT, General tab, first draw, bot_m0000, status now verified): tab line at row 0, one blank row (row 1), `60 FPS` at row 2. The old menu_close shot (bot_m0004, MENU3 build) had no blank row; the current build has it.
- Starts MENU4 total: 6 (2 lost to a missing padscript in the sdmc PaperMario folder, the first two runs; the copy step is now in the row work: copy 3ds/tools/pad_*.txt to the sdmc PaperMario folder before a start).
- Row 4 (CH0 to CH8) and row 5 (MN8, MN9, BT1, BT2, BT3): not started. Named gaps: none opened yet.
