# PAUSE2 diagnosis: pause_menu close

Row PAUSE2, diagnostic. Azahar starts used: 2 of 6 (both pause_menu, scenario string changed between them).

## Question 1: repeated warp during the pause (coordinator request)

Run log: `%APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt`, run 2 of PAUSE2.

- `warp: mac_00 entry 4 (area 1 map 1) story -107 flag 0`: 1 line, log line 117, frame 15 to 51 (the single map load).
- `warp: scene ...` lines: 94 lines. Frame 51 (`frame0 51 scan 66`) to scene f114. Written by `3ds/src/pm_warp.c:470` (frame0) and `3ds/src/pm_warp.c:742` (scene fN, every 2 frames while padscript is on).
- `warp: smoke done`: not present.
- Game mode lines: mode 4 at frame 51 (field), mode 10 at frame 96 (pause open), mode 11 at frame 141 (unpause), mode 4 at frame 147 (field).

Conclusion: no second warp. The map loads once. The pause does not reload the map. The `warp:` lines on the bottom screen are the scene log printed as text by the port HUD (`shotbottom`), not a map change. The source of the string is `pm_warp.c:742`, which runs only with padscript.

## Question 2: why the old scenario failed to close

Cause: the scenario timing, not the game.

- The old string `wait80,START*3,wait284,START*3` puts the second START at scan 66 + 83 + 284 = scan 433. With the mapping frame = 51 + (scan - 66) / 2 (from the vrambase log), that is about frame 234.
- The old until window was frames 192 to 200 and the old pause window was 170 to 181. The close START was after both windows, so the close did not appear.
- The START button is correct: `src/pause/pause_main.c:749` closes on `BUTTON_START` edge. The tutorial gate (`pause_main.c:737`) applies only when `GF_Tutorial_Badges` is set, and that flag is set only in the kmr_02 partner scene (`src/world/area_kmr/kmr_02/npc.c:390`). The toad preset does not set it.
- Input lock after open: `src/state_pause.c` ignores input for about 16 steps after the open (StepPauseDelay).
- Regression check: not run. The new string closes the menu in the current build (game mode 11 at frame 141), so no game regression was shown for the close.

## Fix (scenario only)

`3ds/tools/azt_scenarios.json`, entry pause_menu:
- debug: `preset=toad padscript=wait80,START*3,wait95,START*3,wait200 shots shotwin=120:40 shotbottom=120:40 loglevel=debug`
- until: `shot \d+ frame 15[6-9]\b`
- pause window: frames 120 to 137. Field window: frames 152 to 159.
- status: verified (log checks 4 of 4; agent read the pictures, person to confirm).

## Open point

The Paper Mario status bar (top screen, HP and FP) is retracted after a pause close (`src/inventory.c:1529`, `reset_status_bar`). The field shots at frames 147 and 155 show no status bar. The HUD in the field shot is the port text on the bottom screen. A person must decide whether the status bar must show in the field shot. This would need a different scenario, not a game fix.
