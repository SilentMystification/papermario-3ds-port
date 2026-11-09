# Row TIK19: water and wall textures flicker in tik_19 when Mario moves

## User facts (bug, 2026-11-08, console O3DS XL, wide mode; builds from main of today)
"in tik_19, if you walk left and right between -120 -10 -80 and 424 -10 -80 (left and right) the water will flicker. Walking up and down further to the right near the super block causes the wall textures to flicker."
So the flicker follows camera/Mario movement, not time. Reproducing scenario: add one (warp=tik_19 + pos=-120:-10:-80, padscript walking right to x 424 and back; second scenario near the super block walking up/down; shot bursts every frame or every 2 frames during the walk; until-line a log line at the end of the walk). Flicker = a draw present in one frame and missing or different in the next.

## Goal
Trace by code first (CLAUDE.md rule 5). Candidates to check (not conclusions):
- Model cache hit by visible-group mask (PERF1 4b: pm_dlc_vis_mask / entry vmask, nd_model.c / pm_gpu_model_hit): a group that becomes visible while moving may be skipped or drawn from a stale entry for a frame.
- dlc_box_out culling of map groups (nd_shape.c) with the camera moving: boxes wrong in wide mode (wide FOV) or with a stale matrix, so a group is culled when it is on screen.
- Texture panner / scroll (tik water uses panners; STARFX found panner key word w20 inserting keys each frame in hos_01): a key churn plus mc_clear_all can drop draws for a frame.
- Depth: water as a decal or prim depth (B2 nd_prim_clip_z) fighting the floor; wall decals.
Name the failing draw (file, line, value) with the counts (misses, clears, culled groups per frame; Azahar counts are valid), then fix.

## Start state
Worktree build3ds/wt/tik19, branch tik19 from main (current). Copy assets/us and ver/us/build into it. MAPX (branch mapx) changes the model key rule (colour words, same-rp replace): merge main at each step boundary.

## Done criteria
1. Trace in the handback.
2. Build; nd_host PASS.
3. Picture gate: the shot bursts of both walks show the water and the walls in every frame (no frame with a missing or different draw), wide mode on.
4. The 4 byte gates (spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015) byte-equal unless the trace shows they use the changed path (then name the pixels).
Affected scenarios (no others): the two tik_19 scenarios, the 4 byte gates.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=tik19 PM_ROW=tik19 on every build.sh and bench call. No git stash. No toggles, no stubs. 2 tries per named cause. Scenario and bench in the same commit.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=tik19 PM_LANE=tik19 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=tik19 PM_LANE=tik19 python 3ds/tools/bench.py run <bench> --row tik19 --lane tik19 --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "tik19 ready <hash>".
