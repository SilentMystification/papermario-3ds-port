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

## Run 4 to run 8 (2026-11-03, flk1 lane). Debug switches only, no fix
Added (pm_dlc.inc.c, debug): `nobox=1` (dlc_try box skip off), `noboxint=1` (an out group runs through pm_gbi_run instead of the skip), dlc2 line fields `skip`, `grp`, `xgrp`; dlc_verify_skip counts `xgrp` (triangles of a skipped group that use a slot an earlier group of the list loaded). Scenarios nok02_nobox, nok02_noboxint, nok02_dlccheck (catalog rows added).

Results (pos 250 0 11):
- Run 4 nok02_nobox vs nok02_tri, frame 200, top-left shot: 2855 px differ (3.0 %), 60 regions. Largest in x 136 to 178, y 52 to 129 (1040 px) and x 185 to 219, y 52 to 113 (327 px). Two skip-on runs (nok02_tri run 1 and run 8 repeat) differ by 534 px, only at the figure x 25 to 55, y 89 to 116 (run-to-run noise). So the top-right regions are a real skip effect.
- Run 7 nok02_noboxint vs nok02_nobox in the top-right region: 6 px differ. vs nok02_tri: 1086 px. The interpreter draws what the no-skip path draws there.
- Run 5 and run 8 dlccheck, 300 frames: grp 25502, skip 12452, missing 0, xgrp 0. Every skipped triangle has a common clip plane with the same matrix (clip_bits4), and no skipped group uses a cross-group slot.
- Contradiction, not solved: the per-vertex plane check says the skipped triangles are outside, but the interpreter draws pixels for them in the top-right region. Causes not yet tested: (a) the interpreter keeps triangles that share an out bit (check draw_tri_cpu early-clip for bit 0x10, not in the 0x2f mask of verify), (b) the skipped group changes state for a later group (vert_live, geom), (c) the picture is from a triangle whose vertices come from the group box but not the verify walk.
- Flicker frame (camera move, end piece) NOT reproduced. Frame 200 at pos 250 0 11 shows the path in both builds.

## Next steps
1. Count, in noboxint with tritrace=200:1, the draw_tri_cpu triangles whose clip bits share a plane (they must not be kept), and check whether they produce the top-right pixels. Log the early-clip decision for them.
2. If (a): fix the early clip (pm_gbi.c) so out-plane triangles are rejected; the box skip then matches the interpreter.
3. If (b): compare the vert_live and geom state before and after a skipped group.
4. Do not change the skip rule until step 1 names the triangle.
PMDL bare word: debug_scan needs the word alone, so nobox=1, noboxint=1, dlccheck=1 and skipdump=1 never matched. The switches are now bare words in the scenarios (nobox, noboxint, dlccheck, skipdump).

## Runs 9 to 17 (coordinator request: find the skipped group whose pixels differ)
- Starts 4, 5, 7 and 8 used broken switches. PMDL needs a bare word; `nobox=1` never matched. Runs 4 and 7 equal skip-on runs plus noise. The 2855 px and 1086 px numbers are not a skip effect. Fixed in the scenarios (bare words).
- dlccheck with a working check (start 14, 300 frames, pos 250 0 11): skip 12537 of grp 25587, missing 0, xgrp 0. Every skipped triangle has a common out plane of clip_bits4 with the same matrix.
- skipdump at frames 200 and 204 (start 15): 48 of 99 groups skipped. Screen box with the pm_gbi viewport (vp_sx = vp_tx = 160, so the plane edge is at sx 4 and sx 0 to 4 margin, matching the box). No skipped group overlaps the viewport except at its edge (sx -47 to 4, -41 to -1), and those edge values are the viewport offset.
- Region pixels, top-right (x 137 to 173, y 52 to 129), 3 runs: skip on (run 1 and run 8) 0 px differ; skip off (start 16) 806 px; out groups through the interpreter (start 17) 1238 px vs skip off and 1036 px vs skip on. Neither route equals skip on. Route choice does not explain the difference.
- Conclusion: the skip hides only triangles outside one clip plane (proven by missing 0). Running them changes shading in the region. The skip is not the cause of a visible loss. The cause is state or shading of out-of-frustum triangles. Not tested: state after a skipped group (vert_live, geom, tri_since_vtx), and the lighting or depth write of out triangles.
- No skip rule change. A conservative box test already holds. Next: test the state difference (the same window, skip on, but run only the state side of each skipped group: G_VTX ring loads and G_GEOMETRYMODE, no triangles, switch noboxstate).

## State test (coordinator request, 2026-11-03): INCOMPLETE, FLK1 budget at cap
- Added debug `stdump` (pm_dlc.inc.c dlc_seg): one line `stseg f<frame> n gmi geom epoch pep tss boost fxon rgb tsl` per visible static segment, frames 200 to 205. pm_gpu_rgb_scale_get() added in pm_gpu.c (getter only).
- Scenarios nok02_stdump_on (skip on) and nok02_stdump_off (`stdump nobox`, skip off). Run them with `azt.ps1 <name> -Row FLK1 -Build flk1 -Try`, then diff the stseg lines of the two logs (build3ds/lane-flk1/last_log.txt, copy after each run).
- Not run: the FLK1 budget reported no starts left when start 18 was tried. The orchestrator must reset it (run_azahar.ps1 -ResetBudget FLK1) before these runs.
- Candidate words to check first, in the stseg diff: rgb (rgb_scale set only when ep_pepoch changes, pm_gpu.c:2436), boost, epoch, tsl. The skip path never runs dlc_seg for an out group, so ep_pepoch and rgb_scale stay from the previous visible group.
- Then the fix: keep the state of an out group in the skip path (call the epoch and rgb update for it, or drop the cache for the next visible group), and prove with nok02_nobox against nok02_tri (skip-on repeat noise 0 px in the top-right region).

## State diff result (starts 18 and 19, frames 200 to 205, pos 250 0 11)
- skip on: 102 stseg lines. skip off: 198 lines.
- Per line, rgb (1), boost (0), fxon (0), tsl (1) are constant in both runs. tss, epoch and pep differ only as counters. So the state carried between visible segments does not differ. The rgb_scale cache hypothesis is refuted.
- geom differs by segment content: skip off has 6 extra segments with geom 0x00000405 (lit bit 0x200000 clear) and more 0x00000005 and 0x00200405 segments. These are the out groups that skip on does not draw. So skip off draws geometry that skip on omits.
- dlccheck (earlier, valid): every skipped triangle lies outside one clip plane for all its vertices (missing 0).
- Working mechanism (inferred, not yet tested): an out group can have vertices with w <= 0 (behind the eye). The PICA path gets such triangles without a near clip, so they can rasterize mirrored on screen. Skip on removes them. The top-right difference is the shading of those triangles, which depth test partly hides.
- No fix made. The skip is the correct side for these groups.
- Proposed next test (needs approval): route a visible group with any box corner w <= 0 to the interpreter (no static GPU segment), frame 200 at pos 250 0 11, and diff against skip on. A change in the top-right region means visible behind-eye groups (not skipped) draw wrong on the GPU path. That path is the candidate flicker source for the end piece as the camera moves.
