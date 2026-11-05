# Trace: water tower flicker in sbk_99 (-10 25 -147), 2026-11-03

Branch native, build lane-native (pm_3ds.3dsx, 22:11). Row SBK99. Azahar starts used: 3 of 4 (budget 8 per row, 3 for a diagnostic row). Status: status_SBK99.txt (3 lines, 3ds/logs/perf).
Result: the water tower is NOT in any shot. The triangle trace is NOT done. Cause is INFERRED from code only.

## Runs (warp sbk_99:0, story ch0_toad_town, pos=-10:25:-147)
| Run | pos | shots | census (frame) | drawlog |
|---|---|---|---|---|
| 1 | yaw 90 | frames 200, 204 | f200: 6 batches, 14 tris, 3 keys | no lines |
| 2 | yaw 270 | frames 200, 204 | f200: 6 batches, 14 tris, 3 keys | no lines |
| 3 | yaw 90 | frames 300, 304 | f300: 6 batches, 14 tris, 3 keys | no lines |

- Shots of runs 1 and 2 are byte-equal (PNG diff bbox None). Both show sand dune and blue sky. No tower. The view does not change with yaw.
- Shot 200 equals shot 204 in each run. No motion is visible.
- The log shows `poswarp: placed Mario at -10.000 25.000 -147.000 on map sbk_99`, settled at the same point, in all runs.
- Log line `dlc2 f300 tri 10740` (run 1) counts the static group, not the census. The census sees 14 triangles per frame. The census does not count the map geometry.
- drawlog=200:1 and drawlog=300:1 print no `dd f` line. The map chunks do not pass the G_VTX path (pm_gbi.c:2862) in these frames. The map draws another way (static groups, log `static grp 728 tri 10740`).
- No walk input was used. Mario moved by pos= only (2 units test not run).

## What the trace could not show
- Which triangle drops in or out per frame.
- The value of w, the cull sign or clip_bits per triangle of the tower.
- The near-plane state of the tower.

## Code facts (read, not run)
- Cull test, late path: pm_gbi.c:1706-1720. `cross = (ex1*ey2 - ey1*ex2) * (p1->w * p3->w)` at 1709. Flip at 1712: `if ((p1->w < 0) ^ (p2->w < 0) ^ (p3->w < 0)) cross = -cross;`.
- Cull test, early path (no tex1 plan): pm_gbi.c:1648-1658. Same formula and same flip.
- Math: with X = x/w, ex1*ey2 - ey1*ex2 = w1 w2^2 w3 * D (D = NDC signed area). Times w1 w3 gives cross = (w1 w2 w3)^2 * D. The square is never negative, so sign(cross) = sign(D). The odd-negative-w flip is the only w sign term left, so it inverts the result for each triangle with an odd count of negative w.
- Near plane: clip_near is absent. clip_bits (pm_gbi.c:881-890) has no near test (bit 32 is z > w only). Triangles behind the eye with w < 0 are drawn.
- A vertex that crosses w = 0 changes the count of negative w, so the flip toggles for every triangle that uses that vertex. This is a flip on the whole tower when the camera moves.

## Cause (INFERRED)
pm_gbi.c:1712 (and the early copy at 1656 area) inverts the cull result when an odd number of vertices have w < 0. The math above shows the w product adds no sign. The flip is therefore a sign error for any triangle with one or three vertices behind the eye. Near the eye (w near 0), a vertex crossing w = 0 toggles the flip, and the triangle appears or vanishes frame to frame.
Not PROVEN: no triangle of the tower was logged. No value of w was read.

## Same cause as nok_02 path flicker?
INFERRED yes. The same flip (both copies) and the same missing near clip. The nok_02 end piece (investigate_render_triage.md item 1) matches the same sign rule. The tower and nok_02 share the code path. Both need the same trace.

## Fix proposal (not applied)
1. One helper for the cull sign (both copies, 1656 and 1712). Drop the odd-negative-w flip only after checking that the draw path never keeps w < 0 with the near clip off. If w < 0 is kept, the test must use the screen area sign with the w sign as the true orientation, checked on a known one-sided wall (Mario house yard, V02 note at pm_gbi.c:1707).
2. Test with nocull=1 against default. If the tower stays stable with nocull, the cull is the cause.

## Next steps (not done)
- A trace switch in draw_tri_cpu that logs, per triangle with any w < 0 or any clip bit, the w values, the cross sign and the decision. Needs a code agent (not started).
- Find the camera where the tower is in view. The 3D map is not in the census. Use the static group path (log `static grp`) to find the draw list.
- The 4th start is unused. Do not spend it on a yaw guess. Run it after the trace switch exists.
