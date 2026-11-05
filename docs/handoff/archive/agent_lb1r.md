# Handoff: LB1R epic (LB1B, POSHUD, FINAL9)

## State
- LB1B: not passed. Gate incomplete: mac01 and pause_menu not run (start budget 6 of 6 used). Tree undone to HEAD. Patch kept: 3ds/logs/perf/lb1_dropped.diff (applies to HEAD, checked with git apply --check). Not committed.
- POSHUD: committed fd05e686a.
- FINAL9: mdlsum gate commit a42603c92. Archive 3ds/logs/builds/pm_3ds_final9_fd05e686a.3dsx. Pushed: no.
- Working tree: 3ds/src/map_restore_table.c (user file, restored from .bak after each build; never committed). Other modified files belong to the TOADFOUNT agent.

## LB1B (lazy FrameBegin retry)
- HEAD lane: build3ds/lane-lbhead = final8 binary (3ds/logs/builds/pm_3ds_final8_f230e46c6.3dsx). Source HEAD equals f230e46c6 for 3ds/src and 3ds/include.
- HEAD against HEAD, battle_start (runs A and B): 0000 to 0003 equal. 0004 and 0005 differ. 0006 and 0007 exist in run A only (run B wrote 6 shots). Frames 0004 to 0007 are not a valid gate.
- Patched (lane lb1b) against HEAD, battle_start: 0000 to 0003 equal to both HEAD runs (4 of 4 valid). 5 shots written.
- Patched against HEAD, mac01t_fsplit: 0000_1_2 and 0001_1_2 equal (2 of 2).
- fsplit gwait: 0.0 ms in the HEAD run and in the patched run. Azahar does not show the wait, so this gate cannot show a drop here.
- Patched against yard_headA (HEAD e16fee4b9 proof run, not f230e46c6): 0000_0_11 equal (1 of 1).
- Not run: mac01, pause_menu (budget). No frame that HEAD reproduces differs, so the GPU-reader cause check was not needed.
- Starts: 6 of 6 (battle_start HEAD A, HEAD B, lb1b; mac01t_fsplit lb1b, HEAD; yard lb1b).
- Next: a retry needs 2 starts for mac01 and pause_menu on lane lbhead and lane lb1b (budget reset by the orchestrator), then the gate above.

## POSHUD (Position HUD option)
- Switch: options.ini poshud=0|1, Debug tab row 8 "Position HUD" (Select menu and file select panel share the tab table), or debug string poshud.
- Code: 3ds/include/pm_port.h (PmOpt.pos_hud, pm_pos_text), 3ds/src/pm_options.c (key, panel row, label), 3ds/src/pm_main.c (HUDINT refresh), 3ds/src/pm_route.c (pm_pos_text).
- Values: M = gPlayerStatus.pos and targetYaw. C = gCameras[CAM_DEFAULT].lookAt_eye and curYaw. While on, rows 0 and 1 of the bottom memory block show these; row 2 keeps memory.
- Test: yard_poshud, 3 of 3 log checks. Bottom shot bot_0259: M x 594 y 30 z -216 yaw 180; C x 924 y 161 z 60 yaw 310. Log at f300: pos 594 30 -216. Top shot 0000_0_11 byte-equal to yard_headA.

## FINAL9
- mac01t mdlsum gate: 1175 to 1495 (1335 +/- 12 %). Commit a42603c92 (azt_scenarios.json, test_catalog.md).
- Starts 6 of 6. Not run: partner_menu and text_menu (the catalog asks 8 starts; budget 6).
- Results: yard 3 of 3 log checks. pause_menu 6 of 6 (walk 11.5 ms, closes 135, ops 1237). mac01 4 of 4. mac01t 6 of 6 (mdlsum 1335). battle_start 4 of 4. file_select 6 of 6 (walk 8.3 ms, closes 88, ops 1346).
- Top shots read by eye: yard, pause status page, file select screen, mac01 post office with Mario and Toads, mac01t Toad Town street with HUD, battle_start battle HUD in 0004 to 0007. battle_start 0000 is a colour test pattern; 0001 to 0003 show the field.
- Archive: 3ds/logs/builds/pm_3ds_final9_fd05e686a.3dsx, 13371296 bytes, sha256 d0c7cd712e9d4e2959ad564c088f006fb321d92d7a6f575f7d1aa09b5a952c43, build 1490 (797 + 3865 - 3172). Checked equal to build3ds/lane-final9/pm_3ds.3dsx.

## Open
- docs/handoff/test_catalog.md is staged in the index by the TOADFOUNT agent. The staged blob holds the old mac01t gate text (1623 to 2065) and changes the toad_water row. Their commit would undo the FINAL9 gate text. Re-stage from HEAD before that commit.
- The working copy of test_catalog.md lost the POSHUD section once (another writer). HEAD has it.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=<row> PM_LANE=<lane> PM_BUILD_CAP=900 sh 3ds/build.sh; cp 3ds/src/map_restore_table.c.bak 3ds/src/map_restore_table.c.
- Test: PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build <lane> [-Try].
- Push (user runs): PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.170 pacestat logstat dcsum fsplit loglevel=debug
