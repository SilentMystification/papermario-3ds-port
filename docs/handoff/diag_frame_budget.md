# Yard frame budget, O3DS build 1227-dc_dc2

Source: build3ds/hw_pull/hw_o3ds_log.txt, steady frames f1172-1183. Ms per frame, median. [M] measured, [W] window avg f1157-1183, [I] inference.

| bucket | ms | % wall | kind | src |
|---|---|---|---|---|
| Game logic (PH_LOGIC) | 5.8 | 10.0 | CPU, reducible | W |
| Display-list driver (PH_DL) | 3.2 | 5.5 | CPU, reducible | W |
| Walk, main task (PH_GBI) | 34.9 | 60.2 | CPU, reducible | M |
| GPU submit (PH_GPU) | 3.0 | 5.2 | CPU, reducible | W |
| Texture cache (PH_GCACHE) | 3.0 | 5.2 | CPU, reducible | W |
| Draw submit (PH_GDRAW) | 2.4 | 4.1 | CPU, reducible | W |
| PH_TEX | 0.1 | 0.2 | CPU | W |
| Audio (PH_AUDIO) | 1.1 | 1.9 | CPU | I |
| Frame begin (PH_FBEGIN) | 1.4 | 2.5 | GPU wait | M |
| Frame end (PH_FEND) | 1.8 | 3.1 | CPU | M |
| Idle, vsync (PH_IDLE) | 0.0 | 0.0 | vsync wait | M |
| Other (residual) | 1.3 | 2.2 | CPU | I |
| **Sum = wall** | **58.0** | **100** | | M |

Inside PH_GBI (34.9, not added): tex streak 14.9 (25.7 %), of which flush closes 7.2 (12.5 %; 6.5 non-empty, 0.7 empty [I]), hit lookup 1.0, own work 6.7 [I]. gv streak 1.5 (2.6 %). sync 1.3 (2.3 %, may overlap). Uncounted 18.5 (31.9 %) [I]. kick_check 0.01.

Not in sum: GPU gdraw 8.2 ms, gproc 49.7 ms (frame f-1). gproc is a span, not proven busy time [I].

## Bound type

- CPU-bound. Vsync wait 0.0 ms. GPU wait 1.4 ms.
- 58.0 ms gives 17.2 fps. 69.3 ms (console_numbers.md, mean of 2179 frames) gives 14.4 fps. The mean includes stall frames.
- Step target 33.4 ms. The pacing wait runs only when the step is still in the future. At 58.0 ms no wait runs.

## Top 3 reducible (leaf rows)

1. Walk uncounted remainder: 18.5 ms. No counter splits it.
2. Flush closes at tex ops: 7.2 ms.
3. Tex ops own work: 6.7 ms [I].
Game logic is 5.8 ms (fourth).

## Caveats

- W rows are window averages. Audio and other are outlier-driven (602 ms frame f1183).
- Background walk (taskpl bg 6.6 ms) has no row. Its nesting is not resolved.
- Flush closes are assumed inside PH_GBI (34.7 window vs 34.9 main walk).
