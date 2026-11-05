# Agent BENCH1 handoff

## STATE
- Row BENCH1 (bench harness, replaces the text test catalog): done criteria 1 to 6 checked.
- Tools: 3ds/tools/bench.py (CLI), 3ds/tools/bench_defs.json (11 benches, 179 legacy scenarios), prof_by_scene.py (generic prof fields, frame number).
- Data: 3ds/logs/perf/runs.jsonl (8 runs: 6 imported from perf_history.csv, 1 console log build 2020, 1 Azahar boot run). bench.db untracked.
- Open: boot_mem run ended fail:1. Scenario boot_opts expects loglevel=1, the boot line shows loglevel=3. Scenario not changed (row rule).
- Open: nat_host_test.sh is not in the tree. ndt_host bench replaces it.
- Open: console benches need a live push by the orchestrator or user (not run here).
- Commands: python 3ds/tools/bench.py list | show <bench> | run <bench> --row R --why "..." | ingest <log> --bench B --host H | compare B A B | trend B | rebuild.

## STATE BENCH4
- Row BENCH4 (three spots, build-at, dirty field, best-effort archive): done criteria 1 to 5 checked.
- Tools: bench.py (default flag, dirty field, build-at; copies assets/us and ver/us/build from the main folder), build.sh (PM_BUILD_REV, source volume dropped before and after), push_3ds.sh (archive best effort).
- Spots: spot_mac01_pic, spot_mac00_pic, spot_sbk56_pic log_pass (runs 34, 37, 41), shots opened. Prof spot benches not run (console push).
- build-at c62660a42 (15 back) spot_mac01_pic: run 42, build 2136, dirty False, map_restore_table.c md5 unchanged.
- Open: runs 33 (prof first marker, fixed) and 39 (shot lost, shared shots folder with another session) stay in runs.jsonl as fail records.
- Commands: bench.py build-at <commit> <bench> --row R; bench.py run spot_mac01_pic --row R --why "..."; bench.py list.
