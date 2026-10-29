# Route test (scene tour): handoff

## Status
- Scene tour works: 11 scene lines in one 40 s Azahar run (tour ends at `tour: done`, 10 listed maps plus the warp scene).
- Load wall ms per scene: 1794 to 2260 ms (from `tour: next` to `tour: in map`).
- Steady frame ms: only for the first 2 scenes. pc lines (phasecsv) stop at about frame 300, so later scenes show n/a.
- Per-scene shots: NOT produced. The `shotmode` burst logs `shot <n> frame <f>` (217 lines), but no file reached `sdmc:/3ds/PaperMario/shots/`. dump_shot in 3ds/src/pm_gpu.c returns before the write. Not fixed (pm_gpu.c belongs to another agent).
- ops, closes: no field in the log (n/a).
- sdrd: 0 in all scenes (ROM resident, no SD reads).
- Title and file select: not in the tour. The intro (game mode 16) lasts about 2 min game time and START does not skip it (tested). A route from the title cannot finish in 90 s.
- Boss, water/effects map: not in the list (no boss fights by user decision; no water map picked yet).
- Battle and pause segments: removed. The padscript route (3ds/routes/play1.txt style) stalled on its building segment (see below).

## Command (one run, PM_ROW=route, PM_LANE=route)
    PM_ROW=route PM_LANE=route powershell -NoProfile -File 3ds/run_azahar.ps1 -Until "tour: done" -Timeout 90 -Debug "warp=mac_00:4 story=ch0_toad_town tour=mac_00:4,mac_01:0,dgb_01:1,kmr_09:0,kmr_20:4,hos_00:0,kmr_02:0,kmr_10:0,trd_01:0,mac_00:0 shots shotwin=999999:1 shotmode=4:21 phase phasecsv"
    python 3ds/tools/route_scenes.py build3ds/lane-route/last_log.txt

Build first (lane route): `python 3ds/tools/gen_map_restore.py`, then `PM_LANE=route sh 3ds/build.sh`, then put back 3ds/src/map_restore_table.c from your own copy (never commit it).
Repeat with the same arguments: run_azahar caches the result (sha, args, rc 0). Add `-Force` to run again.

Switch notes:
- `tour=` visits each map with a real map change (pm_warp.c tour_step), 40 frames each (TOUR_FRAMES). The settle window in the report is 20 frames (`--settle`).
- `shotwin=999999:1` stops the one-shot-per-30-frames default writer. `shotmode=4:21` starts a 21-frame burst at each change to mode 4 (world).
- `phase phasecsv` gives the pc lines (per-frame wall w, 0.1 ms units). Without it steady_ms is n/a. It slows the game.

## Game source change (separate commit)
- pm_warp.c: `tour: next ... t=<ms>` (load start time). Used for load_ms.
- padscript event tokens `ev_title ev_file ev_world ev_map ev_battle ev_pause` (pm_route.c, pm_input.c). Each waits for a game mode and logs `route: ev <name> f<step> t=<ms>`. Verified in one run: ev_file and ev_world fired. ev_map failed in the first version (the map compare held before the token armed) and is fixed (no map compare, it waits for the world after a map change). ev_map, ev_battle and ev_pause are not verified in a run.

## Padscript route (3ds/tools/route_test.txt, removed)
Tried and stopped: START presses in the intro, ev_file at about frame 528 (17 s), world at frame 653, goto dgb_01:1 at frame 698 with no ev_map line, and the 90 s run ended in the kmr_20 map. The yard walk token `go_0_0_300` reported `leg blocked`. Coordinates were guessed, not measured.

## Open items
1. Scene shots: find why dump_shot writes no file (check the `src` and `target` values in pm_gpu.c), or add a capture in run_azahar.ps1 at each `tour: in map` line.
2. Steady ms for all scenes: pc lines need the logger to keep writing after frame 300 (check the phasecsv cap).
3. Title and file select: needs a switch that skips the intro, or a save state.
4. Water/effects and boss maps: pick map names and add them to the tour list.
