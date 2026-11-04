# Handoff flk1 (FLK1, sbk_99 and nok_02 flicker), 2026-11-03

Branch flk1 (worktree papermario-3ds-port-flk1, from native ca4ea0f46). Budget: 1 Azahar start of 3 (diagnostic row FLK1).

## Commits
- 20c82555f FLK1: tritrace debug switch (`tritrace=<frame>:<count>`, draw_tri_cpu, max 400 lines) and shared `cull_cross` helper for both cull copies. Same math as before.
- Scenario nok02_tri (azt_scenarios.json) and the catalog row (test_catalog.md). Commit follows this file.

## State
- Step 1 done: trace switch builds (lane flk1, build3ds/lane-flk1/pm_3ds.3dsx).
- Step 2 run 1 (nok02_tri, pos 250 0 11, frame 200): 10 triangles reach draw_tri_cpu. All `early-keep`. All corner w > 0 (548 to 662). No cull or clip reject. Log has no dlc2 line.
- The trace does not show the path end piece. Most map geometry runs through the DLC2 static groups (pm_dlc.inc.c, dlc_static_group) and the GPU path gv_tri (pm_gbi.c, pm_gpu_gtrip). Their cull is not in draw_tri_cpu. Cull for those paths is in pm_gpu.c (not in this row's files).
- Steps 3 and 4 not done. No fix applied.

## Open items
1. Find which path draws the nok_02 end piece (one drawlog or dlc2 line in a run with tritrace). If it is the static group path, the cull sign is in pm_gpu.c. That file belongs to row native's agent.
2. The shot of nok02_tri does not confirm the end piece by eye.
3. The sbk_99 tower (-10 25 -147) is not traced yet.

## Commands
- Build: `PM_LANE=flk1 sh 3ds/build.sh` (assets/us and ver/us/build are copies of the main repo).
- Test: `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 nok02_tri -Row FLK1 -Build flk1 -Try`
- Shot to PNG: a PPM to PNG script with zlib (Python at /c/Python314/python).

## Run 2 and run 3 (2026-11-03, nok02_tri_dd: drawlog=200:1 turns DLC2 and the GPU path off)
- Run 2 (cap 400): the first 400 triangles of frame 200. 0 odd-negative-w triangles with a screen footprint.
- Run 3 (cap 4000, commit after ed7fc923b): 1209 triangles of frame 200 on draw_tri_cpu. Early-clip rejects 815 (798 even negative w, 15 odd negative w, all off screen by a crude footprint test). Early-keep 331 and early-cull 61 (all even negative w, all with footprint). Clip 4. Odd-negative-w visible keep or cull: 0.
- Result: the odd-negative-w flip at pm_gbi.c (cull test, both copies) does not change any visible decision in frame 200 at nok_02 pos 250 0 11. The hypothesis is not supported for this camera.
- Also: map triangles run through the CPU path only with drawlog. Without it, DLC2 static groups and gv_tri draw the map (run 1: only 10 triangles reached draw_tri_cpu).
- The nok_02 end piece is still not identified. Steps 3 (two camera positions) and 4 (fix) are not done. The FLK1 diagnostic budget is at 3 of 3. The orchestrator resets it (-ResetBudget FLK1) before further starts.
- Next: identify the chunk of the end piece in the dd lines (scr bounds around Mario, shot mapping to scr is unverified), then compare the same chunk at pos 250 0 11 and at a point a few units off. Check the GPU path (gv_tri, pm_gpu_gtrip) for that chunk if DLC2 draws it.
