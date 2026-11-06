# Bench process (python 3ds/tools/bench.py)

One tool runs tests, records every result and answers queries. Do not read runs.jsonl or the old catalog by hand. Do not hand-build run_azahar.ps1 commands for a test a bench covers. Numbers live in the database, not in prose. The run history is append-only: every run stays.

## Process by situation

A. Check a change (a row)
1. `bench.py list`, then `bench.py show <bench>` for the benches the row file names (recipe, gates, last runs).
2. For each bench: `PM_ROW=<row> python 3ds/tools/bench.py run <bench> --row <row> --why "<quantity, code path, why no other run answers it>"`. The tool writes the why line, starts the test and records the run. Add `--lane <lane>` to test a lane build (build3ds/lane-<lane>/pm_3ds.3dsx). Several Azahar benches go in one call, `run a,b,c` (see Slots and lists below).
3. `bench.py check <run_id>`: the gates. PASS or FAIL per gate.
4. kind azahar_pic: also open the shot in build3ds/azt/shots/<scenario>/ and confirm the subject. A log pass is not a pass.
5. Budget: 8 Azahar starts per row. At the cap with gates unrun: reset (powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget <row>) and go on.

B. Before and after a change
1. Run the bench on the base build (record it), change the code, rebuild, run the bench again with the same speed (`--speed`).
2. `bench.py compare <bench> <buildA> <buildB>`, or `--last 2`. The tool prints the delta of each metric and flags failed gates. It prints WARNING and no delta when the speeds differ.
3. Console speed claims use console_prof benches (prof mode) only. Azahar numbers show direction.
3a. Display mode: the prof scene line may end with " disp w<0|1> s<0|1> f<fps target>". where, spots, compare and trend show it as disp (w1s0f30; - for old logs). A query uses one mode: the newest row's by default, `--disp w1s0f30|-|any` to pick. A line tells how many rows of other modes it dropped.
4. An older commit: `bench.py build-at <commit> <bench> --row R` builds it in build3ds/at_<short>/ and runs an Azahar bench on it. The main folder is not changed. The run records dirty false.
   Console: `bench.py build-at <commit> <console bench> --row R [--dry-run]` builds it as lane at_<short>, pushes that 3dsx to the O3DS 192.168.1.170 and ingests the live log (dirty false). --dry-run prints the 3 commands only.

C. Console run (the orchestrator pushes, the user opens netloader first)
1. `bench.py show <bench>` prints the push command (the recipe). Run it with PM_ROW=PUSH sh 3ds/push_3ds.sh ... as in docs/handoff/orchestrator_rules.md.
2. After the push, run the `bench.py ingest build3ds/hw_live_log_170.txt --bench <bench> --host o3ds --build <N> --speed normal --row <row> --label "<text>"` line that push_3ds.sh prints.
3. A walking route is not comparable between runs. Use a padscript route or a standing bench.
5. `sh 3ds/push_3ds.sh --attach --ip <ip> -t <s>` reads the live log of a game already running (no send). A live log that stops early is read again with livelog.py --append (3 times). A send that fails after the console answered still reads the live log.
6. After a failed send, push_3ds.sh ingests nothing unless the live log boot line is the sent build (else STALE BINARY). `sh 3ds/push_3ds.sh --check-log <file>` runs that check only (PM_SENT_BUILD, PM_SENT_GIT). bench.py ingest of a console log refuses (ERROR, nothing written) when its boot line differs from --build or --commit.
4. `PM_SRC3DSX=<3dsx> sh 3ds/push_3ds.sh --no-build --ip 192.168.1.170 <switches>` sends that 3dsx instead of build3ds/pm_3ds.3dsx (no archive: the main pm_build.h does not describe it). build-at uses this.

D. Performance in one place, over time
1. Fixed spots, the defaults: spot_mac01_* (mac_01, pos 0 0 200), spot_mac00_* (mac_00, pos -135 0 85), spot_sbk56_* (sbk_56, pos 20 0 440). _pic is an Azahar picture, _prof an O3DS prof run. `bench.py list` shows them first. `bench.py spots [--map <name>]` lists the recorded places with run counts.
2. `bench.py where --map <name> [--entry N] [--x=LO..HI] [--z=LO..HI] [--host o3ds]` (per build, in time order, deltas; negative bounds need the `=` form), and `bench.py trend <bench> [metric]` (one metric over builds).

E. Add a bench
1. Add an entry to 3ds/tools/bench_defs.json: name, kind (azahar_pic, azahar_stream, host_test, console_prof), purpose (one line), recipe, cap in seconds, until (a log line the build prints), gates, speed, tags. "default": false lists it after the others (absent = true). Reuse a scenario of azt_scenarios.json by name when one exists.
2. A new scenario goes into azt_scenarios.json in the same commit. Every scenario has a cap and an until-line.
3. `bench.py rebuild`, then `bench.py show <name>`, then one run.

