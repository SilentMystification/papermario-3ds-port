# Agent PAUSEFIX (epic E3): handoff

## State
- PAUSE4A done. Commit 9461b95a8 (src/background_gfx.c, TARGET_3DS path only).
- PAUSE4B not changed: the premise is wrong (see below).
- PAUSE4C not changed: readback already timed; open-spike rest not measured.
- Gates reset: pause_menu closes 111, ops 1154, walk 14.0 (see open item 1). partner_menu and file_select unchanged (new values are lower).

## Commits
- 9461b95a8 PAUSE4A.
- Uncommitted by this agent: 3ds/tools/azt_scenarios.json (pause_menu gate values), docs/handoff/test_catalog.md (catalog note), 3ds/logs/perf/pushed_builds.txt (archive line). Other stale hunks in these files are not this agent's and are not staged.

## Measured (Azahar, direction only)
| Scenario | HEAD 1514 | New 1517 | Note |
|---|---|---|---|
| pause_menu closes | 135 | 96 | flush count |
| pause_menu ops | 1237 | 1003 | gbiops |
| pause_menu walk cpu ms | 11.4 to 11.5 (f175 to f234) | 9.3 (f176 to f235); 13.6 to 13.9 (f120 to f175) | window mixes pause and field |
| partner_menu closes / ops / walk | - | 68 / 789 / 7.3 | HEAD 107 / 1023 / 9.9 |
| file_select closes / ops / walk | 88 / 1346 / 8.2 | 88 / 1346 / 8.2 | unchanged |

## Open items
1. pause_menu walk window: the until `phase W f1[2-9]\d-` matches f120 to f175 (mixed: pause until about f141, then field) or f175 to f234 (field). Both occur. Decide: change the until so the window is one state, or accept the gate at 14.0.
2. PAUSE4B: diag_pause3.md "111 label texture changes in pause_draw_rect" is a subtraction (151 real tex closes minus 40 strips). pause_draw_rect (src/pause/pause_main.c:852) emits a texrect only. Labels: 9 display lists (src/pause/pause_gfx.c:83 onward), one texture each. Bound about 9 x 23.6 us = 0.2 ms. Not converted to G_PM_TEXI.
3. PAUSE4C: the open spike (112 ms max) is not split. Readback is the D03 line (pm_main.c:262, pm_gpu.c:1492). Next: one run with phase on, read texKB and texmiss on frames 96 to 120.
4. Picture by eye: shots are byte-equal to HEAD (pause_menu, partner_menu, file_select, text_menu). A person must open them.
5. Console: not measured. Push not given (epic partial).

## Next steps
- Decide item 1, then reset the pause_menu walk gate to +15 % of its window's value.
- Item 3 needs a run: `PM_ROW=PAUSE4C powershell -File 3ds/tools/azt.ps1 pause_menu -Row PAUSE4C` with a phase-on window line for f96 to f120 added to the scenario (new scenario entry, catalog row in the same commit).

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`, `sh 3ds/build.sh`, then `cp 3ds/src/map_restore_table.c.bak 3ds/src/map_restore_table.c`.
- HEAD baseline lane: `build3ds/lane-pbase/pm_3ds.3dsx` (copy of 3ds/logs/builds/pm_3ds_1514_6ffb3df08.3dsx). Run: `azt.ps1 <scenario> -Row <row> -Build pbase`.
- Archive: `PM_ROW=PAUSE4C sh 3ds/push_3ds.sh --archive-only`. Archive: 3ds/logs/builds/pm_3ds_1517_9461b95a8.3dsx.
- Push (not given, epic partial): `PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.170 audiostat texstat pacestat logstat phase phsum loglevel=debug`.
