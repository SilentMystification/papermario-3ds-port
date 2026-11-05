# Handoff: LOGTRIM epic (WARPLOG2, PRIMZFIX, UNTILCHK, WMODEL)

## State
- WARPLOG2: committed d99eea457. Spike, b7, V09F5, gv1/gp1/gp3, dr1, dlc, pdl, logtop lines removed. Spike count and worst step are in the logstat line (`spikes N max M ms`). Pace once per 5 s. Warp scene line every 300 frames.
- PRIMZFIX: committed 9c0387c33. Fix in `prim_ndc_z()` (pm_gbi.c): prim_z - 1, clamped at 0. Repro switch `zround` (pm_gpu.c, debug). Scenario toad_pond_zround.
- UNTILCHK: committed 9601f4eaa. azt.ps1: static until check (exit 7, nothing started), 40 s stall stop (exit 6), `-SelfTest`.
- WMODEL: committed db9c007f1. fsplit line adds `wm mean max tk mean max` (world list count, render tasks per frame).
- Final build: 1511, git db9c007f1, archive 3ds/logs/builds/pm_3ds_1511_db9c007f1.3dsx, sha256 2d97e3f34420605813c71b42b4b0a560ce73a0d973b283ff38f5b3e64776d895 (equal to build3ds/pm_3ds.3dsx). pushed_builds.txt line added (archive-only, not pushed).
- Epic status: INCOMPLETE. Catalog: 8 of 9 run (toad_pond not run, LOGTRIM budget at 8).

## Gates
- WARPLOG2: yard_pace warp window (entry to warp scene f300). HEAD 67 lines, new 30 lines. Target under 15 is NOT met on Azahar. Console estimate (diag_warpnoise.md) 127.6 to about 12.5. Log checks pass: yard 3 of 3, pause_menu 6 of 6, mac01t 6 of 6.
- PRIMZFIX: HEAD is not byte-reproducible (2 runs differ in x 92 to 138, the NPC strip). Fixed code (switch off) equals HEAD outside that strip, 10 of 10 shots. zround repro on old code: pond water (x 20 to 60) unchanged. Does NOT reproduce. Fixed code with zround: same picture.
- UNTILCHK: self-test 5 of 5 pass. Stall rule not seen live (the yard run logs every few seconds).
- WMODEL: yard_fsplit f600 wm 103.0 tk 2.00. mac01t_fsplit f900 wm 143.0 tk 2.00. Top shots: yard 1 of 1 byte-equal (HEAD run x2 equal), mac01t 2 of 2 byte-equal. logstat work HEAD 6.94 and 6.95 ms, new 6.96 ms (+0.01 ms).

## Open
- toad_pond catalog run on build 1511 (not run: budget). Run it after the orchestrator resets LOGTRIM.
- PRIMZFIX on the console: the user checks the pond water (diag_primz.md). The repro switch does not show the bug in Azahar.
- WARPLOG2 on the console: count lines per warp (target under 15). Azahar window is longer (pace lines), so the count differs from the console.
- diag_batend.md: wm and tk fields need a console run after a battle.
- test_catalog.md: the working tree still has TOADFOUNT's uncommitted deletion of the toad_water and toad_pond rows. Not committed by LOGTRIM (HEAD keeps the rows).
- map_restore_table.c: untouched, restored from .bak after each build.

## Next steps
1. Orchestrator resets LOGTRIM budget. Run toad_pond -Try on build 1511 and read the top shots.
2. Read the WARPLOG2 target on the console (push with logstat, count lines per warp).
3. Next epic (E3 PAUSEFIX) per docs/handoff/work_queue.md.

## Commands
- Build: `python 3ds/tools/gen_map_restore.py; PM_ROW=<row> [PM_LANE=<lane>] PM_BUILD_CAP=900 sh 3ds/build.sh`; then `cp 3ds/src/map_restore_table.c.bak 3ds/src/map_restore_table.c`.
- Test: `PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row <row> [-Build <lane>] [-Try]`.
- Self-test (no start): `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 -SelfTest`.
- Archive only: `sh 3ds/push_3ds.sh --archive-only --no-build`.
- Push: not given. The epic is not complete (see Open).

## Pass 2: WARPLOG2 line table and final build (build 1514, git 6ffb3df08)
- Commit 6ffb3df08 removes 7 noise lines: dspaudio bus (pm_dspaud.c:293, 3 lines), V09INIT (src/audio/bgm_player.c:708), warp scene f5 (pm_warp.c:741, the f==5 part), mem stack high-water (pm_mem.c:249) and mem gpu max per frame (pm_gpu_log_stats call, pm_mem.c:250).
- Yard warp window (entry line to warp scene f300), yard_pace log, build 1514: 23 lines. Before pass 2 (build 1513 log): 30 lines. HEAD (build 1504): 67 lines. Target under 15 is NOT met.

