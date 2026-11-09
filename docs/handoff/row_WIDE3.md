# Row WIDE3: chapter screen in 4:3, battle command wheel in the corner (wide on)

## User facts
User (2026-11-09), widescreen on: (1) "the new chapter screen when you start a new chapter. Like the intro it should be the original
letterboxed/aspect ratio mode as the original game." (2) "the battle control system for the player. Its locked in the position for if
it was in the original aspect ratio so its floating in the middle of the screen. It should be in the corner. some scales or
transformations might need to be made to maintain the same relative size."

## Start state
Worktree build3ds/wt/WIDE3, branch WIDE3 from main (hash in the first commit). PM_ROW=WIDE3, PM_LANE=WIDE3.

## Code facts
- The intro rule: src/game_modes.c:262-276 sets pm_wide = pm_wide_cfg && not (logos, title, language/file select, GAME_MODE_INTRO),
  with log lines `intro: native aspect on/off`.
- The chapter screen is map kmr_22 (src/world/area_kmr/kmr_22/main.c:40 fx_chapter_change; effect src/effects/chapter_change.c).
- Battle wheel: src/battle/btl_states_menus.c:605-606 BattleMenu_BasePosX = pm_wide_vx(54), BasePosY = pm_wide_vy(173);
  include/pm_wide.h:7 pm_wide_vx(x) = x + (x - 160) / 5 (a point in the 6/5 zoomed battle view, UA4). In wide mode this puts the wheel
  near the 4:3 position, so it floats toward the middle of the 400-wide screen.

## Plan
1. Chapter screen: the same native-aspect rule as the intro while the current map is kmr_22 (on map load into kmr_22 and back on leave),
   with log lines `chapter: native aspect on/off`. Put the rule at the one place that sets pm_wide for map changes (find it; if only
   set_game_mode sets it, add a map check next to it, not a second copy of the rule). The letterbox look must match the intro.
2. Battle command wheel: anchor to the bottom-left corner of the wide screen at the same margin from the screen edge as the original
   game has from the 4:3 edge (x 54 from the left edge of 320 -> 54 from the left edge of the wide view; y the same bottom margin).
   Keep the same size relative to the screen height (the original pixel size; do not stretch). Check every element that moves with
   BattleMenu_BasePosX/Y (wheel icons, title box, the arrow, the submenus that open from the wheel: item/strategy/star power lists,
   partner wheel) and the hud elements drawn with them; each follows the same anchor. Read how pm_wide transforms HUD coordinates
   (pm_wide.h, hud_element.c) to choose the right space.
3. If a submenu list was already left-anchored for wide, keep it; list in the commit what moved.

## Done criteria
1. nd_host PASS. memcheck OK.
2. Picture: one battle shot with wide on (battle scenario, warp=random is fine if a battle starts; else battle_start_f182_cold with
   wide): the wheel in the bottom-left corner, same size as 4:3, Mario and the enemy unchanged. One shot of a submenu open (Items).
3. Picture: one kmr_22 chapter screen shot with wide on: letterboxed 4:3 like the intro; log `chapter: native aspect on`.
4. Byte gate: battle_start_f182_cold with wide OFF byte-equal to base (4:3 path unchanged).
Affected scenarios (no others): the battle wide shot, the kmr_22 shot, battle_start_f182_cold.

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no toggles, no stubs. Commit after each passed step. Never commit 3ds/src/map_restore_table.c.

## At the end
Delete this row file in the commit that completes the row. SendMessage main "WIDE3 ready <hash>".
