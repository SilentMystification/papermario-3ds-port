# agent_draw: DRAW epic (E5), rows DC6, SPK1, DRAWFIN

## State (HEAD 542f2d72d, build 1526)
- DC6 (1) and (2): done. Commit 2d41644a1. Counters only. cmd field fixed (GPUCMD offset). dcgrp line added. Picture: top shot byte-equal to HEAD.
- DC6 (3): dropped. Gate not met by any change: no repeated group remains. Shadows already compare. Palette same-content uploads 0.00 per close.
- SPK1: done. Commit 542f2d72d. Timing line `spk1` under fsplit. pm_mem_hud costs about 6 us Azahar (about 17 us console). Below 3 ms gate. Code unchanged in the HUD path.
- DRAWFIN: INCOMPLETE. 8 of 8 starts used. 8 scenarios pass their log checks: yard, pause_menu, partner_menu, file_select, text_menu, mac01, mac01t, battle_start. Not run: toad_pond, sewer, ruins, volcano, tour10, sun_sbk.
- Archive: 3ds/logs/builds/pm_3ds_1526_542f2d72d.3dsx (sha256 2ed9666b...f663). Line added to 3ds/logs/perf/pushed_builds.txt (uncommitted; the file held stale changes before).
- LB1b (E5 third row) not started: it runs only if gwait stays above 1 ms after LB1. The fsplit data for this is not in this epic.

## Open items
- Run DRAWFIN rest after the orchestrator resets PM_ROW=DRAWFIN: toad_pond (-Try, unverified), sewer, ruins, volcano, tour10, sun_sbk (-Try).
- Read the rest of the top shots: mac01 0001, mac01t 0001, battle_start 0004 (frame 182 battle shot).
- SPK1 f300 over50 comparison: HEAD value not kept. Rerun yard_fsplit on lane dchead and spk1 if needed.
- Bottom screen shots are not stable run to run on yard_fsplit (HEAD about 670 px, SPK1 about 5000 px). Use log values for the HUD gate.
- Stale uncommitted catalog changes (mac01t_mr0, toad_water, toad_pond rows) are still in the working tree. Not committed by this agent.

## Commands
- Build (default lane): `python 3ds/tools/gen_map_restore.py`; `PM_ROW=DRAWFIN sh 3ds/build.sh`; restore 3ds/src/map_restore_table.c from the user copy (never committed).
- Archive: `PM_ROW=DRAWFIN sh 3ds/push_3ds.sh --archive-only`.
- Scenario: `PM_ROW=DRAWFIN powershell -File 3ds/tools/azt.ps1 <scenario> -Row DRAWFIN [-Try]`.
- Lane HEAD reference used: lane dchead, built in a detached worktree of fb9e8f0fc (not in the main tree).
- Starts: DC6 7 of 8, SPK1 4 of 8, DRAWFIN 8 of 8 (INCOMPLETE).

## DRAWV: verification of the six DRAWFIN scenarios (build 1526, git 542f2d72d)

- Row DRAWV, budget reset once. Starts used: 6 (one per scenario). Build: build3ds/pm_3ds.3dsx, sha256 2ed9666b...f663, equal to the 1526 archive.
- Log checks: toad_pond 3 of 3 (-Try), sewer 3 of 3, ruins 3 of 3, volcano 3 of 3, tour10 4 of 4 (tour: done, 10 maps), sun_sbk 3 of 3 (-Try). No failing check. No numeric gate in these six scenarios.
- Top shot read by agent (0000 unless noted). A person must confirm.
  - toad_pond 0000: Toad Town plaza, brown pool rim, no water in this frame. 0009: teal water at the left edge (x 20 to 60), Mario and Toads at the pool. Teal, not blue.
  - sewer 0000: Toad Town Tunnels, Mario at a brick wall with yellow ? blocks.
  - ruins 0000: Dry Dry Ruins, Mario and a Goomba-type sprite, story text box covers the right side.
  - volcano 0000: Mt. Lavalava cave, Mario, lava glow in the centre.
  - tour10 0010 (last of 11 shots): Mario beside grass and a white wall. Consistent with mac_00, the list end map. Not confirmed by eye.
  - sun_sbk 0000: Dry Dry Desert, sand dune with Mario, blue sky at the right, faint diagonal ray streaks.
- Rerun against pm_3ds_1520_af0529787.3dsx: not needed. No check failed.
- Open: the picture gate for toad_pond needs a person to confirm the pool. The water is in the last shot only.
- Logs: 3ds/logs/perf/drawv_<scenario>.txt. Shots: build3ds/azt/shots/<scenario>/.
