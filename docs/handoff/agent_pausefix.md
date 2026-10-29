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

## PAUSEFIX2 (epic 2, PAUSE5A and PAUSE5B): state at handoff

### State
- PAUSE5A done (a02d5ac09). Scenario only: pause_menu window inside the open pause, close START at frame 400, cap 40 s. Gates: pause_menu walk 10.7, closes 111, ops 1154; partner_menu 8.8, 79, 908; file_select 9.5, 102, 1548. Item 1 of the old list (walk window) is closed.
- PAUSE5B done (af0529787). Cold timing only (fsplit + phase). No fix: the numbers below do not show a small safe fix.
- Build 1520 archived (c5a4645f...). Not pushed. Epic complete for the Azahar check; console not measured, so the push command is not given.

### Measured (Azahar, direction only; build 1519/1520, pause opens at frame 96)
| Item | Value |
|---|---|
| Window (60 frames from BEGIN_PAUSED, f96 to f155) | avg 36.0 ms per frame, worst 168 to 175 ms (f128) |
| Readback (pm_gpu_read_frame, 1 call) | 8.8 ms |
| Filter (gfx_transfer_frame_to_depth) | 6.0 ms |
| Setup block | 9.3 ms: ROM overlay 0.0 ms, pause_init 7.3 ms, rest 1.9 ms |
| Texture loads | 95 per open, 1385 KB, tex phase 81.6 ms over the window (about 0.86 ms per load) |
| Evictions, map changes | 0, 0 |
| Worst frame phases | gbi 141 to 148 ms, idle 23 ms, tex 0, fbread 0, gpu 0.9 ms |
| GBI sub-phases of worst frame | gvtx, gtri, gmtx, gop 0.0 ms; gtev, gbind, gst 0.8 ms total (phasegbi) |
| Field, Toad Town, no pause (f241-300) | closes 16, gbiops 465, gbi 3.2 ms, cpu 13.2 ms |
| Pause steady (f176-295) | closes 96, gbiops 1003, cpu 9.3 ms, gbi about 6.6 ms (derived) |

### Open items
1. The worst frame (about 145 ms in Azahar, 32 frames after open) is in the GBI loop, not in the timed sub-phases. Need: a per-op census of frame 128 (which G_ op or handler). Diagnosis agent, row next.
2. Texture loads: 95 per open, about 0.86 ms each on Azahar. Candidate: decode or upload cost per load. A change must keep the picture (no pre-warm, no spread).
3. Top draw groups in the pause (frozen background, labels, icons, windows): not measured. Needs a census counter.
4. Console: not measured for any of the above.
5. Catalog on 1520: sewer, ruins, volcano, tour10, file_select not run (row PAUSE5B budget 8 of 8). Row INCOMPLETE until the orchestrator resets the budget.

### Next steps
- Diagnosis agent (fresh): census of frame 128 in the pause open. Name the GBI op or handler. Use `pause_pcopen_gbi` (phasegbi) and a new counter if needed.
- After the census: one code row for the named handler, if it is small and keeps the picture.
- Budget reset for PAUSE5B, then run the remaining catalog scenarios (sewer, ruins, volcano, tour10, file_select).

### Commands
- Build: `python 3ds/tools/gen_map_restore.py`, `PM_ROW=PAUSE5B sh 3ds/build.sh`, then `cp 3ds/src/map_restore_table.c.bak 3ds/src/map_restore_table.c`.
- Timing: `PM_ROW=PAUSE5B powershell -File 3ds/tools/azt.ps1 pause_pcopen -Row PAUSE5B -Try` (pcopen line), `pause_pcopen_gbi` (pcgbi line), `field_mac00` (gameplay window).
- Gates: `pause_menu`, `partner_menu`, `file_select` with `-Row PAUSE5A` (budget used: 8 of 8, reset needed).
- Archive: `PM_ROW=PAUSE5B sh 3ds/push_3ds.sh --archive-only` (done: 3ds/logs/builds/pm_3ds_1520_af0529787.3dsx).
