# agent_tex: texture quadrant bug (A) and file select rotation bug (B)

State: merged dlc-compiler-c1c2 into branch worktree-agent-a4d0f7fc8fe8139f7 (clean merge). Build inputs copied: ver/us/build, assets/us (copied into assets/us, not assets/). Lane build PM_LANE=tex OK (build3ds/lane-tex/pm_3ds.3dsx, build 1170-tex). No code change. No commit for A or B.

Budget: tex row 1 of 8 used.

## Bug A: texture draws 4 times, top 2 quadrants garbled (Mario House, SUPER text on multibounce)

Blocker: scene not reached yet with a picture that shows the bug.

- Run 1 (no timeout, no until-line, 16 s): preset=yard, shots shotwin=120:6, warp kmr_20 entry 4. Shots 0000 to 0005 written to build3ds/lane-tex/shots/*.ppm. Shot 0005 (game frame 125) read by eye after PIL convert (scratchpad yard_0005.png, crop yard_0005_crop.png). Mario and partner sprites at the house steps look correct. Not a pass and not a fail: the subject frame is not in the window.
- Fault: run 1 ended on "smoke done", not on an until-line. The coordinator rule requires an until-line for each run and a 90 s cap.
- Missing: the Mario House scene (intro house, hos_05 / kmr_20 night) is not warped to yet. The demo table in src/state_demo.c holds attract scenes only.

Next: find the until-line for the house scene (warp or preset that puts Mario on the house steps), take one window on it, read the picture, then log per bind SETTIMG dims, SETTILESIZE dims, stride and GPU dims (the value the draw used).

Update (runs 2 and 3, budget 3 of 8 used):
- Run 2: preset=intro, shotwin=60:40. until "warp: kmr_20" did not match (no such log line on the intro path). Shots show the intro house exterior and Toad, no quadrant pattern.
- Run 3: preset=intro, until "warp: scene f200", shotwin=210:40 (exit 0). Shots 0000 to 0039 show the daytime house with Mario walking in front. Shot 0005, crop origin (40,220) at 8x: an orange and white block near full-res (81,264) beside Toad. It may be a garbled Mario sprite with the top half wrong. Not confirmed.
- Missing: a per-bind log line with SETTIMG and SETTILESIZE dims for the draw at that position.
- Multibounce SUPER text: not in run 2 or run 3 shots. Likely source src/battle/move/jump/multibounce.c. Not reached.
- Bug B: golden.py has a fileselect scene (3ds/tools/golden.py), a candidate warp path. Not run yet.
- Run 4 (budget 4 of 8, commit eabc25f0f): imglog now prints "tilesize" (SETTILESIZE window, SETTIMG fmt/siz/addr/stride) and "gpubind" (slot w/h/stride, GPU w/h, data address). Frames 212 to 213 of the intro window: no slot is half or double its GPU size in both axes. The 296x6 strip uses GPU 512x8 (padding only). Suspicious: tile 0 windows with uls 1024 ult 1024 (texel origin 256,256) on 32x32 and 48x56 slots (GPU 32x32, 32x64, 64x64). The origin is outside the slot image. Not matched to the candidate sprite yet. Next: map the candidate (full-res 81,264) to one of these binds, then compare its origin with the slot size.
- Run 5 (budget 5 of 8): ensure_tile wrap of tile_x0/tile_y0 into the slot size (bug A lead). Shot 0005 (frame 215): the orange and white block beside Toad is GONE, and Mario is not visible there. Not a pass. Edit reverted in the worktree (pm_gbi.c untouched, not committed).
- Run 6 (budget 6 of 8): file select attempt, 30 s cap (turbo divides the timeout). Frame 154 only, logo transition.
- Run 7 (budget 7 of 8): same, 30 s cap. Not reached.
- Run 8 (budget 8 of 8, -Turbo -TurboFactor 1, 90 s cap): file select reached. Shots 0000 to 0059 = frames 330 to 389. Shot 0015 (frame 345), upright: the file select shows "File 1 AAAAm000" with the text overlapping the name strip, and "First Play" box. A rotating box glitch is not confirmed in the 60 frames (no rotation seen at this scale). Not logged per draw.
- Budget is spent (8 of 8). Bug A: no fix. Bug B: no fix. Next agent: start from run 8 window (-Turbo -TurboFactor 1 -Timeout 90, shotwin=330:60 with the fileselect padscript) and add a draw log for the rotating strips.

## Bug B: file select UI glitch during rotation (draw_box DRAW_FLAG_ROTSCALE into VRAM, then strips)

Blocker: not started. No test run. Next: warp to the file select (title, then start), until-line on the file select state log, then shots while the box rotates.

## Commands used

- Build: PM_LANE=tex sh 3ds/build.sh (from the worktree root). Generated headers come from assets/us (the build copies the worktree).
- Run: PM_ROW=tex PM_LANE=tex powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Timeout 90 -Debug "fast preset=yard shots shotwin=120:6" (add an until-line per run; the 90 s cap applies).
- Map restore table: python 3ds/tools/gen_map_restore.py is run before each build. 3ds/src/map_restore_table.c is not committed.
