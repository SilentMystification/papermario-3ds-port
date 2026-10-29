# Route test (scene tour): handoff

## Status (route budget row route: 8 of 8 starts used, no starts left)
- Scene tour runs in one Azahar start, 40 s wall, ends on `tour: done`. 11 scene lines (the warp scene plus 10 tour maps).
- Shots: one file per scene, written. Files in build3ds/lane-route/shots/ (sorted, `<n>_<area>_<map>.ppm`, 240x400). Azahar writes them there, not under the Azahar SD folder. Contact sheet checked by eye: Mario and ground visible in all 11 at half scale. Shadow and exact subject not checked per file.
- Load wall ms per scene: 1285 to 1428 ms (tour: next to tour: in map). Warp scene: n/a.
- Steady frame ms: 33.4 in every scene. This is the 30 fps pacing (wall time per burst frame), NOT the game frame cost. Use the pc or phase lines for cost, and they stop at frame 266 (cause not found).
- ops (C_CMD) and closes (C_FLUSH): 0 in every scene. The counters are incremented (pm_gbi.c:2055, pm_gpu.c:764), but the values read at dump_shot are 0. Not explained. Open item.
- sdrd: 0 in every scene (ROM resident, no SD reads).
- Intro: the warp start skips it (the tour run never shows mode 16). START does not skip the intro in the padscript run (tested, the intro lasts about 2 min game time). Title and file select scene: NOT added (not reachable in 10 s).
- Boss, water or effect maps: not in the list yet.

## Command (PM_ROW=route, PM_LANE=route, lane build)
    PM_ROW=route PM_LANE=route powershell -NoProfile -File 3ds/run_azahar.ps1 -Until "tour: done" -Timeout 90 -Force -Debug "warp=mac_00:4 story=ch0_toad_town tour=mac_00:4,mac_01:0,dgb_01:1,kmr_09:0,kmr_20:4,hos_00:0,kmr_02:0,kmr_10:0,trd_01:0,mac_00:0 shots shotwin=999999:1 shotmode=4:21 phase phasecsv"
    python 3ds/tools/route_scenes.py build3ds/lane-route/last_log.txt --shots build3ds/lane-route/shots

Build first: `python 3ds/tools/gen_map_restore.py`, then `PM_LANE=route sh 3ds/build.sh`, then put back 3ds/src/map_restore_table.c from your own copy (never commit it).
`-Force` runs again with the same arguments (run_azahar caches a rc 0 result otherwise).

Switch notes:
- `tour=` visits each map by a real map change (pm_warp.c tour_step), 40 frames each (TOUR_FRAMES). The settle window in the report is 20 frames (`--settle`).
- `shotmode=4:21` starts a 21-frame burst at each change to the world mode. pm_gpu.c dump_shot writes only the last frame of the burst.
- `shotwin=999999:1` stops the one-shot-per-30-frames default writer.
- `phase phasecsv` gives the pc lines and the counters. It slows the game.

## Game code (separate commits)
- pm_gpu.c dump_shot: (1) the old `mkdir("shots")` and the relative path were replaced by `sdmc:/3ds/PaperMario/shots`. Files still went to the Azahar folder build3ds/lane-route/shots (the cwd of the run), so the result depends on Azahar's cwd. (2) One file per burst, the last frame. (3) File name `<n>_<area>_<map>.ppm`. (4) Burst line: `shot <n> frame <f> map a:m t=<ms> cmd <C_CMD> flush <C_FLUSH>`. (5) Diagnostic lines `shot: no colour buffer` and `shot: fopen ... failed` (did not fire in the last run).
- pm_route.c: `pm_route_mapid` (area and map of the game status, used by the shot name).
- pm_warp.c: `tour: next ... t=<ms>` (load start time).
- pm_input.c, pm_route.c: padscript event tokens `ev_title ev_file ev_world ev_map ev_battle ev_pause` (debug). ev_file and ev_world fired in the padscript test. ev_map was fixed (no map compare) but not verified in a run. ev_battle and ev_pause not verified.

## Padscript route (removed)
Stopped at the building step: the ev_map wait did not fire (fixed after that run). The title and intro could not be passed in 90 s.

## Open items
1. Steady game cost per scene: the pc and phase lines stop at frame 266 (the PHR ring or log writer; not found). Read pm_phase.c ph_format and pm_log.c drain after frame 266.
2. ops and closes read 0: check pm_ph_cnt reset order (pm_ph_frame_end at pm_main.c:467, dump_shot at pm_gpu.c frame begin) and PH_ADD gating.
3. Title and file select: needs a switch or save state that skips the 2 min intro.
4. Boss and water/effect maps: pick map names, add to the tour list.
5. Shot files: the cwd of Azahar decides the folder. Put the shots folder under the SD app folder before the next run.
