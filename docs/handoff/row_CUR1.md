# Row CUR1: curated tour list

## Goal
`bench.py tour-curate <run_id>` builds the curated map list from a full tour run; the list compiles into the game as `tourlist=curated`; benches tour_curated (Azahar) and tour_curated_prof (console). Plan: docs/handoff/plan_tours.md (Design, Curated; row 5).

## Start state
Worktree build3ds/wt/cur1, branch cur1 from nds1 (current). Copy assets/us and ver/us/build into it. TOUR1 (tour engine, tourlist=all, spots.txt read from the SD folder) and TOUR2 (ingest with tour_idx/kind, tour-report) are merged. POS1 (3ds/tours/spots.txt) runs in parallel: the curated list takes the spot of a map from spots.txt when the file exists, else entry 0.

## Premise and its check
Premise: tour-report data (one record per map window) is enough to rank maps. Check first: run tour-report on the tour10_det ingest (scratch PM_RUNS_FILE, as TOUR2 did).

## Touch only
3ds/tools/bench.py (tour-curate; fix tour-report o34 share = o34 / clean frames of the window, not o34 / ft_ms), new 3ds/tools/gen_tour_lists.py, new folder 3ds/tours/ (curated.txt with a header: source run id and build), generated 3ds/src/pm_tour_lists.h (CMake regenerates it as other generated headers; see 3ds/CMakeLists.txt), 3ds/src/pm_warp.c (tourlist=curated reads the compiled list only), azt_scenarios.json and bench_defs.json (tour_curated, tour_curated_prof; scenario and bench in one commit).
tour-curate rules: every map with clean ft over --slow-ms (default 33.3) up to --top (default 20), plus --per-area (default 2) maps per area (area = map prefix before `_`), the ones with the most draws. Until a full tour exists, seed curated.txt from tour10_det with a header that says so.

## Done criteria
1. Build; nd_host PASS.
2. tour-curate on the tour10_det ingest writes curated.txt; the build holds the list.
3. tour_curated (Azahar) reaches `tour: done items <n>` with n = the list length, one window per map.
4. spot_mac01_det byte-equal to the base (tour code off).
Affected scenarios (no others): tour_curated, spot_mac01_det.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=cur1 PM_ROW=cur1 on every build.sh and bench call. No git stash. No stubs. Do not run tour_full.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=cur1 PM_LANE=cur1 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=cur1 PM_LANE=cur1 python 3ds/tools/bench.py run <bench> --row cur1 --lane cur1 --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "cur1 ready <hash>".
