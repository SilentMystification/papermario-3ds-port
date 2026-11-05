# Agent handoff: MISC1 (console colour by level, smdh metadata)

Branch `misc1` (worktree `papermario-3ds-port-misc1`, from main 1519d2974). Not pushed. The orchestrator merges.

## Done
- Console colour per log level (CONCOL). `pm_log.c` drain_locked adds an SGR escape before each console line: error `ESC[91m` (bright red), warn `ESC[33m` (yellow), info `ESC[37m` (white), debug `ESC[90m` (grey). Each line ends with `ESC[0m` before its newline. Only the console bytes carry escapes. log.txt, the socket and the ring keep plain text.
- Console buffer bound: the flush check adds 13 bytes per line (5 escape, 4 reset, newline).
- smdh metadata: `3ds/CMakeLists.txt` sets name `Paper Mario 64 3DS Port`, description `Build ${PM_BUILD_NUM}`, author `Dadrewster569`.
- `3ds/build.sh` passes `PM_BUILD_NUM` (the build string) to the container and reconfigures CMake when the number changes.
- Test aid: switch `lvtest` (`pm_log_lvtest` in `pm_log.c`, called from `pm_main.c`): one line per level at frame 100, `lvtest: done` at frame 125.

## Tests
- Scenario `logcolor` (`3ds/tools/azt_scenarios.json`, catalog section MISC1 in `docs/handoff/test_catalog.md`).
- Start 1 (`3ds/logs/perf/status_MISC1.txt`): marker missing. Another Azahar run used the shared sdmc folder and removed the log and shots of this run.
- Start 2: 6 of 6 log checks pass. Shots `build3ds/azt/shots/logcolor/bot_0115.ppm` to `bot_0118.ppm`. Picture read (bot_0116): error line bright red, warn line yellow, info white, debug grey. Rows 1 to 8, 27 to 30 unchanged.
- smdh dump (`python -I 3ds/tools/smdh_dump.py build3ds/lane-MISC1/pm_3ds.3dsx`): short `Paper Mario 64 3DS Port`, long `Build 1683-MISC1`, publisher `Dadrewster569` (all 16 title slots).

## Notes
- The `-Try` flag is needed for `logcolor` (scenario status unverified).
- The worktree needs `assets/us` and `ver/us/build` copied from the main folder (git-ignored). `3ds/src/map_restore_table.c` is modified in the worktree and never committed.
- The shared sdmc folder collides between concurrent Azahar runs. Wait for `azahar.exe` to end before a start.

## Next
- Orchestrator: merge `misc1`. Push command only after the epic check (rule 6).
