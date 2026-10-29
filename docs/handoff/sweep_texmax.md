# Sweep texmax: VRAM cache 3072 KB (result: failed, size kept at 1536 KB)

## Result
- 3072 KB slab: `vram alloc fail slab 3072 KB (vram free 5394 KB, vramfree min 5394 KB)` at boot (pm_tex.c slab_init). The 6144 KB pool showed 5394 KB free, but no contiguous 3072 KB block was available.
- With no slab, texture failures climbed from 7876 (map 1) to 59569 (map 10) in batch 1. The batch is not a pass.
- Size reverted to 1536 KB (BUDGET unchanged in HEAD). Instrumentation committed: e7edbbbc3.
- Full walk (420 maps from 3ds/maps.cmake, 42 batches of 10) NOT run at 1536 KB: texmax budget row has 1 of 8 starts used; needs an orchestrator reset for 41 more starts.

## Batch 1 (maps 1-10, 3072 KB, PM_ROW=texmax, run 1/8)
| # | map | loaded | tex failures (cumulative) | vramfree min KB | notes |
|---|-----|--------|---------------------------|-----------------|-------|
| 1 | arn_02 | yes (tour: done) | 7876 | 5394 | slab failed |
| 2 | arn_03 | yes | 15618 | 5394 | |
| 3 | arn_04 | yes | 25446 | 5394 | |
| 4 | arn_05 | yes | 33174 | 5394 | |
| 5 | arn_07 | yes | 43133 | 5394 | |
| 6 | arn_08 | yes | 53033 | 5394 | |
| 7 | arn_09 | yes | 55262 | 5394 | |
| 8 | arn_10 | yes | 56818 | 5394 | |
| 9 | arn_11 | yes | 58072 | 5394 | |
| 10 | arn_12 | yes | 59569 | 5394 | |

Shots: build3ds/lane-texmax/shots/0000_14_0.ppm to 0010_14_9.ppm (not viewed). No CRASH line in the log.

## Not done
- Effect scenes (pause menu, file select rotation, battle start, flame map): not run.
- Verdict: 3072 KB fails at boot. Lowest vramfree min seen: 5394 KB (no slab).
