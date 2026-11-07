# Row OFF: off-screen targets, grab, noise go native

## Goal
nd_raw_native accepts off-screen targets, grab images and noise, then the off-screen CPU close is deleted. Plan: docs/handoff/plan_walker_end.md (read Decisions, Streams and ownership, Rows OFF-1 to OFF-4).

## Start state (2026-11-06)
Branch nds1 at 9b5e3d20b (RECT-1b merged 56ee96c93, CEN-1b split 10528125d, AZT3). Work in your own worktree: git worktree add build3ds/wt/off -b off nds1; copy assets/us and ver/us/build; build there (PM_LANE=off); azt -Build <worktree 3dsx>. Hand back with the branch head; the orchestrator merges.
Census facts: pause and text menus have 999 walker rect draws per 300 frames from OFF-SCREEN batches (rectcpu: off_active 1, flush caller pm_gpu_off_begin, comb 8, 6 vertices; rect_raw_ok refuses off_active, nd_raw_native refuses nr2) plus nr8 (unit 0 refused) 210 at draw_msg. So OFF-1 also covers the rect path for off_active (rect_raw_ok and the nr2 line). Debug counters rectwhy/rectcpu exist on branch rect (c7423fc7c), not on nds1. STALE2 is closed on nds1 (300fd4cf1), so OFF-3 may run. The super block agent edits pm_tex.c in the shared folder: keep OFF-3 pm_tex.c hunks small.

## Rows (in order, one commit each)
OFF-1 TARGET, OFF-2 GRAB, OFF-3 UNIT (when told), OFF-4 DELETE: as the plan rows say.

## Touch only
3ds/src/pm_gpu.c pm_gpu_off_begin, pm_gpu_off_end, grab_impl, off_snapshot, the off/grab/noise/unit0 refusal lines of nd_raw_native, noise_bound, the off-screen branches of flush_draw; 3ds/src/pm_tex.c pm_tex_native_unit0 off-screen branch (OFF-3 only); own hunks of azt_scenarios.json, bench_defs.json.

## Done criteria
1. OFF-1: wcen_pause walker wd from 1218 to about 9 (off-screen rects and nr2 native); why nraw2 = 0 at pause_spin_grab, battle_start_fb; shots right by eye.
2. OFF-2: why nraw3 = 0 at pause_spin_grab and the motion_blur_flame part of sbk56_superblock.
3. OFF-3: why nraw8 and nraw7 = 0 at intro_native, nok11_splash.
4. OFF-4: builds; spot_mac01_pic byte-equal.

## Rules
CLAUDE.md rules 5, 7, 9. Decisions D1 to D10 of docs/handoff/plan_walker_end.md are fixed. Other agents edit pm_gpu.c, pm_gbi.c, pm_tex.c, nd_*.c at the same time: edit only the functions your stream owns (plan table "Streams and ownership"); commit only your hunks through your own temporary index (GIT_INDEX_FILE copied from HEAD), never the shared index; no git add -A, commit -a, reset, stash, checkout, restore. Build in a scratch tree from HEAD plus your diff (git archive HEAD, apply your diff, copy assets/us and ver/us/build, gen_map_restore.py, copy 3ds/src/map_restore_table.c), lane named after your row; azt.ps1 <scenario> -Row <row> -Build <lane>. After each code commit build the committed tree clean. Status line in 3ds/logs/perf/status_<row>.txt before each Azahar start. Budget: reset (powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget <row>) and continue. No toggles (debug switches allowed, removed in the deletion row), no stubs, no push, no attribution lines, never commit 3ds/src/map_restore_table.c. Two tries per named cause, then the next code point. A new scenario goes into azt_scenarios.json and bench_defs.json in the same commit. Delete this file in the commit that completes the last row.
