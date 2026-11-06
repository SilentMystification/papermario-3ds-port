# agent_menu: Select menu epic (plan_menu.md sections 2 and 3)

STATE (branch menu, base nds1)
- Branch: menu. Base: nds1 (switched from nds1 at start).
- Audit: done (section 1 below), commit pending in this session.
- Epics: MENU-A (MN0 to MN2) next. MENU-B, MENU-D, MENU-E after.
- MENU-C (RES1 to RES3) not now (Not now list in row_MENU.md).
- Added by coordinator: MN6 remembers last Warp submenu selection (RAM only, no options.ini; log `menu: warp sub restore area <a> row <r>`). Also for PT01c area submenu if it is the MN6 base.
- Next: MN0 menu shot tool and visible-width pad.

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
- (none yet)
