# Agent BENCH1 handoff

## STATE
- Row BENCH1 (bench harness, replaces the text test catalog): done criteria 1 to 6 checked.
- Tools: 3ds/tools/bench.py (CLI), 3ds/tools/bench_defs.json (11 benches, 179 legacy scenarios), prof_by_scene.py (generic prof fields, frame number).
- Data: 3ds/logs/perf/runs.jsonl (8 runs: 6 imported from perf_history.csv, 1 console log build 2020, 1 Azahar boot run). bench.db untracked.
- Open: boot_mem run ended fail:1. Scenario boot_opts expects loglevel=1, the boot line shows loglevel=3. Scenario not changed (row rule).
- Open: nat_host_test.sh is not in the tree. ndt_host bench replaces it.
- Open: console benches need a live push by the orchestrator or user (not run here).
- Commands: python 3ds/tools/bench.py list | show <bench> | run <bench> --row R --why "..." | ingest <log> --bench B --host H | compare B A B | trend B | rebuild.
