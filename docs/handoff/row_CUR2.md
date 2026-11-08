# Row CUR2: curated tour covers every area; full tour covers every map

## User facts (2026-11-08)
"the map warp ... partial test we seem to be implementing only visits some locations, I want it to warp to every area in the small set, not every map, just one of the maps in all of the different areas. The full full test should walk EVERY map."
Now: 3ds/tours/curated.txt holds 8 maps from the Azahar tour10_det seed (kmr, mac, dgb, hos, trd only). spots.txt covers 28 areas (arn dgb dro end flo gv hos isk iwa jan kgr kkj kmr kpa kzn mac machi mgm mim nok obk omo osr pra sam sbk tik trd; area = map prefix before `_`, machi is its own).

## Goal
1. CURATED = at least one map from EVERY area (28 today), plus the laggy maps from a tour run. tour-curate (3ds/tools/bench.py) always emits one map per area: with tour data, the slowest map of that area (rank ft_ms then o34, as now); with no tour data for an area, the map with the highest spot score in 3ds/tours/spots_report.txt (most content on screen). Then the laggy maps (--slow-ms, --top) not already in. --per-area stays (default 1 now). Every line carries its spot (map:entry@x:y:z) from the compiled spot table, as now.
2. Regenerate curated.txt now (seed from spot scores plus the tour10 data; header says so) and pm_tour_lists.h (gen_tour_lists.py).
3. FULL = tourlist=all: check it lists every non-tst map of gAreas (412 incl. kkj_26; tour_s420 had 411) and that tour_full uses it. Report the count printed by the game (`tour: list all <n>` or the done line) from a code read, no full run.
4. tour_curated scenario/bench caps follow the new length (about 28-40 maps x (2 s settle + tourtime)); state the reason in the scenario.

## Start state
Worktree build3ds/wt/cur2, branch cur2 from nds1 (current). Copy assets/us and ver/us/build into it.

## Touch only
3ds/tools/bench.py (tour-curate), 3ds/tools/gen_tour_lists.py, 3ds/tours/curated.txt, 3ds/src/pm_tour_lists.h (generated), azt_scenarios.json / bench_defs.json (tour_curated caps), docs/handoff/plan_tours.md (curated rule text).

## Done criteria
1. curated.txt has every area at least once (a check in tour-curate prints the area count and fails if one is missing).
2. Build; nd_host PASS.
3. tour_curated (Azahar) reaches `tour: done items <n>` with n = the list length, one window per map. This is the only tour run; do not run tour_full.
Affected scenarios (no others): tour_curated.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=cur2 PM_ROW=cur2 on every build.sh and bench call. No git stash. No stubs.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=cur2 PM_LANE=cur2 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=cur2 PM_LANE=cur2 python 3ds/tools/bench.py run tour_curated --row cur2 --lane cur2 --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "cur2 ready <hash>".
