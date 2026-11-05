# Bench process (python 3ds/tools/bench.py)

One tool runs tests, records every result and answers queries. Do not read runs.jsonl or the old catalog by hand. Do not hand-build run_azahar.ps1 commands for a test a bench covers. Numbers live in the database, not in prose. The run history is append-only: every run stays.

## Process by situation

A. Check a change (a row)
1. `bench.py list`, then `bench.py show <bench>` for the benches the row file names (recipe, gates, last runs).
2. For each bench: `PM_ROW=<row> python 3ds/tools/bench.py run <bench> --row <row> --why "<quantity, code path, why no other run answers it>"`. The tool writes the why line, starts the test and records the run.
3. `bench.py check <run_id>`: the gates. PASS or FAIL per gate.
4. kind azahar_pic: also open the shot in build3ds/azt/shots/<scenario>/ and confirm the subject. A log pass is not a pass.
5. Budget: 8 Azahar starts per row. At the cap with gates unrun: reset (powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget <row>) and go on.

B. Before and after a change
1. Run the bench on the base build (record it), change the code, rebuild, run the bench again with the same speed (`--speed`).
2. `bench.py compare <bench> <buildA> <buildB>`, or `--last 2`. The tool prints the delta of each metric and flags failed gates. It prints WARNING and no delta when the speeds differ.
3. Console speed claims use console_prof benches (prof mode) only. Azahar numbers show direction.
4. An older commit: `bench.py build-at <commit> <bench> --row R` builds it in build3ds/at_<short>/ and runs an Azahar bench on it. The main folder is not changed. The run records dirty false.

C. Console run (the orchestrator pushes, the user opens netloader first)
1. `bench.py show <bench>` prints the push command (the recipe). Run it with PM_ROW=PUSH sh 3ds/push_3ds.sh ... as in docs/handoff/orchestrator_rules.md.
2. After the push, run the `bench.py ingest build3ds/hw_live_log_131.txt --bench <bench> --host o3ds --build <N> --speed normal --row <row> --label "<text>"` line that push_3ds.sh prints.
3. A walking route is not comparable between runs. Use a padscript route or a standing bench.

D. Performance in one place, over time
1. Fixed spots, the defaults: spot_mac01_* (mac_01, pos 0 0 200), spot_mac00_* (mac_00, pos -135 0 85), spot_sbk56_* (sbk_56, pos 20 0 440). _pic is an Azahar picture, _prof an O3DS prof run. `bench.py list` shows them first. `bench.py spots [--map <name>]` lists the recorded places with run counts.
2. `bench.py where --map <name> [--entry N] [--x=LO..HI] [--z=LO..HI] [--host o3ds]` (per build, in time order, deltas; negative bounds need the `=` form), and `bench.py trend <bench> [metric]` (one metric over builds).

E. Add a bench
1. Add an entry to 3ds/tools/bench_defs.json: name, kind (azahar_pic, azahar_stream, host_test, console_prof), purpose (one line), recipe, cap in seconds, until (a log line the build prints), gates, speed, tags. "default": false lists it after the others (absent = true). Reuse a scenario of azt_scenarios.json by name when one exists.
2. A new scenario goes into azt_scenarios.json in the same commit. Every scenario has a cap and an until-line.
3. `bench.py rebuild`, then `bench.py show <name>`, then one run.

F. Database lost or stale: `bench.py rebuild` makes bench.db from runs.jsonl and bench_defs.json.

## Command reference
- `list [--tag T]`, `show <bench>`, `run <bench> --row R --why "..." [--speed turbo|normal] [--label L]`, `build-at <commit> <bench> --row R [--why "..."]` (Azahar benches, see B4)
- `ingest <log> --bench B --host azahar|o3ds|n3ds [--build N] [--commit H] [--row R] [--label L] [--speed S]`, `ingest-last` (called by azt.ps1), `import-csv`
- `where`, `spots`, `compare <bench> [A B | --last 2]`, `trend <bench> [metric]`: area filters `--map --mode --story --battle --entry --x --y --z --host`, plus `--speed turbo|normal|unknown|any`, `--chg` (include scene-change windows)
- `check <run_id>`, `rebuild`

## Fields of a run
bench, host (azahar, o3ds, n3ds, host), device, build_no, commit, date, ts, row, label, result, speed (turbo, normal, unknown), frame_limit, dirty (true: a tracked file changed, except map_restore_table.c and 3ds/logs/; false: clean; null: old run), scene, log_path, shot_path, metrics. Per window (sample): frame, map, area, mode, story, battle, entry, chg, position at start and end (x y z), fps, ft_ms, nd_ms, lg_ms, gb_ms.

## Rules
- Turbo and normal rows are never averaged. Old runs have speed unknown.
- Console comparisons: same switches, same route, prof mode. phase, pmu and sp0c only split a cost.
- Scenes without a position tag record map "untagged".

## Files
- 3ds/tools/bench.py, 3ds/tools/bench_defs.json (benches, tracked), 3ds/tools/azt_scenarios.json (scenarios), 3ds/tools/azt.ps1 (Azahar runner).
- 3ds/logs/perf/runs.jsonl (source of truth, tracked, append only), 3ds/logs/perf/bench.db (index, untracked).
- Old text catalog: docs/handoff/archive/test_catalog.md and archive/test_catalog/. Read only when a row file names them.
