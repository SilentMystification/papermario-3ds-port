# Bench process (python 3ds/tools/bench.py)

> NO VISUAL CHECKS (user, 2026-11-10, CLAUDE.md rule 1): no agent looks at or judges a picture. Picture gates in older rows are void; gates are nd_host and the mechanical byte compare. The look is the user's console check.


One tool runs tests, records every result and answers queries. Do not read runs.jsonl or the old catalog by hand. Do not hand-build run_azahar.ps1 commands for a test a bench covers. Numbers live in the database, not in prose. The run history is append-only: every run stays.

## Process by situation

A. Check a change (a row)
1. `bench.py list`, then `bench.py show <bench>` for the benches the row file names (recipe, gates, last runs).
2. For each bench: `PM_ROW=<row> python 3ds/tools/bench.py run <bench> --row <row> --why "<quantity, code path, why no other run answers it>"`. The tool writes the why line, starts the test and records the run. Add `--lane <lane>` to test a lane build (build3ds/lane-<lane>/pm_3ds.3dsx). Several Azahar benches go in one call, `run a,b,c` (see Slots and lists below).
3. `bench.py check <run_id>`: the gates. PASS or FAIL per gate.
4. kind azahar_pic: also open the shot in build3ds/azt/shots/<scenario>/<build>/ (build = the -Build lane name, or main) and confirm the subject. A log pass is not a pass.
5. Budget: 8 Azahar starts per row. At the cap with gates unrun: reset (powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget <row>) and go on.
6. Byte gates against main (the det shots): no build of main per row. The base is stored (BASECACHE):
   `PM_ROW=<row> PM_LANE=<row> python 3ds/tools/bench.py base <bench>` returns the stored shots of the merge-base of HEAD with main at once.
   Missing: it builds that commit ONCE (lane base_<short>, the committed tree only), runs the bench, stores its shots and log in
   <main folder>/build3ds/basecache/<commit>/<bench>/ and deletes the lane. Then run the row's own bench (step 2), then
   `python 3ds/tools/bench.py cmp <bench> --row <row>`: byte-equal count, per shot diff pixels and bounding box, one diff PNG per
   differing shot in build3ds/basecmp/<row>/<bench>/. Exit 0 when every shot is byte-equal. The orchestrator fills the store after a
   merge with `python 3ds/tools/bench.py base --fill` (main folder, its build, no build).

B. Before and after a change
1. Run the bench on the base build (record it), change the code, rebuild, run the bench again with the same speed (`--speed`).
2. `bench.py compare <bench> <buildA> <buildB>`, or `--last 2`. The tool prints the delta of each metric and flags failed gates. It prints WARNING and no delta when the speeds differ.
3. Console speed claims use console_prof benches (prof mode) only. Azahar numbers show direction.
3a. Display mode: the prof scene line may end with " disp w<0|1> s<0|1> f<fps target>". where, spots, compare and trend show it as disp (w1s0f30; - for old logs). A query uses one mode: the newest row's by default, `--disp w1s0f30|-|any` to pick. A line tells how many rows of other modes it dropped.
4. An older commit: `bench.py build-at <commit> <bench> --row R` builds it in build3ds/at_<short>/ and runs an Azahar bench on it. The main folder is not changed. The run records dirty false. A byte reference of main is not a build-at: use `base` (A6).
   Console: `bench.py build-at <commit> <console bench> --row R [--dry-run]` builds it as lane at_<short>, pushes that 3dsx to the O3DS 192.168.1.170 and ingests the live log (dirty false). --dry-run prints the 3 commands only.

C. Console run (the orchestrator pushes, the user opens netloader first)
1. `bench.py show <bench>` prints the push command (the recipe). Run it with PM_ROW=PUSH sh 3ds/push_3ds.sh ... as in docs/handoff/orchestrator_rules.md.
2. After the push, run the `bench.py ingest build3ds/hw_live_log_170.txt --bench <bench> --host o3ds --build <N> --speed normal --row <row> --label "<text>"` line that push_3ds.sh prints.
3. A walking route is not comparable between runs. Use a padscript route or a standing bench.
5. `sh 3ds/push_3ds.sh --attach --ip <ip> -t <s>` reads the live log of a game already running (no send). The read ends at -t, or when no complete line came for `--idle <s>` seconds (default 300; it prints `live log idle <s> s: stop`, livelog.py exit 4, no probe). go.ps1 push o/n passes its own pid as the owner (`-IdleSeconds` sets the idle limit): when the owner is gone, the live log and the netload tail end at once. A read that ends by silence is read again with livelog.py --append (3 times). A send that fails after the console answered still reads the live log.
6. After a failed send, push_3ds.sh ingests nothing unless the live log boot line is the sent build (else STALE BINARY). `sh 3ds/push_3ds.sh --check-log <file>` runs that check only (PM_SENT_BUILD, PM_SENT_GIT). bench.py ingest of a console log refuses (ERROR, nothing written) when its boot line differs from --build or --commit.
4. `PM_SRC3DSX=<3dsx> sh 3ds/push_3ds.sh --no-build --ip 192.168.1.170 <switches>` sends that 3dsx instead of build3ds/pm_3ds.3dsx (no archive: the main pm_build.h does not describe it). build-at uses this.
7. NETLOADER REFUSED: a send that stops below 100% (a `N sent (P%)` line under 100, then no log) prints a block with the 3dsx size and the last line of 3ds/logs/perf/pushed_builds.txt. The text the user sees: "NETLOADER REFUSED (likely not enough free RAM in applet mode). On the 3DS: hold R while you start any game to open the Homebrew Launcher in title mode with full memory, then start the netloader (Y) and push again." Test with no console: `sh 3ds/tests/push/run.sh`, or `sh 3ds/push_3ds.sh --check-send <saved 3dslink output>`.

