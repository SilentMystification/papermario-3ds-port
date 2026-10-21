# ROUTE notes: scripted play-through benchmark (switch `route=<name>`)

## Why
The golden scenes and the demo cycle keep Mario idle in one scene. A route walks Mario through maps, enters a battle, opens the pause menu and saves. It gives one table per segment, in Azahar and on the console, from the same script.

## Files
- `3ds/routes/play1.txt`: the route (90.6 s, 2718 game steps). Copy it to the SD card: `sdmc:/3ds/PaperMario/routes/play1.txt`. The game reads `routes/<name>.txt` from the app folder (the game does `chdir("sdmc:/3ds/PaperMario")`). `padscript=@routes/play1.txt` does the same.
- `3ds/src/pm_input.c` (padscript parser, UI-1 code): new tokens, bigger buffers (2500 tokens, 24 KB), `#` comments, switch `route=<name>`.
- `3ds/src/pm_route.c` (new): walk to a point, map change, save, state line. One line in `3ds/CMakeLists.txt`.
- `3ds/tools/route_report.py` (new): per-segment table and the determinism check.

## Switches and start state
`warp=mac_00:4 story=ch0_toad_town partner=1 route=play1 phase phsum phasecsv loglevel=debug fixedseed statehash=300`
`warp` and `story` set the start. Without them the route starts at scan 0. The switch string holds 256 bytes. `phasecsv` gives the pc line of every step (route_report.py needs it). Without `phasecsv` the tool uses the phs lines (coarse).

## Tokens (added for the route)
Unit of `wait<n>` and `*<n>`: pad scan. The main loop scans the pad once per loop pass and the game takes a step every second pass, so 2 scans are 1 game step. The count follows the loop, not the clock and not the drawn frames.
- `seg_<name>`: no scan used. Logs `route: seg <name> f<step> scan <n>`. The report cuts its table at these lines.
- `go_<x>_<z>_<max scans>`: the circle pad walks Mario to the world point (x, z) and the token ends when Mario is within 24 units, after the max scans, or when Mario does not move for 90 scans (`route: leg blocked at x z`). The stick angle uses the camera yaw, as `player_input_to_move_vector` does. The input depends only on the game state, so a run repeats exactly. The stick tokens `up down left right` (circle pad, 80 of 80) also work. No new circle pad token was needed.
- `goto_<map>:<entry>`: real map change, the same code as one step of the `tour` switch. Takes no scan. Use it where the walk would take too long or the path is closed. Logs `route: goto`.
- `save_<slot>`: `fio_save_game(slot)`, the call of the save block. Logs `route: save slot <n> f<step> took <ms> ms`. WARNING: on the console this writes the real save slot. The route uses slot 3. Copy `sdmc:/3ds/PaperMario/` save files before the run if slot 3 holds data.
- D-pad: `k_dup k_ddown k_dleft k_dright` (3DS keys, UI-1). In the pause menu and in battle the Multifunction mode makes them stick moves. In the free overworld they are C buttons, so the route walks with `go_` and the circle pad.
- Every 30 scans the game logs `route: st f<step> mode <m> map <area>:<map> bs <battle state> bs2 <sub state> hp coins`.

## Segments of play1 (the map change types)
town_walk (mac_00 Gate District, walk to the west gate) -> map_entry_field and field_walk (kmr_10, walked map change) -> goto mac_01 plaza (town, many NPCs: spawn) -> goto kmr_09 Goomba Road (field with NPCs) -> road_walk into the Goomba -> battle_transition -> battle (menu with the D-pad, Jump, partner, win) -> battle_exit -> road_back (walk to the west edge, walked map change to kmr_02, Goomba Village) -> village_walk -> goto dgb_01 (interior, Tubba's Castle hall) -> goto hos_00 (Starborn Valley, wide background) -> pause_menu (START, tabs, A, B) -> save -> walk_on.
Not covered: shop or popup menu (no shop is reachable by a short walk from these maps), a story NPC spawn scene (the maps with entry loads give the NPC spawn load), the `trd_01` interior (Mario is blocked at x -216 there).
The battle takes 1087 steps (36 s). The battle is the game's own timing: the enemy turn and the animations run at game speed.

## Determinism
Three runs of build 965-laneC (Azahar fast, Azahar normal speed with `PM_FL=100 ... -F`, Azahar fast again): `statehash` equal at the 9 samples (f300 to f2700), run hash 2440c7c2, 21 seg lines equal. Check: `python3 3ds/tools/route_report.py A.txt --compare B.txt`.
With `fs1force=5` (every fifth draw skipped) the route ends 2 steps late (2720) and 4 of 9 samples differ in the game state: the inputs follow the loop count, but a skipped draw changes a little game logic. Read per-segment numbers of a console run with a tolerance of 2 steps in the segment bounds.
Hardware: the route needs the same build and the same save-independent start (`warp`, `story`, `partner`). The statehash of the console equals the Azahar value only when the build logic is equal. Compare a console run with a second console run of the same build, and the seg lines with Azahar.

## Report
`python3 3ds/tools/route_report.py LOG --title "<text>"`
Columns: steps, cpu avg/p95/max (w - i - fb of the pc line, ms), wall avg/p95/max (the pc `w`, real time on the console: paced 33.3), `over33` (cpu over 33.3 ms), `wall36` (steps whose wall time is over 36 ms: late for the player), texmissT, flushT (sums), tri and draw per step, game modes and map ids of the segment. Second table: the phase averages per step (lo dl gb gp tx au ro y fb fe ot gc gd). The footer gives the goto lines, the save time and the statehash samples. Azahar numbers are host numbers. Only the console numbers count.
Reference table (Azahar, build 965-laneC): `3ds/logs/perf/route_ref_965-laneC.txt` (git-ignored). Short form:

| segment | steps | cpu avg | cpu p95 | cpu max | steps cpu over 33 | note |
|---|---|---|---|---|---|---|
| town_walk (mac_00) | 278 | 18.3 | 20.8 | 39.8 | 1 | gb 12.4 ms, tri 230 per step |
| map_entry_plaza (mac_01 load) | 40 | 15.7 | 43.6 | 72.6 | 2 | flushT 7272, 26.5 draws per step |
| plaza_walk | 62 | 18.3 | 19.7 | 21.1 | 0 | gb 12.2 ms |
| map_entry_road, road_walk | 158 | 10.4 | 19.5 | 42.3 | 1 | |
| battle_transition | 80 | 12.7 | 15.5 | 16.1 | 0 | |
| battle | 1087 | 14.5 | 20.9 | 42.1 | 1 | texmissT 2182, wall over 36 ms: 14 steps |
| road_back (battle exit and map change) | 105 | 11.3 | 13.8 | 78.1 | 1 | flushT 8112 |
| map_entry_interior (dgb_01) | 40 | 11.8 | 17.4 | 47.0 | 1 | |
| pause_menu | 184 | 13.4 | 18.2 | 60.7 | 4 | gd 2.1 ms, wall over 36 ms: 10 steps |
| save | 30 | 13.6 | 14.2 | 14.5 | 0 | `save_3` took 0 ms in Azahar |

Findings from the table: (1) the heavy segments are the map entries (the peak steps are 43 to 78 ms of cpu with 2000 to 8000 flush calls in the segment: map entry and the return from battle), (2) the town walks cost most per step (18 ms, gb 12 ms), (3) the battle has the most texture misses (2182) and the most late steps (14), (4) the pause menu has 4 steps over 33 ms. Rows that matter most: map-entry load (flush calls, texture loads), the display list cost in town maps (gb), battle texture misses, the first steps of the pause menu.
