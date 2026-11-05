# Agent BENCH1 handoff

## STATE
- Row BENCH1 (bench harness, replaces the text test catalog): done criteria 1 to 6 checked.
- Tools: 3ds/tools/bench.py (CLI), 3ds/tools/bench_defs.json (11 benches, 179 legacy scenarios), prof_by_scene.py (generic prof fields, frame number).
- Data: 3ds/logs/perf/runs.jsonl (8 runs: 6 imported from perf_history.csv, 1 console log build 2020, 1 Azahar boot run). bench.db untracked.
- Open: boot_mem run ended fail:1. Scenario boot_opts expects loglevel=1, the boot line shows loglevel=3. Scenario not changed (row rule).
- Open: nat_host_test.sh is not in the tree. ndt_host bench replaces it.
- Open: console benches need a live push by the orchestrator or user (not run here).
- Commands: python 3ds/tools/bench.py list | show <bench> | run <bench> --row R --why "..." | ingest <log> --bench B --host H | compare B A B | trend B | rebuild.

## STATE BENCH3
- Row BENCH3 (speed mode of every run): done criteria 1 to 4 checked.
- Tools: bench.py (speed and frame_limit fields; --speed on run, where, spots, compare, trend; check and show per speed), azt.ps1 -Speed turbo|normal (a scenario with -Turbo records turbo), bench_defs.json speed per bench (normal for all).
- Data: run 21 (boot_mem turbo, build 2123 b553ce977, frame_limit 1000), run 22 (boot_mem normal, build 2123), run 23 (boot_opts normal, lane base binary, build 2099 b729b930b). Runs 1 to 20: speed unknown (not guessed).
- Open: where has no rows for the boot runs (no windows, no sample rows). Use trend and show.
- Open: console runs stay unknown. The default (normal) toad_standing query shows none until a console run is ingested with --speed normal.
- Open: runs 18 to 20 in runs.jsonl (other session, builds 2122 and 2123) are not in the BENCH3 commit.
- Commands: bench.py trend boot_mem --speed turbo | show boot_mem | compare boot_mem b729b930b b553ce977 --speed any.
