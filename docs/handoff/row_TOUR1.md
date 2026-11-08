# Row TOUR1: map tour state machine with prof windows synced to each warp

## Goal
One game session visits a map list. Each warp: fixed seed, settle, one prof window of `tourtime` s, close before the next warp. Plan: docs/handoff/plan_tours.md (read it first; section Design, row 1).

## Start state
Worktree build3ds/wt/tour1, branch tour1 from nds1 7bbd2dfee+. Copy assets/us and ver/us/build into it. PERF1 runs in parallel and edits pm_phase.c (prof hooks, core-1 probe): keep your pm_phase.c change to the window API and the tour-mode gate, so the merge stays small.

## Premise and its check (oracle plans)
1. full411 reaches `tour: done` in one boot: `python 3ds/tools/bench.py show full411` and its last run in the history (no new start). Find where tour_s420.txt lives.
2. The load settles a fixed number of frames after the first world frame: in the last full411/tour10 log, the gap between the map-in line and the last texmiss/ROM read for 5 maps. Set SETTLE from it.
3. A window closed early is valid: pf_report (pm_phase.c:419-457) divides by pf_w/pf_s.
4. A fixed gRandSeed at WARP gives equal rng: grep rand_int callers in map init; compare the det step rng values of two tour10_det runs.
If 1 or 3 fails, hand back at once.

## Touch only
3ds/src/pm_warp.c (tour engine, tourlist=all from gAreas, tourtime=, seed), 3ds/src/pm_phase.c (pm_ph_window_open/close, tour mode stops the 300-frame report and the scene drop), azt_scenarios.json / bench_defs.json (tour10_det until-line if it changes).

## Done criteria
1. Build; nd_host PASS.
2. tour10_det reaches `tour: done items 10`, one `prof ... tour <i>/<n> <map>:<entry>` line per map, no window across two maps.
3. spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015 byte-equal to the base (tour code is off there).
4. A warp with a position (map:entry@x:y:z) snaps y with a downward collision ray from y+200 (the game raycast), so Mario stands on the surface: log `tour: at <map> y <snapped>`; one shot of 2 tour10 maps shows Mario on the floor. POS1 writes the spot list in parallel (3ds/tours/spots.txt); tourlist=all reads it once it lands, entry 0 until then.
5. TOUR_FRAMES and the old fixed-frame tour path deleted.
Affected scenarios (no others): tour10, tour10_det, the 4 byte gates.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=tour1 PM_ROW=tour1 on every build.sh and bench call. No git stash. No toggles, no stubs. Commit after each passed step.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=tour1 PM_LANE=tour1 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=tour1 PM_LANE=tour1 python 3ds/tools/bench.py run <bench> --row tour1 --lane tour1 --why "<text>"
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget tour1

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "tour1 ready <hash>".
