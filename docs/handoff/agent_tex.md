# agent_tex: texture quadrant bug (A) and file select rotation bug (B)

State: merged dlc-compiler-c1c2 into branch worktree-agent-a4d0f7fc8fe8139f7 (clean merge). Build inputs copied: ver/us/build, assets/us (copied into assets/us, not assets/). Lane build PM_LANE=tex OK (build3ds/lane-tex/pm_3ds.3dsx, build 1170-tex). No code change. No commit for A or B.

Budget: tex row 1 of 8 used.

## Bug A: texture draws 4 times, top 2 quadrants garbled (Mario House, SUPER text on multibounce)

Blocker: scene not reached yet with a picture that shows the bug.

- Run 1 (no timeout, no until-line, 16 s): preset=yard, shots shotwin=120:6, warp kmr_20 entry 4. Shots 0000 to 0005 written to build3ds/lane-tex/shots/*.ppm. Shot 0005 (game frame 125) read by eye after PIL convert (scratchpad yard_0005.png, crop yard_0005_crop.png). Mario and partner sprites at the house steps look correct. Not a pass and not a fail: the subject frame is not in the window.
- Fault: run 1 ended on "smoke done", not on an until-line. The coordinator rule requires an until-line for each run and a 90 s cap.
- Missing: the Mario House scene (intro house, hos_05 / kmr_20 night) is not warped to yet. The demo table in src/state_demo.c holds attract scenes only.

Next: find the until-line for the house scene (warp or preset that puts Mario on the house steps), take one window on it, read the picture, then log per bind SETTIMG dims, SETTILESIZE dims, stride and GPU dims (the value the draw used).

## Bug B: file select UI glitch during rotation (draw_box DRAW_FLAG_ROTSCALE into VRAM, then strips)

Blocker: not started. No test run. Next: warp to the file select (title, then start), until-line on the file select state log, then shots while the box rotates.

## Commands used

- Build: PM_LANE=tex sh 3ds/build.sh (from the worktree root). Generated headers come from assets/us (the build copies the worktree).
- Run: PM_ROW=tex PM_LANE=tex powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Timeout 90 -Debug "fast preset=yard shots shotwin=120:6" (add an until-line per run; the 90 s cap applies).
- Map restore table: python 3ds/tools/gen_map_restore.py is run before each build. 3ds/src/map_restore_table.c is not committed.
