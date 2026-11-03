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

## Start 2 (kmr10_pos, PM_ROW POSWARP, budget 2 of 8)
- Log line now prints set and settled: `poswarp: placed Mario at -142.000 136.000 221.000 on map kmr_10 settled -142.000 97.265 221.000 yaw 90.00 (readback)`. x and z exact. Until changed to shot frame 56 to 60; the run ended at 21 s (Azahar exited), the until shot was not written. Shots written: frames 40 and 44 only.
- Top-screen shots at frames 40 and 44 are fully black (all pixel 0). Bottom shot 44 has content.
- Mario falls: scene f300 pos -142 20 221, vy -1913, actionState 0. He does not stop at 97.265. No floor under this spot near the wall at x -142 z 221, so the map gate (Mario on terrain) fails.
- Open: find the floor under the spot (ground probe at the placement, or another x z point). Azahar start 3 is not used.

## Start 3 and 4 (probe)
- Start 3: ray probe returned -1 at every frame, because hitDepth (max ray length) was not set. Start 4: fixed (hitDepth 1000).
- Start 4 probe: ray from y 400 at x -142 z 221 hits collider 28 at y 20.000 at f30 (before placement), f60, f120, f180. Player ground check returns 20.000 after the fall. kmr_10 has 48 colliders. No collider at y 136 on that spot, so the placement cannot stand at 136.
- Start 5 (shotwin 56, deadline 35): top shot frame 56 shows Mario on the path by the fence. Log checks pass (2 of 2). Settled y 20, the settle gate of 3 units around 136 is not met.
- Decision for the orchestrator: keep y 136 (falls to y 20), or change the kmr10_pos y to 20 for a pass on the terrain.

## Final state (orchestrator decision: y 20 gate)
- Floor probe (start 4, ray from y 400 at x -142 z 221, max length 1000): floor collider 28 at y 20.000 at scene frames 30 (before placement), 60, 120, 180. The ray misses at 136, so no surface exists at y 136 on this spot. kmr_10 has 48 colliders.
- Start 3 ray results are invalid (hitDepth unset). Start 5 settled at y 20, top shot frame 56 shows Mario on the path by the fence.
- Scenario kmr10_pos: pos=-142:20:221. Gate: x and z within 0.5, y settled within 3 of 20. Start 6 runs this gate (result in the catalog row).
- Scenario kmr10_pos_hi: pos=-142:136:221 (texture garble band view). Gate on x and z only. Not run.
- Azahar starts on POSWARP: 7 of 8. Start 6: Azahar exited at 1 s, no log. Start 7 (retry): Azahar exited at 21 s, no log. Both were not a test result. The shared Azahar log (sdmc log.txt) was written by other runs at 13:41. One start is left. kmr10_pos with the y 20 gate has not run yet.
- Padscript token pos_<x>:<y>:<z>[:<angle>]: built, not run.
- Branch poswarp: merged main (4fc505904) into it, then commits 13d73836a, d69a18551 and the y 20 commit.

## Final (budget reset, main 7b6e3735b merged)
- kmr10_pos (pos -142 20 221): start 8. Settled -142 20 221 exact, 2 of 2 log checks. Shot at frame 56: Mario on the path by the fence. PASS by log and picture (picture read by the code agent).
- kmr10_pos_hi (pos -142 136 221): start 9. Readback at frame 40 settled y 97.265 (falling), x and z exact. Shot at frame 56: Mario on the path. Gate x and z only.
- Azahar starts: 9 used, starts 6 and 7 gave no result.

## WARPFIX2 state (branch warpfix2, from main)
- Commits on warpfix2: 368800bff (block and boss keep), 32e868095 (HP line and save prompt log), 8e3fbd4a7 (scenario entries re-added after main's entries; JSON validated).
- Saves blocked by new-file states: boot warp without loadslot, file select or title warp, boss launch outside the world. Log: `saving disabled by ...` at the block, `warp: save blocked, slot N not written (...)` at each write attempt, `save prompt: save blocked (...)` when the save block prompt opens (pm_save_prompt, SaveBlock.c entity_SaveBlock_pause_game).
- A process restart clears the block. A file load does not lift it (fio_load_game also runs when browsing the file select).
- Boss tab in the world keeps the loaded save: before and after counts. Azahar start 4 (warpfix_boss): HP 10/10 FP 5/5 before the fight, 75/75 and 30/30 during it, 10/10 and 5/5 after the restore (level 1, BP max 3, items 2, coins 50, story -128 after). Hash unchanged (850cc147...).
- Scenarios: warpfix_boot_save (start 1, pass), warpfix_fswarp_save (start 2, pass), warpfix_boss (starts 3 and 4, pass). Azahar starts on WARPFIX2: 4 of 8.
- Not tested: the save prompt log (pm_save_prompt) needs a run that touches a save block. The file menu save path has no prompt log.
- Next commands: build `PM_LANE=WARPFIX2 sh 3ds/build.sh`; test `PM_ROW=WARPFIX2 powershell -File 3ds/tools/azt.ps1 warpfix_boot_save -Row WARPFIX2 -Build WARPFIX2 -Try` (same for the other two).

