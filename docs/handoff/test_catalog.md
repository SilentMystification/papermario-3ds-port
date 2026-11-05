# Test and profiling benches (bench.py)

## Usage (one page)

- List the benches: `python 3ds/tools/bench.py list`. Show one (recipe, gates, last runs): `python 3ds/tools/bench.py show <bench>`.
- Azahar bench (kind azahar_pic or azahar_stream): `python 3ds/tools/bench.py run <bench> --row <row> --why "<text>"`. The `--why` text is the justification line. It goes to 3ds/logs/perf/status_<row>.txt before the start. Each start counts against the row budget (8 starts per row).
- Host test (kind host_test): `python 3ds/tools/bench.py run <bench> --row <row>`. The run records metric rc (the exit code).
- Console bench (kind console_prof): `run` prints the push command only. Push in Git Bash. Then run `python 3ds/tools/bench.py ingest build3ds/hw_live_log_<last octet>.txt --bench <bench> --host o3dsxl --commit <hash>`.
- Compare two builds: `bench.py compare <bench> <buildA> <buildB>` (build = commit prefix or build number), or `bench.py compare <bench> --last 2`.
- Trend: `bench.py trend <bench>` (fps and ft per scene, the prof_record layout) or `bench.py trend <bench> <metric>`.
- Gates of one run: `bench.py check <run_id>`.
- A log pass of an azahar_pic bench is not a pass. A person reads the shot in build3ds/azt/shots/<scenario>/.
- Unverified scenarios run with `-Try`. bench.py adds the switch.

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
