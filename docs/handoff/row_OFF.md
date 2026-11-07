# Row OFF: off-screen targets, grab, noise go native

## Goal
nd_raw_native accepts off-screen targets, grab images and noise, then the off-screen CPU close is deleted. Plan: docs/handoff/plan_walker_end.md (read Decisions, Streams and ownership, Rows OFF-1 to OFF-4).

## Start state
Branch nds1, HEAD 3f6ce7a01. CEN-1b (refusal split) is committed before you start. OFF-3 waits for STALE2 (pm_tex.c): the orchestrator tells you when.

## Rows (in order, one commit each)
OFF-1 TARGET, OFF-2 GRAB, OFF-3 UNIT (when told), OFF-4 DELETE: as the plan rows say.

## Touch only
3ds/src/pm_gpu.c pm_gpu_off_begin, pm_gpu_off_end, grab_impl, off_snapshot, the off/grab/noise/unit0 refusal lines of nd_raw_native, noise_bound, the off-screen branches of flush_draw; 3ds/src/pm_tex.c pm_tex_native_unit0 off-screen branch (OFF-3 only); own hunks of azt_scenarios.json, bench_defs.json.

## Done criteria
1. OFF-1: why nraw2 = 0 at pause_spin_grab, battle_start_fb; shots right by eye.
2. OFF-2: why nraw3 = 0 at pause_spin_grab and the motion_blur_flame part of sbk56_superblock.
3. OFF-3: why nraw8 and nraw7 = 0 at intro_native, nok11_splash.
4. OFF-4: builds; spot_mac01_pic byte-equal.

## Rules
CLAUDE.md rules 5, 7, 9. Decisions D1 to D10 of docs/handoff/plan_walker_end.md are fixed. Other agents edit pm_gpu.c, pm_gbi.c, pm_tex.c, nd_*.c at the same time: edit only the functions your stream owns (plan table "Streams and ownership"); commit only your hunks through your own temporary index (GIT_INDEX_FILE copied from HEAD), never the shared index; no git add -A, commit -a, reset, stash, checkout, restore. Build in a scratch tree from HEAD plus your diff (git archive HEAD, apply your diff, copy assets/us and ver/us/build, gen_map_restore.py, copy 3ds/src/map_restore_table.c), lane named after your row; azt.ps1 <scenario> -Row <row> -Build <lane>. After each code commit build the committed tree clean. Status line in 3ds/logs/perf/status_<row>.txt before each Azahar start. Budget: reset (powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget <row>) and continue. No toggles (debug switches allowed, removed in the deletion row), no stubs, no push, no attribution lines, never commit 3ds/src/map_restore_table.c. Two tries per named cause, then the next code point. A new scenario goes into azt_scenarios.json and bench_defs.json in the same commit. Delete this file in the commit that completes the last row.
