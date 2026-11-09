# Row RIGHTON: jump/hammer prompt shows the Japanese "Right On!" art with the perfect action cheat

## User facts (2026-11-08, console)
The jump action command shows an icon with Japanese text. (Seen with the Perfect action commands cheat on; the save has the Lucky Star.)

## Cause (oracle trace)
HES_RightOn (src/battle_ui_gfx.c:489-509) draws ui_battle_cmd_right_on (src/battle_ui_gfx.c:158-159, assets/us/ui/battle/cmd/right_on.png): the katakana バッチリ. It belongs to the unused debug badge Right On! and was never localized. jump.c:41-42 creates it (HIDX_RIGHT_ON); jump.c:114-118 shows it when acs->autoSucceed && acs->showHud; hammer.c:187-193 the same. Our cheat (src/battle/action_cmd.c:291-296, pm_cheat_action_on()) sets autoSucceed and keeps showHud, so the art shows. The game's own auto-succeed flag sets showHud = false.

## Fix
1. src/battle/action_cmd/jump.c lines 117-118: `if (acs->showHud` gets `#ifdef TARGET_3DS && !pm_cheat_action_on() /* the Right On! art is Japanese in every ROM (unused debug badge): the cheat keeps the A prompt only */ #endif` before `) {`. Prototype `int pm_cheat_action_on(void);` at the top in an #ifdef TARGET_3DS block (as action_cmd.c:4-7).
2. src/battle/action_cmd/hammer.c lines 190-193: inside `if (acs->showHud) {`, `#ifdef TARGET_3DS if (!pm_cheat_action_on()) #endif` before `hud_element_clear_flags(hid, HUD_ELEMENT_FLAG_DISABLED);`; the `break;` stays inside the if. Same prototype.
3. Switch `acmd` (3ds/src/pm_warp.c battle_tick, just before load_battle(formation)): `if (pm_debug_has("acmd")) gPlayerData.hasActionCommands = true; /* test: action command prompts without the Lucky Star */`.
Check the line numbers against the code first.

## Repro
Scenario + bench dmg_jump_ac (same commit): `warp=kmr_20:0 battle=BTL_KMR_1_FORMATION_00:BTL_KMR_1_STAGE_01 battlef=30 acmd cheats=action autoa det fixedseed shots` (confirm the cheat token `action` with grep has_token 3ds/src/pm_dbg_game.c). Until-line `cheat: perfect action command, auto success (mario)` (action_cmd.c:294); shotwin burst from that event, 4 shots 6 frames apart; cap 25 s. Before: blue A button with バッチリ to its right. After: only the A button.

## Start state
Worktree build3ds/wt/righton, branch righton from main.

## Done criteria
1. Build; nd_host PASS.
2. dmg_jump_ac shots before and after (open them).
3. A second run without cheats=action: the A prompt still shows.
4. spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015 byte-equal to a build of main.
Affected scenarios (no others): dmg_jump_ac (with and without the cheat), the 4 byte gates.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=righton PM_ROW=righton on every build.sh and bench call. No git stash. No toggles, no stubs.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=righton PM_LANE=righton sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=righton PM_LANE=righton python 3ds/tools/bench.py run <bench> --row righton --lane righton --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "righton ready <hash>".
