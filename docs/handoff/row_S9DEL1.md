# Row S9DEL1: delete the per-frame list runners (nd_walk_state, nd_ops_run, ENT_GBI, the nd_dlc run-time list compile)

Model: Sonnet (user decides).
- Why not Haiku: the runners are called from many places (grep below). Each caller must already be on a native record; the
  coder proves that and deletes the caller arm.
- Why not Opus: deletion.

## Step 0 (the orchestrator, before the coder starts)
Tag main: `git tag pre-list-removal <main hash>`. Recovery of the last tree with list code is then `git checkout pre-list-removal`.
The coder checks that the tag exists (`git tag -l pre-list-removal`). If it is missing, stop and hand back.

## Blocker and base
S8FB merged. `sh 3ds/tools/wt_new.sh S9DEL1`; PM_ROW=S9DEL1 PM_LANE=S9DEL1.

## Readers and writers (grep first; list the hits in the commit)
`grep -n "nd_walk_state\|nd_ops_run\|ENT_GBI\|dlc_try\|nd_dlc" 3ds/src 3ds/include`:
- nd_walk.c (nd_walk_state, nd_parts_run, nd_range_run, walk_call);
- nd_ent.c (ENT_GBI);
- nd_rec.c;
- nd_recs.c;
- nd_model.c;
- nd_shape.c;
- pm_phase.c;
- nd_dlc.c (the whole file, if S7MAP and S7ENT moved its users).

## Edit
Delete each function and its callers' arms. A caller that still has a list path left is a fault of an earlier row: stop and hand
back the caller (do not move it here). Remove the deleted files from 3ds/CMakeLists.txt.

## Gate (expected values)
- Full build, memcheck OK; nd_host PASS (remove the host tests of deleted code, and list them).
- DIRECT byte set, battle_wheel_nowide_det, pause_tab_det_pic, the talk det scene: every shot byte-equal (bases at the
  merge-base).
- The grep above returns nothing. listcen reader down. Budget 8.

## Stop
A byte difference or a remaining caller: stop, commit, hand back. Delete this row file in the completing commit.
