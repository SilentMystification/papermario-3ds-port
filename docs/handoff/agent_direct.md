# Handoff DIRECT (epic: the game calls the native producers directly)

STATE
- DIRECT completion runs as row HC4 (worktree build3ds/wt/HC4, branch HC4; design plan_hotcode.md section 5, row_HC4.md). Main 13144da1c merged.
- Done: HC4a ff92aa293 typed scalar state ops (nd_op.h ids 0x20+, nd_op.c nd_op_xlate on the recorder, nd_set.c nd_op_same/nd_op_apply, op_typed in nd_walk_state); g2_note, gbi_moveword and the scalar word cases deleted.
- Gates HC4a: nd_host PASS; byte-equal vs stored base 13144da1c (bench.py base/cmp): spot_mac01_det 6/6, battle_start_f182_cold 1/1; hotset A 22.8 -> 22.5 KB.
- Next: HC4b (T_COMB, render mode table W3, recorder shadow dedupe + t4chk), HC4c texture + R1/R2, HC4d matrix/geometry + R3-R10 (deletes g2_same, word cases, row_DIRECT.md), HC4e typed gs* macros, HC4f shape converter, HC5, HC6.
- Transition rule now: word lists (static, until HC4e) pass nd_op_xlate on core 0 per op; nd_r11_op maps typed othermode back to words (nd_op_om_word) for the native model path until HC4b.
- hot_order.ld: 5 stale names after HC4a; regenerate (PM_FNCOV=1 lane, 3 fncov benches, hot_order.py) only after HC4d and HC4f. fncov M3 not run yet.
- Base shots for cmp: build3ds/basecache/13144da1c... (spot_mac01_det, battle_start_f182_cold); a new main merge needs new base shots (bench.py base).
