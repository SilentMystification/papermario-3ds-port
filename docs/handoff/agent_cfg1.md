# agent_cfg1: saved logging options vs launch arguments

## Precedence (implemented)

1. Launch argument (switch string: debug3ds.txt, then the pushed arguments appended after it).
2. Saved option (options.ini keys `log`, `loglevel`).
3. Built-in default (`pm_opt` initializer in pm_options.c: log=1, loglevel=1 (warn)).

Phase, prof, pmu and fsplit have no saved value. They read the final log gate (pm_log_file()) and the switch string.

## Boot order (3ds/src/pm_main.c, game_thread)

| Step | File:line | What it does |
|---|---|---|
| A. Switch string built (file + args) | pm_main.c:600-615 | debug3ds.txt first, then `args` appended |
| B. Argument parse, gate and level | pm_log.c:200-212 (pm_log_set_switches) | `loglevel=` sets log_level. `log=off`/`log=0` sets log_file_on=0. `log=on`/`log=1` sets log_file_on=1 (ARG1, was not set before) |
| C. Writer start | pm_main.c:617-618 | pm_log_init, pm_log_start. Writer waits while gate is off |
| D. Saved values copied | pm_options.c:149-150 | sv_log, sv_loglevel = values from options.ini (or defaults) |
| E. Final apply (args win) | pm_options.c:151-157 | if `loglevel=` present: pm_opt.loglevel = pm_log_level(). If `log=` present: pm_opt.log = pm_log_file(). Else pm_opt keeps the saved value. Then pm_log_set_level and pm_log_set_file (gate, writer wake) |
| F. Phase and profiling init | pm_main.c:634 (pm_ph_init) | moved from before pm_log_start to after E. Reads pm_log_file() once E is done |
| G. Options written | pm_options.c (pm_options_write) | writes sv_log and sv_loglevel. A launch argument is never saved |
| H. Panel change | pm_options.c:183-184 (pm_options_flip rows 6 and 7) | changes pm_opt and sv_* (a user change is saved) |

## Changed in this commit

- 3ds/src/pm_log.c:210: `log=on` now sets the gate. Before, only `log=off` did.
- 3ds/src/pm_options.c: sv_log, sv_loglevel (line 13, 149-150), apply step E (151-157), write (sv_* in the fprintf), flip (183-184).
- 3ds/src/pm_main.c:634: pm_ph_init moved after pm_options_init.

## Before this change

- pm_ph_init ran before the saved log was applied. With saved `log=off` and no argument, the phase timer and the PMU counters were set up while the gate was still on, then turned off later.
- pm_options_init wrote the argument value into pm_opt.log and pm_opt.loglevel. The next options write then saved the argument value.

## Open points

- `pm_debug_value` returns the first match. If debug3ds.txt and a pushed argument both set `log=` or `loglevel=`, the file value wins. Arguments are appended after the file.
- A pushed `phase` or `prof` with saved `log=off` and no `log=on` argument stays silent. The gate has priority. Add `log=on` to the push.
- The Logging and Log level rows show the run value (pm_opt), not the saved value.

## Commands

- Build: `python 3ds/tools/gen_map_restore.py`, `sh 3ds/build.sh`, then restore `3ds/src/map_restore_table.c` from the copy taken before the generator.
- Test: none run (console decides).
