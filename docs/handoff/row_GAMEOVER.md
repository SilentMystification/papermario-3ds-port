# Row GAMEOVER: game over screen flickers and is slow

## User facts (bug, 2026-11-08, console O3DS XL, build cand = main f6ebb9e58 + PERF2 steps 1-2)
"game over screen is flickery and laggy." Trigger: lose a battle; the game goes to the game over map gv_01 (area 26, src/world/area_gv/gv_01). The user plays with 60 fps mode ON (prof scene line "disp w0 s0 f60": wide off, stereo off, interp 60).
Console prof window at the game over (build3ds/hw_live_log_131.txt #1412-1418, f14641): fps 25.2, ft 39.9 ms, max 159.6 ms, o34 195 of 300; wk 24.1; gbi 29.1; cls rec 0.00 ent 4.40 spr 0.00 sub 4.01 rest 20.66; dr 210 draws, tri 515; gb (GPU) 14.3 ms; pmu I-miss 75,318, D-miss 21,256 per clean frame; aujoin_us 634 per frame, aujoin_max 75,237 us (one audio join waited 75 ms).
Reproducing scenario: add one (debug battle that Mario loses, or a direct warp to gv_01 with the game over state; 60 fps on and off; until-line a log line the game over script prints).

## Goal
Trace by code first (CLAUDE.md rule 5), name file, line and value for each:
1. FLICKER: what flickers (the whole screen, the "GAME OVER" text, the background). Check the 60 fps interp path (key frames vs in-between frames: does the game over draw a part only on key frames, or does a frame-buffer capture/copy alternate between two buffers?), and the off-screen/frame-buffer reads the game over effect uses.
2. LAG: 210 draws and rest 20.7 ms for a near-static screen: name what draws (likely many small rects/texrects of a background or text effect, each its own draw) and why rest is high; batch or cache them. Check the 75 ms audio join (aujoin_max): which audio job runs at the game over (music change, bank load) and why the game thread waits for it.
Fix each named cause. 60 fps mode must look right (no flicker) and 30 fps too.

## Start state
Worktree build3ds/wt/gameover, branch gameover from main (current). Copy assets/us and ver/us/build into it.

## Touch only
The paths the traces name. src/ changes under #ifdef TARGET_3DS. Do not edit src/msg.c.

## Done criteria
1. Traces in the handback.
2. Build; nd_host PASS.
3. Picture gate: a shot burst of the game over (60 fps on) shows no alternating frames (consecutive shots consistent), and the screen as on N64.
4. Draw count before/after in the game over scene (Azahar count is valid); timing direction only, the console decides.
5. The 4 byte gates (spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015) byte-equal unless the trace shows they use the changed path (then name the pixels).
Affected scenarios (no others): the new game over scenario (60 fps on and off), the 4 byte gates.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=gameover PM_ROW=gameover on every build.sh and bench call. No git stash. No toggles, no stubs. 2 tries per named cause. Scenario and bench in the same commit.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=gameover PM_LANE=gameover sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=gameover PM_LANE=gameover python 3ds/tools/bench.py run <bench> --row gameover --lane gameover --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "gameover ready <hash>".
