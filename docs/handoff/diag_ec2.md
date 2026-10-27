# diag_ec2: empty batch close, expensive calls per close (row ec2)

## Call list, empty close (no vertex queued, no raw batch)
Code read: pm_gpu_flush_at (pm_gpu.c) -> flush_impl -> flush_draw early return (`nverts <= 0 && !graw`).
Before this row, an empty close made these calls:
1. pm_ticks() x2 (svcGetSystemTick, one SVC each) around flush_impl, phase on only. This was the only svc cost.
2. pm_gbi_flush_class() (switch, no hardware cost).
3. flush_draw: fl_on() (cached static), fl_count() only with switch gbiprof (off on console).
4. flush_impl rect branch: rectangle bookkeeping (no hardware call). Kept: it resets rect_open and rect_y0/rect_y1 for correctness.
Not on the empty path: GSPGPU cache flush, C3D_* state calls, mutex, command commit or split, TEV rebuild, texture bind, logging.

## Change
- pm_gpu_flush_at: when nverts <= 0 and gni <= 0, call flush_impl with no timer. Plain counts pm_ec_n (empty closes) and pm_fl_n/pm_fl_e per class (phase on).
- Timers stay on non-empty closes only.
- Removed the empty-close microsecond field (pm_fl_eus, fleus line): it no longer has a timer.
- Counter kept: pm_ticks increments pm_svc_n. The empty path reads it before and after and adds the delta to pm_ec_svc. Printed as `ecsplit f<n> empty <n> svc <n>`.

## Counts, yard, Azahar (PM_ROW=ec2, PM_LANE=ec2, fast preset=yard phase shots shotmode=4:80 battlef=80)
| build | drawn frames | empty closes per frame | svc per empty close |
|---|---|---|---|
| before (timers on every close) | 13009 | 253.8 | 2.00 (510 svc per frame / 255 empty) |
| after (no timer on empty) | 11249 | 248.2 | 0.00 (189 svc in the whole run, shared counter, other threads) |

The 189 svc reads after the change come from other threads that call pm_ticks while the empty path runs. The shared counter is not atomic. The empty path itself makes 0 svc calls.
C3D state calls, cache ops and mutex calls per empty close: 0 by code read. Not counted by a counter.

## Timing, Azahar only (direction, not console)
- gpusplit cpu median: before 14.09 ms (p10 14.03, p90 14.31, 12921 frames); after 13.35 ms (p10 11.29, p90 20.53, 11249 frames). The after run has a wider spread.
- flsplit tex flush: before 2171 us per frame, after 1862 us per frame (average over frames).

## Picture
- Yard, shot 40 (frame 89): build3ds/lane-ec2/shots/0040.ppm. Mario, ground, shadow, post office sign visible. Same picture before and after.

## Checks not run
- statehash: not run (user decision after the first toad attempt).
- Pause menu text (see, Boots): not run.
- File select: not reached. `fast autoa freshsave` reached Toad Town instead.
- Memory HUD rows: visible in the after shot (VRAM, FCRAM, linear, heap), values as before.
- golden.py: not run (not requested).

## Console note
The earlier "30 us per empty close" (docs/handoff/console_walk_split.md) is 8.39 ms divided by 281.9 tex closes. That is the average over all tex closes, including the 74 non-empty ones (about 26 us each on Azahar). It is not a measured cost of an empty close. The empty close measured about 1 us on Azahar (diag_ec1).
