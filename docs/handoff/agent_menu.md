# agent_menu: Select menu epic (plan_menu.md sections 2 and 3)

STATE (branch menu, base nds1; rows of MENU2 on branch nds1 main folder)
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