D. Performance in one place, over time
1. Fixed spots, the defaults: spot_mac01_* (mac_01, pos 0 0 200), spot_mac00_* (mac_00, pos -135 0 85), spot_sbk56_* (sbk_56, pos 20 0 440). _pic is an Azahar picture, _prof an O3DS prof run. `bench.py list` shows them first. `bench.py spots [--map <name>]` lists the recorded places with run counts.
2. `bench.py where --map <name> [--entry N] [--x=LO..HI] [--z=LO..HI] [--host o3ds]` (per build, in time order, deltas; negative bounds need the `=` form), and `bench.py trend <bench> [metric]` (one metric over builds).

E. Add a bench
1. Add an entry to 3ds/tools/bench_defs.json: name, kind (azahar_pic, azahar_stream, host_test, console_prof), purpose (one line), recipe, cap in seconds, until (a log line the build prints), gates, speed, tags. "default": false lists it after the others (absent = true). Reuse a scenario of azt_scenarios.json by name when one exists.
2. A new scenario goes into azt_scenarios.json in the same commit. Every scenario has a cap and an until-line.
3. `bench.py rebuild`, then `bench.py show <name>`, then one run.

F. Database lost or stale: `bench.py rebuild` makes bench.db from runs.jsonl and bench_defs.json.

