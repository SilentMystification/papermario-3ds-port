# diag_ec1: empty batch close (row ec1)

## Result
- No draw-path edit. An empty close already costs about 1 us. The task premise (empty closes cost the walk) does not hold on Azahar.
- Named blocker: the empty close has no state work left to skip. The skip would save at most 0.25 ms per drawn frame on Azahar (phase timers included). The cost sits in the 76 non-empty tex closes per frame (26 us each) and in tex ops outside the flush.
- Added the `fleus` line (empty-close us per class, phase on only). Count-only.

## What an empty close does (code read)
- `pm_gpu_flush_at` (pm_gpu.c) sets `fl_fn`, calls `flush_impl`, and with phase on reads `pm_ticks()` twice (svcGetSystemTick).
- `flush_impl` checks `rect_open`. With no rectangle batch it calls `flush_draw`.
- `flush_draw` (pm_gpu.c ~line 709): `gni`, `fl_count` (only with switch gbiprof), then `if ((nverts <= 0 && !graw) || !gpu_ok)` sets `nverts = 0`, `gni = 0`, `gi_start = gidx_n`, `batch_identity = 0` and returns. The state commit (uniforms, TEV, depth, blend, texture bind, draw) sits after this return and does not run.
- So an empty close does no GPU command, no bind, no TEV rebuild. The only remaining work is the two timer reads and the counters (phase on only).

## Numbers, yard, Azahar, build with ec1 split (PM_ROW=ec1, PM_LANE=ec1, 1 start for the timing run, 1 start for the picture run)
Run: `fast preset=yard phase shots shotmode=4:80 battlef=80`. Log: build3ds/lane-ec1/last_log.txt. Averages over 10849 drawn frames (flsplit lines, `python` averaging over the log). Us are per drawn frame.

| class | closes per frame | empty closes per frame | us per frame, all closes | us per frame, empty closes |
|---|---|---|---|---|
| tex | 240.8 | 165.4 | 2160 | 168.5 |
| cc | 6.0 | 6.0 | 6 | 6.1 |
| rect | 60.1 | 54.3 | 196 | 55.3 |
| tri | 3.0 | 3.0 | 3 | 3.1 |
| oth | 24.9 | 24.9 | 25 | 25.4 |
| all | 334.8 | 253.6 (76 %) | 2390 | 253 |

- Tex empty close: about 1.0 us each. Tex non-empty close (76 per frame): about 1992 us in total, about 26 us each.
- gbisplit per frame: sync 744 us, gv 194 us, tex 4854 us. The tex 4854 us includes the tex ops outside the flush (for example G_SETTILE wrap change, pm_tex_set_wrap, rect sync). The tex flush part is 2160 us.
- Earlier console data (docs/handoff/console_numbers.md) put 207 of 281 tex closes empty. Azahar gives 165 of 241. The empty share matches, the cost per empty close does not show in the Azahar numbers.

## Before and after
- Before: the numbers above (build with ec1 split, draw path unchanged).
- After: no draw-path change, so no after run. The change of this row is the count only.

## Picture
- build3ds/lane-ec1/shots/0040.ppm (shotmode 4, frame 40 of the series, yard). Converted: build3ds/lane-ec1/shots/ec1_base_0040.png. Shows Mario with shadow, ground and the yard. Read by eye.

## Checks
- statehash toad and battle: not run. The draw path did not change, so the statehash bases stay valid by construction.
- Pause menu and file select: not run for the same reason.

## Next target (not done here)
- The 76 non-empty tex closes per frame (about 2.0 ms of the 2.16 ms tex flush). Each one draws the batch before a texture change. The cost per close is the draw itself. Row ec1 does not change that path.
- The tex ops outside the flush (2.7 ms per frame: 4854 minus 2160) are the next place to measure with a split of pm_tex_set_wrap and pm_gpu_rect_sync.