F. Database lost or stale: `bench.py rebuild` makes bench.db from runs.jsonl and bench_defs.json.

## Slots, warm restart and lists (Azahar)
- Slots: N private portable Azahar copies (env PM_AZ_SLOTS, default 2) in build3ds/azslots/slot<k>/. One named mutex per slot, so runs on different slots go in parallel. A start that fails before its first marker with the GPU fault (Vulkan "Failed to allocate Stream buffer" in the emulator log, or exit 0xC000001D) is retried once on the same slot after 10 s (azt.ps1, "AZT: GPU memory fault, retry"). A run takes the free slot whose kept Azahar restarts it (same lane 3dsx and turbo setting: warm), then a free slot with no kept Azahar, then any free slot. It waits only when all are busy. Each slot has its own sdmc, config and logs; the user's Azahar install and %APPDATA% are only read.
- Keep (default since AZT-3b): after a run that matched its until line, the slot's Azahar stays open, paused. The next run on that slot restarts emulation in it (warm, first marker about 1 s; a cold start takes 12 to 16 s). An idle kept Azahar closes after 3 min (azreap.ps1). `azt.ps1 ... -NoKeep` closes it after the run. A run with another 3dsx, other turbo settings or -Interactive starts cold.
- List: `azt.ps1 a,b,c` (or `bench.py run a,b,c --row R --why "..." [--lane L]`) runs the scenarios one after the other, one start each, on one kept slot. All names are checked before the first start (an unknown, unverified or rejected scenario stops the whole list). The benches of a list share one speed. `--label` applies to every run of the list; without it each run is labelled by its scenario. The LIST lines print the wall time of each scenario and of the list.
- Warm line: each start writes `azslot: slot <k> warm` or `azslot: slot <k> cold (<reason>)` (reason: no kept, closed, other lane, turbo, option, restart failed). azt.ps1 puts it as the first line of build3ds/azt/logs/<scenario>.txt and records it as `warm` 1 or 0 in the run record (absent when the start ended before the decision).
- Logs: azt.ps1 prints `LOG:`, the copy of this run's game log in build3ds/azt/logs/<scenario>.txt, and its shots are in build3ds/azt/shots/<scenario>/. Read those copies, not build3ds/last_log.txt (last writer wins, and a slot run overwrites its slot copy at the next run).
- Until-line check: azt.ps1 checks the until line against the C sources. The source text is cached in build3ds/azt/srctext.cache (rebuilt when a .c or .h file changes).

## Command reference
- `list [--tag T]`, `show <bench>`, `run <bench>[,<bench>...] --row R --why "..." [--speed turbo|normal] [--label L] [--lane L]`, `build-at <commit> <bench> --row R [--why "..."]` (Azahar benches, see B4)
- `ingest <log> --bench B --host azahar|o3ds|n3ds [--build N] [--commit H] [--row R] [--label L] [--speed S]`, `ingest-last` (called by azt.ps1), `import-csv`
- `where`, `spots`, `compare <bench> [A B | --last 2]`, `trend <bench> [metric]`: area filters `--map --mode --story --battle --entry --x --y --z --host`, plus `--speed turbo|normal|unknown|any`, `--chg` (include scene-change windows)
- `check <run_id>`, `rebuild`

## Fields of a run
bench, host (azahar, o3ds, n3ds, host), device, build_no, commit, date, ts, row, label, result, speed (turbo, normal, unknown), frame_limit, warm (1 warm restart, 0 cold, absent: unknown), dirty (true: a tracked file changed, except map_restore_table.c and 3ds/logs/; false: clean; null: old run), scene, log_path, shot_path, metrics. Per window (sample): frame, map, area, mode, story, battle, entry, chg, position at start and end (x y z), fps, ft_ms, nd_ms, lg_ms, gb_ms.

## Rules
- Turbo and normal rows are never averaged. Old runs have speed unknown.
- Console recipes carry only the switches named in the bench recipe (prof=4 for spots). No stat switches by default: they change the frame time. Console comparisons: same route, prof mode; phase, pmu and sp0c only split a cost.
- Scenes without a position tag record map "untagged".

## Files
- 3ds/tools/bench.py, 3ds/tools/bench_defs.json (benches, tracked), 3ds/tools/azt_scenarios.json (scenarios), 3ds/tools/azt.ps1 (Azahar runner), 3ds/tools/azslot.ps1 (slot pool, warm restart), 3ds/tools/azreap.ps1 (closes an idle kept Azahar), 3ds/run_azahar.ps1 (one run; -Slot, -Keep, -NoWarm, -ResetBudget).
- 3ds/logs/perf/runs.jsonl (source of truth, tracked, append only), 3ds/logs/perf/bench.db (index, untracked).
- Old text catalog: docs/handoff/archive/test_catalog.md and archive/test_catalog/. Read only when a row file names them.
