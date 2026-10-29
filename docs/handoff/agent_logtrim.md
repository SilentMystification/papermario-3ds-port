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
