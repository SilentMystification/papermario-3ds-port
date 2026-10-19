# Post-S28 optimization order (PO2, PO3, AU1, MA1)

Written 2026-10-19. Read-only plan. Sources: docs/plans/AU1_row.md (full AU1 plan), PO3.md, MA1_list.md, 3ds/SLICES.md.
Target: the O3DS at full speed (30 steps per second, 33.3 ms per step). Core 0 = logic plus the CPU part of the RSP work. Core 1 = audio. PICA = rasterization plus vertex work.
Mark [USER] = needs the user. Mark [O3DS] = needs the O3DS on. Every Azahar run sets PM_ROW=<row>. Golden: 1 to 3 named scenes.
Baseline: the S28 O3DS numbers (phase split, `audio cost`, `pacestat`). HANDOFF.md has no S28 numbers yet. If they are not recorded, get them first [USER] [O3DS].

## Order (most speed first)

| # | Row | Expected gain on core 0 | Why this place |
|---|---|---|---|
| 1 | AU1 | 6 to 8 ms per step (EST from Azahar 3 to 4 ms per tick) | Largest single cost. Bit exact. Small code. Separate files. |
| 2 | PO2 | part of GBI interpret 11 to 15 ms (corner work done about 1.8 times per vertex) | CPU only. Needed by PO3. |
| 3 | PO3 | most of the rest of the triangle work (79 % of triangles are rejected after a full CPU transform) | Large. Needs PO2. Only if the O3DS still shows a large GBI share after PO2. |
| 4 | MA1 | small per site, unknown sum | Many small edits. Logic edits need a replay check and approval. |

## 1. AU1: audio on core 1
- Change: slices AU1a to AU1d. Add `audiohash` and `audiolock` switches, move the tick to the kick point, replace the yield with `pm_audio_kick` and `pm_audio_join` (LightEvent), create the audio thread on core 1, join at `__wrap_step_game_loop` and before `pm_warp_tick` / `pm_map_tick`.
- Files: 3ds/src/pm_audio.c, pm_os.c, pm_main.c, pm_phase.c (join point), pm_log.c (time limit constant).
- Azahar test: audio hash equal to B1 in 3 runs (toad, yard, battle). Golden: toad, battle. Smoke yard and toad. `crashtest_audio` trace names core 1. `warp=nok_01:0` gives the same fault as before.
- Metric: `audio cost avg/max us per tick`, `audio wait` us (core 0 join wait), job wall time, underruns, steps per second.
- AU1e [USER] [O3DS]: `push-o`, record the metrics at limit 30, then 50 and 80 if `audio wait` > 0. User ear check [USER].
- AU1f (ARMv6 SIMD in pm_rsp_audio.c) only if core 1 is over budget at limit 80. Hash must stay equal to B1.

## 2. PO2: transform each vertex once at G_VTX
- Change: at G_VTX, transform position, colour and UV once per vertex into a per-slot cache with a matrix id. `draw_tri` reads the cache. No 64 B matrix copy per vertex.
- Files: 3ds/src/pm_gbi.c (G_VTX case, `draw_tri`, `xform_vtx`, `vert_color`, `vert_uv`), pm_phase.c (timer).
- [USER] first: the PO2 user question is not in a questions file yet (HISTORY.md line 1548). Ask before code.
- Azahar test: golden toad, yard, battle. Must be 0 differences (same math, same order). Smoke yard and toad.
- Metric: phase 3 (GBI interpret) ms per step, corners transformed per frame (`stats`). Then O3DS phase 3 [USER] [O3DS].

## 3. PO3: GPU vertex transform
- Change: slices PO3a to PO3g in PO3.md. Raw Vtx ring, u16 index ring, matrix and UV and colour uniforms, `gpuvtx` switch (default off until PO3g), GPU cull and clip. Add `--extra` to golden.py.
- Files: 3ds/src/pm_gbi.c, pm_gpu.c, pm_gpu.h, 3ds/shaders (new program), CMakeLists.txt, 3ds/tools/golden.py.
- Gate: PO3a counters (matrix ids per frame, mixed-id triangles) are go or no-go. PO3b waits for PO2 and the O3DS phase 3 number after PO2 [O3DS].
- Azahar test: golden switch off = 0 differences, `--extra gpuvtx` within TOL. Scenes per slice: yard, pause1, battle (PO3e also kmr_20 near wall). Check the render_gaps list (X02, F03, water_splash grabs) at every slice.
- Metric: phase 3 and 4c, draws per frame, FrameBegin wait, command buffer %.
- PO3f [USER] [O3DS]: switch off and on, `phase phasecsv`, `pull-o`, `phase_report.py`. Picture check on hardware (Azahar clipper can differ).

## 4. MA1: ARM11 math
- Step 0: add a state-hash switch (hash of game state per N frames). No replay tool exists today. Record baseline hashes in 2 runs (toad, yard, battle).
- Render and port sites first (no logic change): P13 to P15 in pm_tex.c, P16 and P17 in pm_gu.c (check which guRotateRPY links first), P18 in pm_os.c, P1 to P12 in pm_gbi.c and P7 in pm_gpu.c.
- Logic sites (D8 to D12, D16, and P16 if it moves NPCs): only with equal state hashes and [USER] approval per change.
- Azahar test: state hash equal, golden toad and yard (render sites), audio hash equal for P20.
- Metric: phase ms per step for the touched phase, then O3DS [USER] [O3DS]. Measure divide cost on the O3DS before a rewrite.

## Parallel and conflict
| Pair | Status | Shared file |
|---|---|---|
| AU1 and PO2 | parallel (lane B for the second agent) | pm_phase.c only. AU1 edits `__wrap_step_game_loop`. PO2 edits the timer. Merge the second one after the first. |
| AU1 and PO3 | parallel | none |
| PO2 and PO3 | CONFLICT, sequential | pm_gbi.c (`draw_tri`, G_VTX). PO3a counters only can run before PO2. |
| MA1 pm_gbi.c / pm_gpu.c sites and PO2, PO3 | CONFLICT | pm_gbi.c, pm_gpu.c. Do these MA1 sites after PO3g (many leave the CPU path). |
| MA1 pm_tex.c, pm_gu.c sites and AU1, PO2, PO3 | parallel | none |
| MA1 P18 (pm_os.c), P20 (pm_audio.c) and AU1 | CONFLICT | pm_os.c, pm_audio.c. Do after AU1d. |
- Code may run in parallel. Azahar tests stay serial: one Azahar user at a time. Budget 8 Azahar starts per row.
- Every [O3DS] step needs the user. Batch them: one hardware session after AU1d plus PO2, one after PO3g plus MA1.
