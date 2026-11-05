# Row BENCH2: record every test with build, time, device and position, and query performance by area

## Goal
Every test run (Azahar or console) records its numbers automatically, one sample per prof window, with build number, date and time, device and where Mario was. A query then answers: "what was the performance in this area at this point in time, per build".

## Start state
Branch nds1. Python: /c/Python314/python (stdlib). Tools and docs only. No game source. NO console push in this row (harness test uses saved logs and Azahar only). Never touch or commit 3ds/src/map_restore_table.c. Read first: docs/handoff/agent_bench1.md, 3ds/tools/bench.py, 3ds/tools/prof_by_scene.py (header comment lists the scene line fields), 3ds/tools/azt.ps1 (end of a run), 3ds/push_3ds.sh (where the log is written).

## Do
1. Device field. `host` takes only: azahar, o3ds (Old 3DS, XL included), n3ds, host (host tests). Map the old value o3dsxl to o3ds in runs.jsonl and bench.db on import. Add a free-text `device` note field (for example "o3ds xl 192.168.1.131").
2. Sample table. In bench.db add table `sample(run_id, win, frame, ts, build_no, host, map, area, mode, story, battle, entry, chg, x0, y0, z0, x, y, z, fps, ft_ms, nd_ms, lg_ms, gb_ms, extra_json)`: one row per prof window, all position fields parsed from the scene line (`at <map> <area>:<map> m<mode> sp<story> b<battle> e<entry> chg pos0 x y z pos x y z`) including y. Keep windows with chg1 in the table with chg=1 and exclude them from means by default. The jsonl run line keeps the same per-window data in `windows` (it is the source of truth). `ts` is the wall time of the run, plus the window frame number. Do not drop y: the old x_range and z_range stay for compatibility.
3. Query commands in bench.py:
   - `bench.py where --map <name> [--mode N] [--story N] [--battle N] [--entry N] [--x LO..HI] [--z LO..HI] [--y LO..HI] [--host H]`: per build (build_no, commit, date, host): windows, mean fps, ft_ms, nd_ms, lg_ms, gb_ms, with delta against the first row. Position bounds filter on the window end position.
   - `bench.py spots`: list the distinct (map, entry, rounded x z) spots with run counts, so a test route can be compared like for like.
   - `bench.py compare` and `trend` take the same area filters as `where`.
4. Auto record. After every azt.ps1 scenario run that has a bench, bench.py ingests the log without a manual step (azt.ps1 calls `python 3ds/tools/bench.py ingest-last` or bench.py run does it for azahar kinds; choose one path and make sure no run goes unrecorded; a scenario with no bench records as bench "adhoc:<scenario>"). After a console push, push_3ds.sh prints the exact `bench.py ingest build3ds/hw_live_log_131.txt --bench <name> --host o3ds --build <N>` line; bench.py ingest also auto-detects the build number from the log when it contains it.
5. Re-ingest the existing logs and perf_history.csv into the new tables (`bench.py rebuild` must reproduce the sample table from runs.jsonl alone).
6. Fix boot_opts: the bench boot_mem fails because scenario boot_opts expects loglevel=1 and the build prints loglevel=3 (3ds/tools/azt_scenarios.json). Change the scenario expectation to the loglevel the build prints.
7. Update the usage section at the top of docs/handoff/test_catalog.md (20 lines or fewer) with the new commands and the device values.

## Done criteria (all must pass; handback only then)
1. `bench.py rebuild` runs clean; `sample` has rows for every window of every run in runs.jsonl; host values are only azahar, o3ds, n3ds, host.
2. `bench.py where --map mac_01` prints per build rows with fps and ft_ms and deltas, from the imported console history and from build3ds/hw_live_log_131.txt ingested with --host o3ds. Paste the output in your report. If a position filter (`--x`, `--z`) is given, the window count changes accordingly (show one example).
3. `bench.py spots` prints at least one spot.
4. One azahar bench run through `bench.py run` (use boot_mem) ends with result pass and writes a run with ts, build_no, commit and host azahar. One azt.ps1 scenario run without a bench writes an "adhoc:" run (use the shortest scenario).
5. Commit tools, defs, docs, runs.jsonl (git add -f) and the scenario fix. No attribution. No push.

## Blockers (treat as work)
- A log without the scene tag: record map "untagged", position empty, and keep the run.
- Build number not in the log: take it from 3ds/logs/perf/pushed_builds.txt (last line before the log time) or leave null and print a warning.
- Do not stop on a parser mismatch: print the line, fix the parser, rerun.

## At the end
Append to docs/handoff/agent_bench1.md a STATE block (10 lines or fewer) for BENCH2.