## Slots, warm restart and lists (Azahar)
- Slots: N private portable Azahar copies (env PM_AZ_SLOTS, default 3, clamped to the cap) in build3ds/azslots/slot<k>/. One named mutex per slot, so runs on different slots go in parallel. Cap (AZT4-1, constant AzCap in azslot.ps1): at most 3 live azahar.exe of any slot folder, all worktrees (AZT12-4: a 4th live Azahar fails at start with the GPU stream-buffer fault; 3 active cost a run 4 % against 2). At the cap a start takes a free slot whose Azahar is live (restart) before a cold slot; a cold start closes the idle kept Azahars of other slots first (Close-IdleKept), else waits. A start that fails before its first marker with the GPU fault (Vulkan "Failed to allocate Stream buffer" in the emulator log, or exit 0xC000001D) closes every idle kept Azahar (any worktree), then retries once on the same slot after 2 s (azt.ps1, "azslot: gpu fault, closed <n> idle, retry"). A run takes the free slot whose kept Azahar restarts it (same lane 3dsx and turbo setting: warm), then a free slot with no kept Azahar, then any free slot. It waits only when all are busy. Each slot has its own sdmc, config and logs; the user's Azahar install and %APPDATA% are only read.
- Keep (default since AZT-3b): after a run that did not crash or exit (a timeout or a missed until line too, AZT3-2), the slot's Azahar stays open, paused. The next run on that slot restarts emulation in it (warm, first marker about 1 s; a cold start takes 12 to 16 s). An idle kept Azahar closes after 60 min (azreap.ps1; one value, azslot.ps1 AzIdleSec = 3600). `azt.ps1 ... -NoKeep` closes it after the run. A run whose Azahar speed differs (frame_limit and vsync of the slot config, azslot.ps1 Get-SlotSpeed: turbo is vsync off and TurboLimit, but a config limit of 0 is already unthrottled, so with the user's config turbo and normal share one kept Azahar) or -Interactive starts cold (AZT3-1: any lane restarts in the kept Azahar). The restart key (F6) is posted to the kept pid's window between a posted WM_SETFOCUS and WM_KILLFOCUS (no foreground change; Qt fires the shortcut only with an active window). The restart is proven by Azahar's BootGame line in its log after the key and by log.old coming back (the run deletes it, the game's log rotation recreates it); a failed restart leaves slot<k>/warmfail_emu.txt and warmfail_log.txt. Det scenarios restart warm too: a warm restart reboots the emulated system and gives the same det bytes as a cold start (AZT12-2, spot_mac01_det shots 200 to 205 byte-equal warm vs cold), so no scenario carries -NoWarm (run_azahar.ps1 -NoWarm stays for a manual run).
- List: `azt.ps1 a,b,c` (or `bench.py run a,b,c --row R --why "..." [--lane L]`) runs the scenarios one after the other, one start each, on one kept slot. All names are checked before the first start (an unknown, unverified or rejected scenario stops the whole list). The benches of a list share one speed. `--label` applies to every run of the list; without it each run is labelled by its scenario. The LIST lines print the wall time of each scenario and of the list.
- Warm line: each start writes `azslot: slot <k> warm` or `azslot: slot <k> cold (<reason>)` (reason: no kept, closed, speed, option, restart failed). azt.ps1 puts it as the first line of build3ds/azt/logs/<scenario>.txt and records it as `warm` 1 or 0 in the run record (absent when the start ended before the decision).
- Logs: azt.ps1 prints `LOG:`, the copy of this run's game log in build3ds/azt/logs/<scenario>.txt, and its shots are in build3ds/azt/shots/<scenario>/<build>/ (one folder per scenario and build: a run of another lane keeps its shots). Read those copies, not build3ds/last_log.txt (last writer wins, and a slot run overwrites its slot copy at the next run).
- Test-event lines: a line a scenario uses as its until-line (the shot line, ndref done, encwarp window end, jump f ran) is printed with pm_log_evt, which passes the logging gate in every mode (logging on, off, Profiling). The io gates still apply (log file open, live client). Other until-lines use pm_log_prof_line (prof lines) or the level lines.
- Until-line check: azt.ps1 checks the until line against the C sources. The source text is cached in build3ds/azt/srctext.cache (rebuilt when a .c or .h file changes).

## Command reference
- `list [--tag T]`, `show <bench>`, `run <bench>[,<bench>...] --row R --why "..." [--speed turbo|normal] [--label L] [--lane L]` (--lane default: PM_LANE, else in a worktree build3ds/wt/<row> its branch; a missing lane 3dsx stops the run with one line), `build-at <commit> <bench> --row R [--why "..."]` (Azahar benches, see B4)
- `ingest <log> --bench B --host azahar|o3ds|n3ds [--build N] [--commit H] [--row R] [--label L] [--speed S]`, `ingest-last` (called by azt.ps1), `import-csv`
- `where`, `spots`, `compare <bench> [A B | --last 2]`, `trend <bench> [metric]`: area filters `--map --mode --story --battle --entry --x --y --z --host`, plus `--speed turbo|normal|unknown|any`, `--chg` (include scene-change windows)
- `check <run_id>`, `rebuild`
- `base <bench> [--commit C] [--row R]`, `base --fill` (main folder), `cmp <bench> --row R [--commit C]` (A6: stored base shots, byte compare)
- `azstat [--since <ISO date or time>] [--row R]` (default today, all rows; --row: that row's runs and the pool.log lines of build3ds/wt/<row>): Azahar throughput of every tree's runs.jsonl (starts, warm and cold, wait_s, boot_s, until_s, wall_s: median, p90, max) and the slot pool events of build3ds/azslots/pool.log (cold starts by reason, kills by reason, warm restarts, stale boots).

## Fields of a run
bench, host (azahar, o3ds, n3ds, host), device, build_no, commit, date, ts, row, label, result, speed (turbo, normal, unknown), frame_limit, warm (1 warm restart, 0 cold, absent: unknown), wait_s (queue wait, enqueue to grant), boot_s (Azahar start or warm restart to the first game line of the boot), until_s, wall_s, dirty (true: a tracked file changed, except 3ds/logs/; false: clean; null: old run), scene, log_path, shot_path, metrics. Console prof windows also keep the PMU and core2 values (pmu_i, pmu_d, pmu_cyc, pmu_stall: 1 for the stall event; lgms, wwait, ser, stk, ic1, dc1, cyc1, kf, bw, fw, aj, au, mx, drain_dr, drain_drw, drain_wqf): per run as metrics (`bench.py trend <bench> pmu_i`, compare) and per window in extra_json.
Per window (sample): frame, map, area, mode, story, battle, entry, chg, position at start and end (x y z), fps, ft_ms, nd_ms, lg_ms, gb_ms.

## Rules
- Turbo and normal rows are never averaged. Old runs have speed unknown.
- Console recipes carry only the switches named in the bench recipe (prof=30 for spots). No stat switches by default: they change the frame time. Console comparisons: same route, prof mode; phase, pmu and sp0c only split a cost.
- Scenes without a position tag record map "untagged".
- No scene under test: warp=random (go.ps1 pushes add it when no spot is named). warp=random:<seed> boots to the spot the seed picks (pm_tour_spots); the log line `warp: random seed <n> -> map:entry@x:y:z` repeats the run. A scenario that tests a scene keeps its fixed spot.

## Files
- 3ds/tools/bench.py, 3ds/tools/bench_defs.json (benches, tracked), 3ds/tools/azt_scenarios.json (scenarios), 3ds/tools/azt.ps1 (Azahar runner), 3ds/tools/azslot.ps1 (slot pool, warm restart), 3ds/tools/azreap.ps1 (closes an idle kept Azahar), 3ds/run_azahar.ps1 (one run; -Slot, -Keep, -NoWarm, -ResetBudget).
- 3ds/logs/perf/runs.jsonl (source of truth, tracked, append only), 3ds/logs/perf/bench.db (index, untracked).
- Base store: <main folder>/build3ds/basecache/ (index.json, <commit>/<bench>/ shots, run.log, meta.json; last 5 commits). Compare output: build3ds/basecmp/<row>/.
- Old text catalog: docs/handoff/archive/test_catalog.md and archive/test_catalog/. Read only when a row file names them.
