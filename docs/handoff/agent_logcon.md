# Handoff: LOGCON (separate console log level)

Row LOGCON. Branch main. Code commit: see `git log --oneline -1 -- 3ds/src/pm_log.c`.

## State
- Console level (`conlevel`) added. Values 0 error, 1 warn (default), 2 info, 3 debug, 4 off. Log level (`loglevel`) unchanged for log.txt and the live log.
- Launch argument `conlevel=<0..4 or error, warn, info, debug, off>`. Order: argument > options.ini > default.
- options.ini key `conlevel` (read, range 0 to 4, written). Saved value kept in `sv_conlevel`.
- Options panel row "Console level" on the Debug tab, right after "Log level". A cycles error, warn, info, debug, off. Debug tab rows 8 to 10 keep their option rows (no change to their index mapping); the console row is option row 11.
- Hot path: `vlog` has one compare against `pass_lv` (max of log level and console level, off counts as -1). `drain_locked` decides per line: log.txt and socket when the line level is within `log_level`, console when within `con_lv`. A line for the console only costs no copy when the console level is off.
- logstat2 line has a new counter `console lines N` (lines the sink thread printed, since boot).

## Test (build lane logcon, build 1670-logcon, git da8feb072 before this commit)
- `logcon_err` (conlevel=error, loglevel=debug): 3 of 3 log checks. `console lines 0`. log.txt has the `logstat frames 300` debug line.
- `logcon_dbg` (conlevel=debug): 3 of 3 log checks. `console lines 105`.
- Azahar starts used for LOGCON: 2 of 8.
- Pictures not read by the agent. A person must read `build3ds\azt\shots\logcon_err` (bottom screen blank of log text) and `build3ds\azt\shots\logcon_dbg` (log text on the bottom screen).
- Live socket log not tested. Only log.txt was checked.

## Behaviour change
- A run with loglevel=debug and no conlevel now prints warn lines on the console. Before this row, the console printed what loglevel allowed. Add `conlevel=debug` to get the old console output.
- log=off still stops every sink, console included (unchanged).

## Open items
- User reads the two shots.
- Live socket check (only if the user needs it): add `livelog` to the debug string of a new scenario.

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`, then `PM_LANE=logcon sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c from a saved copy.
- Test: `PM_ROW=LOGCON powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 logcon_err -Row LOGCON -Build logcon -Try` (and `logcon_dbg`).
