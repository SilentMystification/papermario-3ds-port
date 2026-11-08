# Row TOUR2: tour windows into the perf DB, tour-report, full tour benches

## Goal
bench.py reads the tour lines (TOUR1) into the perf DB, one window record per map, and prints a per-map report. Add the full tour scenario and benches. Plan: docs/handoff/plan_tours.md (Design, Perf DB; row 3).

## Start state
Worktree build3ds/wt/tour2, branch tour2 from nds1 d48d62cf2+. Copy assets/us and ver/us/build into it. No game source change.

## Premise and its check
Premise: TOUR1 prints one `prof f<n> w<n> tour <i>/<n> <map>:<entry>` window per map and `tour: done items <n>`. Check first: the tour10_det log from the TOUR1 runs (bench history, runs.jsonl, log_path); ingest it with the current bench.py and see the scene records.

## Touch only
3ds/tools/bench.py (ingest: tour_idx and kind map|battle in extra_json, run tags tour_full/tour_curated/tour_battle; new `tour-report <run_id>`: one row per map with ft_ms, wk, o34 share, gbi, draws, tris, ranked by ft_ms then o34), 3ds/tools/prof_by_scene.py if ingest needs it, azt_scenarios.json (scenario tour_full: `warp=arn_02:0 story=ch0_toad_town tourlist=all prof=4`, cap with a stated reason: soak, about 411 x (2 s settle + tourtime)), bench_defs.json (benches tour_full and tour_full_prof for the console, o3ds, `prof=4 tourlist=all`). Scenario and bench in the same commit.

## Done criteria
1. Ingest of the TOUR1 tour10_det log gives 10 window records with tour_idx 1..10; tour-report prints 10 rows.
2. `bench.py show tour_full` and `bench.py show tour_full_prof` work. Do NOT run tour_full (the full tour runs only when the user asks).
3. bench.py host tests (if any) pass.
Affected scenarios (no others): none run except a re-ingest of an existing log.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=tour2 PM_ROW=tour2. No git stash. No stubs. Commit after each passed step.

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "tour2 ready <hash>".
