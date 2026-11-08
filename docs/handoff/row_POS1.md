# Row POS1: tour spot per map (central, busy, outdoors, on the surface)

## Goal
For every map of the tour, one Mario position where the most content is on screen, near the map centre, not inside a building or under a roof, on a floor surface. Output 3ds/tours/spots.txt (map:entry@x:y:z), read by the tour (TOUR1). Plan: docs/handoff/plan_tours.md (Design, Spots).

## Start state
Worktree build3ds/wt/pos1, branch pos1 from nds1 77356d4b3+. Copy assets/us and ver/us/build into it. TOUR1 runs in parallel in pm_warp.c: do not edit pm_warp.c or pm_phase.c.

## Premise and its check
Premise: the extracted assets hold, per map, the collision (floor triangles, with their surface type) and the shape (model bounding boxes) in a form a host Python tool can read. Check first: find the map collision and shape files (assets/us, ver/us/build, src/world/area_*), and parse 3 maps (mac_01, kmr_20, iwa_10). If the data is not readable on the host, hand back with what exists.

## Method
Candidate = floor triangle centroid (floors only, no walls/ceilings). Reject: an upward ray hits collision or model geometry within roof height (inside or under a roof); not connected to the floor of entry 0. Score = model and entity boxes inside the default camera view box around the point, minus a weight times the distance to the floor centre. Best candidate wins. y = floor height at the point (TOUR1 also snaps y at run time). Maps with no floor or a single room: entry 0 spot, marked.

## Touch only
New: 3ds/tools/tour_spots.py, 3ds/tours/spots.txt. A host test 3ds/tools/tour_spots_test.py (3 maps: point on a floor triangle, no roof above).

## Done criteria
1. tour_spots.py writes spots.txt for every map of tour_s420.txt (no tst_ maps); a line per map with its score and a reason for entry-0 fallbacks.
2. Host test PASS.
3. Picture gate (after TOUR1 lands, or with the existing pos= switch now): one Azahar start, 3 maps (mac_01, kmr_20, one forest or desert map) warped to their spots; each shot shows Mario on the ground, outdoors, with the busy part of the map on screen.
Affected scenarios (no others): one new scenario tour_spots3 (azt_scenarios.json and bench_defs.json, same commit).

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=pos1 PM_ROW=pos1 on every build.sh and bench call. No git stash. No stubs.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=pos1 PM_LANE=pos1 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=pos1 PM_LANE=pos1 python 3ds/tools/bench.py run <bench> --row pos1 --lane pos1 --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "pos1 ready <hash>".
