# diag_push_switches: why the prof, pmu, fsplit and sp0c lines are missing

Scope: read-only. Build 1624, git 597b0f00e. Pushed switches: `jumptest prof phase fsplit pmu loglevel=debug`.
Console log: build3ds/ftp_170k/hw_o3ds_log.txt (46 lines, first line `#13`, last line the watchdog).
Tags: CONFIRMED = read in source or log. INFERRED = reasoning from source plus log.

## 1. Does each argument reach the switch string?

- CONFIRMED. 3ds/push_3ds.sh:20-24: the option loop eats only `-t`, `--ip`, `--no-build`, `--archive-only`. The game words stay in `"$@"`.
- CONFIRMED. 3ds/push_3ds.sh:139 (3dslink, local) and :144 (docker) pass `-- --verbose livelog "$@"`.
- CONFIRMED. 3ds/src/pm_main.c:674-679 (`main`): joins `argv[1..]` with spaces into `args`. `args` is `char args[256]` (pm_main.c:24).
- CONFIRMED. 3ds/src/pm_main.c:597-614 (`game_thread`): reads debug3ds.txt into `sw[256]`, then `strncat(sw, args, ...)` at :614. Then `pm_log_set_switches(sw)` at :618.
- CONFIRMED. 3ds/src/pm_log.c:169-172 copies the string into `switches[256]` (pm_log.c:39).
- CONFIRMED. Every reader uses that one string: `pm_debug_has` (pm_log.c:113, via `debug_scan` at :101) and `pm_debug_value` (3ds/src/pm_warp.c:56, via `pm_debug_text` at pm_log.c:141).
- CONFIRMED. No truncation for the pushed words. The pushed words take about 64 bytes. The 255-byte cut at pm_main.c:600-606 applies to debug3ds.txt only.
- CONFIRMED. `phase` reaches the string. The log shows `phase W` windows (hw_o3ds_log.txt lines 2-23), and `phase` is read at pm_phase.c:~440 (`ph_want`). `jumptest` also reaches it (JT lines in the log).
- CONFIRMED. The boot line `switches <string>` (pm_main.c:632) is not in hw_o3ds_log.txt, because that file starts at `#13`. The full string is therefore not verified from this file. Check the head of the full pull.

Answer 1: all pushed words reach the string. None is dropped by the parser.

## 2. Why prof, pmu, fsplit and sp0c do not print

All four read the same string, so the cause is not the parser.

- CONFIRMED. No compile-time flag and no options.ini field gates these four. The only options.ini values in this path are `log` and `loglevel` (pm_options.c:149-150).
- CONFIRMED. The lines are logged at levels that the run shows. `prof f` and `prof cal` use `pm_log_w` (pm_phase.c:333, :418). `loglevel=debug` is set, so the level is not the cause.
- CONFIRMED. Each line waits for a count of 300:
  - `fsplit f`: pm_phase.c:223 `if (++fs_n < 300) return;`. Counted per drawn frame in `fs_frame_end` (pm_phase.c:200), called from `pm_ph_frame_end` (pm_phase.c:636).
  - `pmu f`: pm_phase.c:259 `if (++pmu_n < 300) return;`. Counted in `pmu_frame_end`, called from pm_ph_frame_end (pm_phase.c:635).
  - `sp0c f`: pm_gbi.c:2198 `if (++os_fr >= 300) { ... sp0_report(); }`. Gated by `pm_ph_on && !pm_pf_arm`. The line itself is at pm_gbi.c:1970.
  - `prof f`: pm_phase.c:392 `if (++pf_w >= 300) pf_report();`. Called from pm_main.c:493 on steps that are not skipped.
- CONFIRMED. `pm_ph_frame_end` runs only on drawn steps. pm_main.c:474 is in the `else` branch of the skip test (pm_main.c:435-476). Skipped steps do not count.
- CONFIRMED. The jump test does not wait for 300. It prints at fixed steps (`jump f270`, `f300`, `f330`, hw_o3ds_log.txt lines 13-14, 20-21, 42-43). It works in a 332-step run for that reason.
- CONFIRMED. The phase window is 60 drawn frames (pm_phase.c:573 `PC_FR 60`). The log shows windows `f1-1` and `f2-61` (line 22). No window closes at frame 121.
- CONFIRMED. The watchdog fired after steps 332, at phase 5 (gfx retrace): hw_o3ds_log.txt line 45, pm_wdog.c:62.
- INFERRED. The run ended with about 61 to 120 drawn frames. That is below the 300 that the four counters need. Skipped steps explain the gap between 332 steps and about 61 drawn frames.
- INFERRED. `prof cal` (pm_phase.c:418) prints at boot, with no 300 wait. Its absence is not proven, because the file starts at `#13`.

Answer 2: the four lines need 300 drawn frames (or 300 non-skipped steps for prof). The run stopped below that count, after the hang at step 332.

## 3. Which single change makes every pushed switch take effect

- CONFIRMED. The switch path already works. No code change to the parser is needed.
- The four missing lines need the 300 count. The literal 300 appears at pm_phase.c:223, :259, :392 and pm_gbi.c:2198.
- INFERRED. In a short run, one change in pm_phase.c could read the window length from a switch (for example `pwin=<n>` via `pm_debug_value`) and use it at those four sites. That is four edits, one helper.
- INFERRED. The fix that matters most for this push is the hang at step 332 (pm_wdog.c:62 fired at phase 5). With the hang gone, a run of the same length reaches 300 drawn frames only if the scene draws enough steps. Check the drawn count in the next pull.

No source file was changed.
