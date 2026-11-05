# Test and profiling benches (bench.py)

## Usage (one page)

- List or show: `python 3ds/tools/bench.py list`, `python 3ds/tools/bench.py show <bench>`.
- Azahar bench: `python 3ds/tools/bench.py run <bench> --row <row> --why "<text>"`. The `--why` text goes to 3ds/logs/perf/status_<row>.txt before the start (8 starts per row).
- Any azt.ps1 scenario start records its run itself (`bench.py ingest-last`). No bench: bench "adhoc:<scenario>".
- Host test: `bench.py run <bench> --row <row>` (metric rc). Host values: azahar, o3ds, n3ds, host (o3dsxl reads as o3ds).
- Console: `run` prints the push command. After the push, push_3ds.sh prints the `bench.py ingest ... --host o3ds --build N` line. Run it.
- Where (per build, one area): `bench.py where --map <name> [--mode N --story N --battle N --entry N --x=LO..HI --y=LO..HI --z=LO..HI --host H]`. Chg1 windows are excluded (`--chg` adds them).
- Spots (route comparison): `bench.py spots [--map <name>]` lists (map, entry, x, z rounded to 10) with run counts.
- Compare and trend take the same area filters: `bench.py compare <bench> <buildA> <buildB> --map <name>`, `bench.py trend <bench> [<metric>] --map <name>`.
- Gates of one run: `bench.py check <run_id>`. Unverified scenarios run with `-Try` (bench.py adds it).
- A log pass of an azahar_pic bench is not a pass. A person reads the shot in build3ds/azt/shots/<scenario>/.

## Files

- 3ds/tools/bench_defs.json: the benches (tracked, hand edited). Field meanings are in its note. The `legacy` list names the scenarios no bench uses.
- 3ds/tools/azt_scenarios.json: the scenarios (until line, cap, expected log events). Read by azt.ps1 and by bench.py run.
- 3ds/logs/perf/runs.jsonl: one line per run with all its metrics. Append only. This is the source of truth (tracked).
- 3ds/logs/perf/bench.db: SQLite index of runs.jsonl. Untracked. `bench.py rebuild` makes it again from runs.jsonl and bench_defs.json.
- docs/handoff/perf_history.csv: the old console history. `bench.py import-csv` loads its rows as runs (tag legacy_import).

## Benches

The bench set is in 3ds/tools/bench_defs.json. Run `bench.py list` for the current table.

## Old catalog

The scenario tables, verified and unverified states, and per-topic notes of the old text catalog are in docs/handoff/archive/test_catalog.md and docs/handoff/archive/test_catalog/. Read them only when a row file names them.
