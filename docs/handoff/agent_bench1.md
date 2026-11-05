# Agent BENCH1 handoff

## STATE
- Row BENCH1 (bench harness, replaces the text test catalog): done criteria 1 to 6 checked.
- Tools: 3ds/tools/bench.py (CLI), 3ds/tools/bench_defs.json (11 benches, 179 legacy scenarios), prof_by_scene.py (generic prof fields, frame number).
- Data: 3ds/logs/perf/runs.jsonl (8 runs: 6 imported from perf_history.csv, 1 console log build 2020, 1 Azahar boot run). bench.db untracked.
- Open: boot_mem run ended fail:1. Scenario boot_opts expects loglevel=1, the boot line shows loglevel=3. Scenario not changed (row rule).
- Open: nat_host_test.sh is not in the tree. ndt_host bench replaces it.
- Open: console benches need a live push by the orchestrator or user (not run here).
- Commands: python 3ds/tools/bench.py list | show <bench> | run <bench> --row R --why "..." | ingest <log> --bench B --host H | compare B A B | trend B | rebuild.

## STATE BENCH2
- Row BENCH2 (record every window, area queries): criteria 1, 3, 4, 5 checked. Criterion 2 open (see below).
- Tools: bench.py (sample table, where, spots, ingest-last, area filters on compare and trend), prof_by_scene.parse_windows, azt.ps1 Exit-Run (every start with a log is recorded), push_3ds.sh prints the record line.
- Data: runs.jsonl host o3dsxl -> o3ds, device field added. Run 14: hw_live_log_131 ingested as o3ds (build 2099 b729b930b). Run 15: boot_mem log_pass (build 2118, 6411d34ec). Run 16: adhoc:ndref_splash.
- Open: run 7 (BASE) says build 2020 for the 2099 log. Not changed (append only).
- Open: where --map mac_01 has no o3ds window from hw_live_log_131 (its one window is map "?"). Needs a console push of a mac_01 scene (no console push in BENCH2).
- Open: bench_defs.json and azt_scenarios.json hold uncommitted M4-2 hunks (mdldiff bench, rec gate, mac01_mdldiff). BENCH2 commit holds its hunks only.
