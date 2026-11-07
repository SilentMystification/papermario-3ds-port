# Row CORE: record mode, NdRec kinds, rect/fill and range records

## Goal
Producers write typed native records (NdRec) into the main list instead of N64 words, so pm_gbi.c can be deleted (S10). Plan: docs/handoff/plan_s10_path.md (read Producer inventory, Streams, Stream CORE, Decisions D1 to D10). Plan decisions are fixed.

## Start state
Branch nds1, HEAD 4f3daf97a. nd_gbi.h bridge leaves force-included in main_loop.c, cam_main.c, background.c, background_gfx.c (CMakeLists.txt near line 338). gbi_task (pm_nusys.c) cuts the main list as one LEGACY span.

## Rows (in order, one commit each)
CORE-1 RECORD MODE, CORE-2 NdRec KINDS, CORE-3 RECT/FILL (after RECT-1 lands: grep git log for "RECT-1"; if not in, hand back after CORE-2 with CORE-3 open), CORE-4 RANGE (after RECT-2b lands, same rule). Text: plan Stream CORE. CORE-5 to CORE-8 are not in this run.

## Touch only
3ds/tools/gen_nd_gbi.py, 3ds/include/nd_gbi.h, 3ds/include/nd_gbi_leaf.h (generated), new 3ds/include/nd_rec.h, 3ds/include/nd.h (NdRec kinds), 3ds/src/nd_core.c (nd_emit, nd_bridge_cut), 3ds/src/nd_state.c, 3ds/src/pm_nusys.c gbi_task, 3ds/CMakeLists.txt (opt-in list), 3ds/tests/ or 3ds/tools/nat_host_test.c (host test). pm_gpu.c only as a caller of its API (pm_gpu_fill_rect, pm_gpu_tex_rect, pm_gpu_gtrip), never an edit. pm_gbi.c: no edit.

## Done criteria
1. CORE-1: host test setter state = nd_words state for every leaf; spot_mac01_pic byte-equal with no file opted in.
2. CORE-2: main_loop.c and cam_main.c opted in; spot_mac01_pic byte-equal; nd_emit order log equal to the walker op order (ndref).
3. CORE-3: background.c, background_gfx.c opted in; walker ops of camera_render and the 2 background lists 0 (WCEN census); mac_00 sky right by eye.
4. CORE-4: walking_dust.c opted in; its walker ops 0; dust right by eye at mac_01.

## Rules
CLAUDE.md rules 5, 7, 9. Decisions D1 to D10 of docs/handoff/plan_walker_end.md are fixed. Other agents edit pm_gpu.c, pm_gbi.c, pm_tex.c, nd_*.c at the same time: edit only the functions your stream owns (plan table "Streams and ownership"); commit only your hunks through your own temporary index (GIT_INDEX_FILE copied from HEAD), never the shared index; no git add -A, commit -a, reset, stash, checkout, restore. Build in a scratch tree from HEAD plus your diff (git archive HEAD, apply your diff, copy assets/us and ver/us/build, gen_map_restore.py, copy 3ds/src/map_restore_table.c), lane named after your row; azt.ps1 <scenario> -Row <row> -Build <lane>. After each code commit build the committed tree clean. Status line in 3ds/logs/perf/status_<row>.txt before each Azahar start. Budget: reset (powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget <row>) and continue. No toggles (debug switches allowed, removed in the deletion row), no stubs, no push, no attribution lines, never commit 3ds/src/map_restore_table.c. Two tries per named cause, then the next code point. A new scenario goes into azt_scenarios.json and bench_defs.json in the same commit. Delete this file in the commit that completes the last row.
