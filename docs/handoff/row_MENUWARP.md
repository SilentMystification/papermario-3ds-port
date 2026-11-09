# Row MENUWARP: a warp or boss launch from the Select menu during a battle closes the menu itself

## User facts
User (2026-11-09): "when warping in a battle, it says ending the battle then warp. I need to close the menu for the warp to happen. the
warp should close the select menu so we dont have to do it ourselves."

## Start state
Worktree build3ds/wt/MENUWARP, branch MENUWARP from main (hash in the first commit). PM_ROW=MENUWARP, PM_LANE=MENUWARP.

## Code facts
3ds/src/pm_warp.c:1016-1024 bypass_begin: sets the bypass, ends the battle (btl_set_state(BATTLE_STATE_END_BATTLE)) or unpauses,
shows pm_menu_msg("Ending the battle, then warp|boss") and returns 0, so the Select menu stays open and the game stays held until the
user closes it. pm_menu_set (extern at :1025) opens/closes the menu; pm_options.c:772 lists the action results (5 = warp started,
close the menu).

## Plan
1. When the bypass starts, close the Select menu the same way a normal warp closes it (return the "warp started" result 5 to the
   caller, or call pm_menu_set(0) at the point the normal warp path does; read which one the normal warp uses and use the same).
   The message stays visible as a footer/toast after the close if the existing message path supports it; else log only.
2. Same for the boss launch (kind 2) and the pause-mode case (GAME_MODE_PAUSE -> UNPAUSE).

## Done criteria
nd_host PASS; memcheck OK; Azahar: a battle (battle switch), open the Select menu, Warp area > a map, A: the menu closes, the log shows
`bypass: battle end` then the warp line and the new map, without a menu close input (padscript has no B/Select after the A). One shot of
the destination map.

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no toggles, no stubs. Never commit 3ds/src/map_restore_table.c. Padscripts in the azslots sdmc.

## At the end
Delete this row file in the completing commit. SendMessage main "MENUWARP ready <hash>".
