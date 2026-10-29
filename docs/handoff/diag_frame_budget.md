# Yard frame budget, O3DS build 1227-dc_dc2

Source: build3ds/hw_pull/hw_o3ds_log.txt, steady frames f1172-1183 (12 frames). Values are ms per frame, median. [M] measured, [W] window average f1157-1183 (27 frames), [I] inference.

## Budget (wall median 58.0 ms = 100 %)

| bucket | ms | % wall | kind | src |
|---|---|---|---|---|
| Game logic (PH_LOGIC) | 5.8 | 10.0 | CPU, reducible | W |
| Display-list driver (PH_DL) | 3.2 | 5.5 | CPU, reducible | W |
| Walk, main task (PH_GBI) | 34.9 | 60.2 | CPU, reducible | M taskpl main |
| GPU submit (PH_GPU) | 3.0 | 5.2 | CPU, reducible | W |
| Texture cache (PH_GCACHE) | 3.0 | 5.2 | CPU, reducible | W |
| Draw submit (PH_GDRAW) | 2.4 | 4.1 | CPU, reducible | W |
| PH_TEX | 0.1 | 0.2 | CPU | W |
| Audio (PH_AUDIO) | 1.1 | 1.9 | CPU | I (W p95) |
| Frame begin (PH_FBEGIN) | 1.4 | 2.5 | GPU wait | M fb |
| Frame end (PH_FEND) | 1.8 | 3.1 | CPU, may hold a wait | M fe |
| Idle, vsync (PH_IDLE) | 0.0 | 0.0 | vsync wait | M idle 8.5 us |
| Other (residual) | 1.3 | 2.2 | CPU | I |
| **Sum** | **58.0** | **100** | | M wall |

Rows below are inside the walk (not added again):

| walk part | ms | % wall | kind | src |
|---|---|---|---|---|
| - tex streak (gbisplit tex) | 14.9 | 25.7 | CPU | M |
| -- flush closes at tex ops (flsplit tex) | 7.2 | 12.5 | CPU | M |
| --- non-empty closes (67) | 6.5 | 11.2 | CPU | I (97 us each) |
| --- empty closes (267) | 0.7 | 1.2 | CPU | I (2.6 us each) |
| -- texture hit lookup (thsplit) | 1.0 | 1.8 | CPU | M, nesting I |
| -- tex ops own work | 6.7 | 11.6 | CPU | I remainder |
| - gv streak (gbisplit gv) | 1.5 | 2.6 | CPU | M |
| - sync, combiner (gbisplit sync) | 1.3 | 2.3 | CPU | M, may overlap |
| - uncounted walk remainder | 18.5 | 31.9 | CPU | I |
| - kick_check (kick us) | 0.01 | 0.0 | CPU | M 10 us |

GPU own time, not in the sum: gdraw 8.2 ms (C3D_GetDrawingTime) and gproc 49.7 ms (C3D_GetProcessingTime), both of frame f-1 [M]. gproc is a span, not proven busy time [I].

## Bound type

- CPU-bound. Wall 58.0 ms, idle 0.0 ms, FBEGIN wait 1.4 ms.
- Wall 58.0 ms gives 17.2 fps. The 69.3 ms in console_numbers.md is the mean of 2179 yard frames and gives 14.4 fps. The 69.3 ms includes stall frames. Steady frames are faster.
- Pacing target 33.4 ms (2 vblanks). The wait in pm_main.c (PC01) runs only when the next step is still in the future. A 58.0 ms frame is past that time, so no wait runs and steps slip.

## Top 3 reducible buckets (leaf rows, no overlap)

1. Walk uncounted remainder: 18.5 ms. No counter splits it. Needs per-op-class timing.
2. Flush closes at tex ops: 7.2 ms (6.5 non-empty, 0.7 empty).
3. Tex ops own work: 6.7 ms [I].
Game logic is 5.8 ms, fourth.

## Caveats

- PH_LOGIC, DL, GPU, GCACHE, GDRAW, TEX are window averages. Their p95 is within 0.6 ms of the average. Audio and other are outlier-driven, so they are estimates.
- Background walk (taskpl bg) is 6.6 ms per frame [M]. It is not a row. Its nesting is not resolved from the code, so it may be inside PH_LOGIC or PH_DL. Both are below 6.6 ms, so the sum does not confirm it.
- Flush closes (7.2 ms) are assumed inside the walk, because PH_GBI window (34.7) equals main walk (34.9). PH_GPU (3.0) is not the close total.
- dcsplit stage sums (pal 0.79, bind 0.59, tev 0.46, st 0.67, draw 1.93, kick 0.10 ms, sampled x8) total 4.5 ms. About 2.0 ms of the closes is not split.
