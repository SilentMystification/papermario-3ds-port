# Agent HUDMAP (2026-11-03)

## State

Two console requests are done in the code and tested in Azahar (picture read by the code agent).

1. MAPHUD: the bottom screen top row shows the map after each map change. The row reads `Now in <map id> (area A, map M)` for 60 steps (2 s at 30 steps per second, 120 at 60), then `Map <map id> (area A, map M)`. Shown only with the HUD option (`pm_opt.dbg_hud`, on by default). The port has no English map name table. The game table `gAreas[].name` holds Japanese debug names, so the row shows the map id and the area and map index.
2. Exit row red: in the port menu (SELECT), after one A press on `Exit to Homebrew Menu`, the value `A again` is drawn in red (ANSI colour 31). Other rows stay white.

## Code

- `3ds/src/pm_route.c`: `pm_map_hud_step(step, hold)`. Per step: two loads and two compares. Text is built (one snprintf) on a change and again at the end of the hold.
- `3ds/src/pm_gpu.c`: `pm_gpu_hud_map()` copies the row. `pm_gpu_hud_flush` draws six rows: cursor row 1 is the map row (top of the screen), rows 2 to 6 are FPS, frame time and the three memory rows. The log window starts at cursor row 7 (was 6). `menu_put_line` adds a reset (`ESC[0m`) after each cursor move, so a red value does not stay red.
- `3ds/src/pm_main.c`: one call in the HUD block (`pm_map_hud_step`, hold = 2 s of steps).
- `3ds/src/pm_options.c`: `pm_optui_alert()` is 1 when the exit row is armed.
- `3ds/src/pm_options_ui.c`: value drawn with the red escape when `pm_optui_alert()` is 1.
- `3ds/include/pm_port.h`: three declarations.

## Tests (lane `hudmap`, HEAD 617540255 plus these edits)

| Run | Scenario | Result |
|---|---|---|
| 1 | mapchange_hud | Text drawn at screen row 0, which wrapped to the bottom under the log. Changed the row layout. |
| 2 | mapchange_hud | Text at top row. Picture bot_0211: `Now in mac_01 (area 1, map 2)`. |
| 3 | mapchange_hud (window 226 to 237) | bot_0228 `Now in`; bot_0231 and bot_0234 `Map mac_01 (area 1, map 2)`. Hold end confirmed. |
| 4 | exit_armed | bot_0203 and bot_0205: `A again` red, other rows white. |

Starts used: 4 of 8 for row HUDMAP. Status lines: `3ds/logs/perf/status_HUDMAP.txt` (ignored by git).

## Open items

- Console check by the user: the row 1 text and the red value on the N3DS and O3DS consoles.
- The log window is one row shorter (24 rows, cursor 7 to 30).
- The shared file `3ds/tools/azt_scenarios.json` was changed by another session during this run (entry `tex_kmr_11` added in the working tree, and my two entries were removed once and restored). Only my two entries are in the commit.
- Shot runs use the bottom dump (`shotbottom`) every drawn step. The top dump (`shotwin`) writes only drawn steps, so a `until` on a frame in the window can miss. Use a window of several frames.

## Next steps

- If the user wants English map names, add a table keyed by map id. The table is new data, not in the port.
- No further runs are needed for HUDMAP.

## Commands

- Build: `python 3ds/tools/gen_map_restore.py`, then `PM_LANE=hudmap sh 3ds/build.sh`, then restore `3ds/src/map_restore_table.c` from the saved copy (never committed).
- Test: `PM_ROW=HUDMAP powershell -File 3ds/tools/azt.ps1 mapchange_hud -Row HUDMAP -Build hudmap -Try` and `exit_armed` (same form).
