# agent_poswarp: row POSWARP, pos= switch (place Mario at x y z)

Branch: poswarp (worktree ..\papermario-3ds-port-poswarp, from main 99266ab2d). Not merged.

## State
- Code: `pos=<x>:<y>:<z>[:<angle>]` in 3ds/src/pm_warp.c (pm_pos_parse, pm_pos_set, pm_pos_log, pos_tick). Hooked in pm_warp_world_mode (once per map load, by area and map id) and pm_warp_tick.
- Padscript token `pos_<x>:<y>:<z>[:<angle>]` in 3ds/src/pm_input.c (go 4, no scan). NOT run on Azahar.
- Scenario kmr10_pos in 3ds/tools/azt_scenarios.json, catalog row in docs/handoff/test_catalog.md (section POSWARP).
- Docs: docs/HISTORY.md, Switches section.
- Lane build: PM_LANE=POSWARP sh 3ds/build.sh, build3ds/lane-POSWARP/pm_3ds.3dsx.
- Worktree setup: assets/us and ver/us/build copied from the main folder (gitignored). map_restore_table.c copied, never committed.
- Build note: strtof is declared by hand in pm_warp.c (stdlib.h conflicts with the decomp headers).

## Behaviour
- Placement at 30 world frames after the map entry (POS_DELAY). Readback log line 10 frames later (POS_READBACK).
- Log: `poswarp: placed Mario at x y z on map M yaw a (readback)`.
- Writes: pos, yaw (only with angle), curSpeed 0, timeInAir 0, curFloor NO_COLLIDER, idle action state, camera targetPos. Camera follows by its normal update.

## Open
- Azahar start 1 of 8 (kmr10_pos, PM_ROW POSWARP): log line `poswarp: placed Mario at -142.000 97.265 221.000 on map kmr_10 yaw 90.00 (readback)` at scene frame 53 + 40. x and z equal the request. y did not stay at 136: gravity put Mario on the floor at y 97.265 within 10 frames (no floor at y 136 under the spot). The expected event with y 136 fails. The azt marker check reported `marker missing: warp: kmr_10 entry 0` although the line is in build3ds/lane-POSWARP/last_log.txt (line 114). Picture: not read.
- Decision needed: keep y 136 (Mario falls), or hold y until the floor is found, or accept the floor y. The test expects 136 and must change to the agreed rule.
- Padscript token: untested.

## Commands
- Test: `PM_ROW=POSWARP powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 kmr10_pos -Row POSWARP -Build POSWARP -Try`
- Justification: 3ds/logs/perf/status_POSWARP.txt, one line per start.