| # | Line prefix | Source | Kind | Decision and reason |
|---|---|---|---|---|
| 1 | warp: kmr_20 entry | pm_warp.c:220 | scene start | keep: first_marker of yard scenarios |
| 2 | game mode 7 frame | pm_map.c:552 | mode change | keep: attract detector (azt.ps1 attractRe reads game mode 2, 3, 17) |
| 3 | load map area | pm_map.c:572 | load fact | keep: run_azahar.ps1:273 marker, phase_report.py |
| 4 | mem map linear | pm_mem.c:263 | load fact | keep: perf_az_full.py reads it |
| 5 | vram acct map | pm_mem.c:267 | load fact | keep: VRAM accounting per load, fail counter |
| 6 | mem map vram free | pm_mem.c:271 | load fact | keep: vramfree min per load (HUDVRAM) |
| 7 | mem map heap | pm_mem.c:273 | load fact | keep: heap budget per load |
| 8 | sprite: player sprite set | src/sprite.c:823 | load fact (once per load) | keep: player sprite loaded for the map |
| 9 | bgm: SetMusic | src/evt/audio_api.c:97 | song start | keep: music change at the warp |
| 10 | spawn npcs | src/encounter.c:2804 | load fact (once per load) | keep: NPC count of the map |
| 11 | game mode 4 frame | pm_map.c:552 | mode change | keep (see 2) |
| 12 | warp: scene frame0 | pm_warp.c:469 | scene start | keep: run_azahar.ps1:273 marker |
| 13 to 18 | pace: wall | pm_main.c:305 | periodic, 1 per 5 s | keep: WARPLOG2 decision. Six lines because the Azahar window lasts about 33 s (5 s each). On the console the window is about 10 s, so about 2 lines |
| 19 | shot 0 frame 203 | pm_gpu.c:1458 | shot writer | keep: yard expected_events regex |
| 20 | logstat frames 300 | pm_log.c:252 | periodic, 1 per 300 frames | keep: yard_pace regex |
| 21 | logstat2 writer | pm_log.c:254 | periodic, 1 per 300 frames | keep: writer health (fbuf waits, drops) |
| 22 | fs1 skipped | pm_main.c:474 | periodic, 1 per 300 steps | keep: frame-skip count |
| 23 | warp: scene f300 | pm_warp.c:741 | periodic, 1 per 300 frames | keep: until of yard scenarios |

- Count: 23. Pace 6, load and memory facts 10 (lines 3 to 10), scene and mode 5, periodic stats 3, shot 1.
- Under 15 needs a user decision. Option A: pace every 10 s (Azahar window 3 pace lines, total 20). Option B: drop the load facts 4 to 10 (total 16 with option A not applied). Option A and B together give 13. Neither was applied.

## Catalog on build 1514 (LOGTRIM budget, 8 starts)
- toad_pond (-Try): 3 of 3 log checks. Top shot 0009 read: blue-teal pool at the left edge (x 20 to 60 of the rotated shot). The water fills its area. No background shows through the pool. Equal to the 1511 run outside the NPC strip (x 113 to 137).
- yard 3 of 3, pause_menu 6 of 6, mac01t 6 of 6, mac01 4 of 4, battle_start 4 of 4 (log checks).
- Not run (budget): partner_menu, file_select, text_menu. Build 1511 passed these 3 (7 of 7, 6 of 6, 3 of 3).
- Archive: 3ds/logs/builds/pm_3ds_1514_6ffb3df08.3dsx, sha256 9744a4c935f092ebbe5bf97b20b020d408ba2b23d0d9bfba0a509499d9ff1e3e (equal to build3ds/pm_3ds.3dsx). Line in pushed_builds.txt. Not pushed.

## Push line: withheld
- CLAUDE.md rule 6: no push command for a partial stage. The epic gate (warp under 15) is not met and 3 catalog scenarios are not run on build 1514. The push line is added when the user or the orchestrator accepts the gate.

## Console checks (user, after a push)
- Toad map pond at x -174 z 112: read the top shot and the water (PRIMZFIX, diag_primz.md).
- fsplit wm and tk fields: read the f600 line during the steady window. After a battle, the wm value tests diag_batend.md.
- Warp window line count: count lines from the entry to the scene f300 line on the console.

## Verification LOGTRIM3 (build 1514, git 6ffb3df08, archive pm_3ds_1514_6ffb3df08.3dsx)
- Starts: 3 (one per scenario, PM_ROW=LOGTRIM3). No rebuild, no source edits.
- partner_menu: log checks 7 of 7 pass. Top shot 0000 (frame 200) shows the Party tab with Goombario. Pass.
- file_select: log checks 6 of 6 pass. Top shot 0000 (frame 100) shows the file select screen (File 1 "First Play", Select file to start). Pass.
- text_menu: log checks 3 of 3 pass. Top shot 0000 (frame 170) shows the pause status page with the dialog "Check here to see what Mario's current status is." Pass.
- Failing checks: none. No rerun against pm_3ds_1504_c294e993f.3dsx was needed.
- Catalog result on build 1514: 9 of 9 scenarios run (yard, pause_menu, mac01t, mac01, battle_start, toad_pond, partner_menu, file_select, text_menu) with log checks passing.
- Logs: 3ds/logs/perf/LOGTRIM3_partner_menu.txt, LOGTRIM3_file_select.txt, LOGTRIM3_text_menu.txt.
