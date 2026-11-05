# Row BENCH1: test and profiling harness with a lightweight database (replaces test_catalog.md)

## Goal
Replace the text catalog by a small set of named benchmarks, a database of runs and metrics, and one CLI. An agent or the orchestrator looks up a bench by name, runs it, and compares profiling numbers between builds by command. No prose catalog.

## Start state
Branch nds1. Python: /c/Python314/python (stdlib only: sqlite3, json, csv, argparse). Never touch or commit 3ds/src/map_restore_table.c. Docs and tools only. No game source change.

## Read first (only these)
- 3ds/tools/azt.ps1 (how a scenario runs, how shots and logs are named), top 60 lines of 3ds/tools/azt_scenarios.json (field meanings), 3ds/tools/prof_record.py and 3ds/tools/prof_by_scene.py (prof line parser, reuse it, do not rewrite it), docs/handoff/perf_history.csv (data to import), docs/handoff/frame_budget.md section 3 (baseline), docs/handoff/test_catalog.md (index only).

## Design (follow it)
Files:
- 3ds/tools/bench.py: the CLI. 3ds/tools/bench_defs.json: the bench definitions (tracked, hand edited). 3ds/logs/perf/runs.jsonl: one JSON line per run with all its metrics (tracked, append only, the source of truth). 3ds/logs/perf/bench.db: SQLite index rebuilt from runs.jsonl by `bench.py rebuild` (untracked, add to .gitignore).
Bench definition (bench_defs.json), one entry per bench: name, kind (azahar_pic | azahar_stream | host_test | console_prof), purpose (one line), the run recipe (for azahar kinds: the azt.ps1 scenario name; for host_test: the command; for console_prof: the push switches, warp, story, pos, padscript), cap in seconds, the until-line (a log regex the build prints), gates (list of {metric, op, value} or {metric, op, ref:"baseline"}), tags.
Tables in bench.db: build(build_no, commit, date), bench(name, kind, purpose, defs_json), run(id, bench, build_no, commit, host, ts, row, label, result, log_path, shot_path, note), metric(run_id, win, key, value).
CLI commands (all print compact tables):
- `bench.py list [--tag T]`; `bench.py show <bench>` (recipe, gates, last 5 runs).
- `bench.py run <bench> --row <row> [--label ...]`: azahar kinds call powershell -File 3ds/tools/azt.ps1 <scenario> -Row <row>, host_test runs the command, then ingests. console_prof only prints the push command and says to run `ingest` on the log (the orchestrator or the user pushes).
- `bench.py ingest <log> --bench <name> --host <azahar|o3dsxl|n3ds> [--build N] [--commit H] [--row R] [--label L]`: parse prof lines (reuse prof_by_scene.py) and census/stream counters, write one run line to runs.jsonl and the db. Metrics per window: fps, ft_ms, nd_ms, lg_ms, gb_ms plus every numeric field of the prof stage line; per run: batches, tris, draws, closes when the log has them.
- `bench.py compare <bench> <buildA> <buildB>` (or `--last 2`): mean of each metric per build, delta and percent, a flag when a gate fails. `bench.py trend <bench> <metric>`. `bench.py check <run_id>`: evaluate the gates. `bench.py rebuild`: drop and rebuild bench.db from runs.jsonl and bench_defs.json.
- Import docs/handoff/perf_history.csv rows as runs with host o3dsxl (tag legacy_import) so the trend starts with the console history.

## Bench set (start small; 8 to 12 benches, not 187)
Curate from these. Reuse existing azt scenarios by name where one exists (grep 3ds/tools/azt_scenarios.json): boot and memory line; Toad Town standing (mac_01 pos -264 20 -326, console prof, the baseline of frame_budget.md); Toad Town fixed walk (padscript go_91_87_900,wait60,go_-300_-300_700 after warp=mac_01:0 story=ch0_toad_town); mac01_seam picture (azahar_pic); mac01_texdiff, mac01_ndplan, mac01_ndunit (azahar_stream, gate: mismatch counters equal 0); nd_emit_test and the other host tests (sh 3ds/tools/nd_host_test.sh, sh 3ds/tools/nat_host_test.sh); a battle scene and a menu scene (console_prof, pick an existing warp scenario). For each, set cap and until-line from the existing scenario. Mark every scenario of azt_scenarios.json that no bench uses as status "legacy" with a one line comment in bench_defs.json under `legacy` (a name list). Do not delete scenarios.

## Done criteria (all must pass; handback only then)
1. `python 3ds/tools/bench.py rebuild` runs clean from an empty bench.db. `bench.py list` shows 8 to 12 benches. `bench.py show` works for every bench.
2. perf_history.csv is imported. `bench.py trend` for the Toad Town standing bench prints the rows with fps and ft_ms deltas, equal to `prof_record.py trend` output for that scene.
3. `bench.py ingest` on one real log under 3ds/logs or build3ds that contains prof lines writes a run and `bench.py compare` between two imported builds prints deltas. Show the output in your report.
4. One azahar bench runs through `bench.py run` end to end (pick the shortest, for example a host test or mac01_ndplan) and writes a run.
5. A short usage section (20 lines or fewer) at the top of the new docs/handoff/test_catalog.md, which replaces the old index (the old index file and test_catalog/ go to docs/handoff/archive/ with git mv). Update docs/handoff/INDEX.md and the CLAUDE.md rule 5 first line to name bench.py as the entry point (keep every other rule 5 sentence).
6. Commit tools, defs, docs, runs.jsonl and .gitignore. No attribution. No push.

## Blockers (treat as work)
- No console log with prof lines at hand: use build3ds/hw_live_log_131.txt or search 3ds/logs for files containing "prof" lines. If none parse, unit-test the parser on a synthetic log in the report and say so.
- A scenario regex or cap looks wrong: copy it as is. Fixing scenarios is not this row.
- Windows paths: use pathlib and forward slashes in JSON.

## At the end
Write docs/handoff/agent_bench1.md with a STATE block (10 lines or fewer).
