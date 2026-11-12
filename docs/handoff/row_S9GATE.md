# Row S9GATE: proof that no display-list code is left (the end-state gate of NATIVEAPI)

(Model: Haiku. Runs tools only: no code edit. Why not smaller: none.)

## Blocker and base
S9DEL4 (all worktrees) merged. `sh 3ds/tools/wt_new.sh S9GATE`; PM_ROW=S9GATE PM_LANE=S9GATE.

## Steps
1. `python 3ds/tools/bench.py run listcen --row S9GATE --why "end-state gate"`: all 7 totals are 0 (build, gfxpos, data,
   data_arrays, gfxtype, reader, model).
2. `grep -rnE "\bg[DS]P[A-Za-z]|\bgs[DS]P[A-Za-z]|\bGfx\b|gMainGfxPos|NDO_|nd_ops_run|nd_walk_state|NDC_STATE|sync_combine|set_tilesize" src include 3ds/src 3ds/include > build3ds/out/s9grep.txt`:
   the file is empty.
3. Build, memcheck OK; `bench.py run nd_host`: PASS.
4. Byte set, with bases at the merge-base. One list: spot_mac01_det, mdl5_iwa10_det, spot_mac00_det, battle_wheel_nowide_det,
   pause_tab_det_pic, pause_spin_grab_det_w300, fxpic_63, the talk det scene and the file select det scene (9 starts: the budget
   is 8, so the orchestrator splits the list into two lists of this row, or resets once as a planned step). Every shot
   byte-equal.
5. hotset_size on the lane: hand back M1 per class.

## Console (the orchestrator)
CONSOLE C5: prof push `mac_01 0 0 200`, plus pause, battle and file select, compared with the stored prof run, plus PMU (pmu_i,
cycles). The user judges speed and look.

## Stop
Any non-zero count, a non-empty grep, or a byte difference: stop, hand back the output (the row that owns that piece is named in
row_ORACLE_NEXT.md). At a full pass, delete this row file in one commit: the NATIVEAPI chain is done.
