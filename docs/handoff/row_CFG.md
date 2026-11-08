# Row CFG: console run flags (log levels, profiling, run profiles, bottom-screen panel)

## User facts (2026-11-08)
"I should be able to push to a console with certain flags applied to configure the game and this should override configuration. Fully control the log level on the console and the SD. Toggle performance profiling on and off. Depending on the type of run, smartly switch between logging and profiling. This should reflect in the bottom screen information."

## State (oracle read, nds1 110ac50b3)
- Push words reach the game: push_3ds.sh 3dslink argv (278, 284); pm_main.c:792-797 argv -> args; game_thread (pm_main.c:700-730) builds sw = debug3ds.txt + " " + args, then pm_log_set_switches(sw). pm_debug_value (pm_warp.c:69) is first match wins.
- Levels: pm_log.c:59-63 lv_live, lv_disk, lv_con (-1 off, 0-3 err..debug), log_file_on. Switch words loglevel, livelevel, disklevel, conlevel, profdisk, log=off|on (pm_log_apply_switch_levels, pm_log.c:231-242). options.ini keys log_on, live_level, disk_level, con_level, prof_disk (pm_options.c:86). flag > ini already for levels (pm_options.c:197-213).
- Prof: pm_options_prof_boot (pm_options.c:233-238), pm_log_prof_row (pm_log.c:511-526), pm_log_prof_apply (529-535). prof=<N> is also the sample period (pm_phase.c:286).
- Panel: frame_draw (pm_gpu.c:1708-1722) prints pm_log_header (pm_log.c:543-548): console level and live log state only.
Gaps: G1 prof=0/off and phase=0 turn profiling ON. G2 debug3ds.txt beats push flags. G3 legacy prof=1 scan of options.ini (pm_main.c:722-729). G4 panel lacks SD level, prof mode, sources. G5 tools have no run profile; console_prof recipes toad_walk, battle_console, menu_console carry prof=4 with loglevel=debug (contradiction).

## CFG-1 (game: precedence, prof=off). Files: pm_main.c, pm_options.c, pm_log.c, pm_warp.c only if needed.
Check first: grep -n "prof=1" 3ds/src/pm_main.c hits ~726; pm_debug_value("prof" in pm_options.c:235 and pm_log.c:532 has no value test.
1. pm_main.c: sw = args first, then debug3ds.txt (first match: push flag wins). Delete the legacy prof=1 scan (722-729).
2. pm_options.c: pm_cfg_src[] (F flag, I ini, D default) for log, live, disk, con, prof, set in the ARG1 block (197-213) and pm_options_prof_boot (233-238). Export a getter for the panel.
3. prof=off|0 and phase=off|0 set mode 0 with source F. prof=<N>, N >= 4, keeps mode 1 plus the period. Same value test in pm_log.c:532.
4. lv_word (pm_log.c ~220) maps "off" to -1 for loglevel, livelevel, disklevel, conlevel.
Gate (Azahar, 3 starts, until-line = boot line "options: levels run ..." pm_options.c:216): (a) args prof=off with options.ini profmode=1 -> mode 0 in the log; (b) args loglevel=debug with loglevel=err in debug3ds.txt -> debug; (c) no args -> the ini values. Restore options.ini and debug3ds.txt after the runs. Plus spot_mac01_det byte-equal to base (no picture change).

## CFG-2 (panel, after CFG-1). Files: pm_log.c header functions, pm_gpu.c frame_draw.
Header row: `L:<lv><src> SD:<lv><src> C:<lv><src> P:<off|prof|phase><src>` (<= 39 columns, e.g. `L:D F SD:- I C:I D P:off D`). Live connection state moves to the separator row LOG_TOP-2. pm_log_header_key includes sources and lv_disk. Echo the header string once to the log at boot and on change (event line `panel: ...`).
Gate: the 3 CFG-1 cases show the right panel text (log echo plus one bottom-screen shot that shows the row).

## CFG-3 (tools, parallel with CFG-1). Files: 3ds/tools/bench_defs.json, 3ds/tools/bench.py, 3ds/push_3ds.sh, 3ds/go.ps1.
1. bench_defs.json top-level "profiles": prof = `prof=4 livelevel=err disklevel=off conlevel=off`; log = `prof=off phase=off loglevel=debug conlevel=info`; none = no words. Default per kind: console_prof -> prof; azahar_* and diagnostic -> log. A bench may set "profile".
2. bench.py push_switches and push_cmd (~753, ~1057): the recipe words first, the profile words after (first match: manual words win).
3. push_3ds.sh --profile prof|log|none (reads the words from bench_defs.json with a python one-liner, appends them after "$@"). go.ps1 -Profile.
4. Drop loglevel=debug from the console_prof recipes toad_walk, battle_console, menu_console.
Gate: bench.py show spot_mac01_prof prints the push line with the profile words after the recipe; a dry-run of push_3ds.sh --profile prof prints the 3dslink argv. No console push without the user.

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. One agent per worktree, PM_LANE = worktree name. No git stash. Never commit runs.jsonl or 3ds/src/map_restore_table.c. The CFG-2 commit deletes this row file.
