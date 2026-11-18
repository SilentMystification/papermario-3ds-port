# ws_NST2JB (Haiku): bisect the NST2J crash in two halves (measure only; no merge)
Designer: oracle FIN. Worktree build3ds/wt/NST2JB, branch NST2JB from NST2J a41cc4d4d4. PM_ROW=NST2JB PM_LANE=NST2JB. Edit only what the steps name. Never merge this branch.
## Facts (code read of `git diff 762d995639 a41cc4d4d4`, and the probe NST2JP b9a4183e33)
- The probe (base plus `uint8_t sc_pad[2]` in NdDs: the same .data move) runs spot_mac01_drec with no crash. So the .data move alone is not the cause.
- NST2J crash (FACT, lane-NST2J/last_crash.txt): RotateModel (map_api.c:58) writes through a NULL Model. get_model_list_index_from_tree_index returns 0 when no
  model matches (model.c:3083), so wModelList holds no model of mac_01.
- The code read finds no write of NST2J outside nd_ds and the dschk counters. The diff has 2 groups that run without the dschk switch:
  group 1: the 5 feed calls nd_ds_feed -> nd_ds_feed_dir (nd_direct.c:285, :367, hud_element.c:564, :596, screen_overlays.c:131);
  group 2: the ds_sync call in nd_ds_apply (nd_ds.c) and its sc_lost writes. The other edits (nd_rec_scl removal, dschk) change no game data.
## Steps (output to build3ds/out/, read the last 20 lines; 2 builds, 2 Azahar starts)
1. `git worktree add build3ds/wt/NST2JB -b NST2JB a41cc4d4d4`. All next commands run in build3ds/wt/NST2JB.
2. Half A (group 1 out): `git checkout 762d995639 -- src/hud_element.c src/screen_overlays.c 3ds/src/nd_direct.c`. Commit `NST2JB A: NST2J without the direct feed calls`.
   `PM_LANE=NST2JB PM_ROW=NST2JB sh 3ds/build.sh > build3ds/out/nst2jb_a_build.txt`. Then
   `python 3ds/tools/bench.py run spot_mac01_drec --row NST2JB --lane NST2JB --why "NST2J bisect half A" > build3ds/out/nst2jb_a_run.txt`.
   Copy build3ds/lane-NST2JB/last_crash.txt (when it exists) to build3ds/out/nst2jb_a_crash.txt.
3. Half B (group 2 out): `git checkout a41cc4d4d4 -- src/hud_element.c src/screen_overlays.c 3ds/src/nd_direct.c`. In 3ds/src/nd_ds.c nd_ds_apply:
   Before: `    ds_sync(s, k, v, ch, h0, du0);` After: `    if (0) ds_sync(s, k, v, ch, h0, du0); /* NST2JB B: no sync state */`.
   Commit `NST2JB B: NST2J without the ds_sync call`. Build and run as step 2 with the names nst2jb_b_*.txt and the why "NST2J bisect half B".
4. Commit the 4 or 6 output files. Do not change the NST2J branch.
## Result table for the hand-back (one line per half: build number, run rc, crash yes or no, the first 3 crash PCs)
- A no crash, B crash: the cause is a feed call. B no crash, A crash: the cause is ds_sync. Both crash: the cause is neither group alone. Both pass: the two together.
Fix rights: compile errors of steps 2-3 only. A tool fault (build rc not 0, a stale build, no run line): stop, send the output path to the oracle.
Stop: after step 4. One SubagentHandback with the result table and the output paths. The orchestrator removes the branch and the worktree.
