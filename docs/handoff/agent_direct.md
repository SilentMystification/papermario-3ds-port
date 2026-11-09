# Handoff DIRECT (epic: the game calls the native producers directly)

STATE
- DIRECT completion runs as row HC4 (worktree build3ds/wt/HC4, branch HC4; design plan_hotcode.md section 5, row_HC4.md). Main 2aff8e99b merged.
- Done: HC4a ff92aa293 typed scalar state ops. HC4b 49a27798e: NDO_COMB/NDO_END/NDO_DROP; render mode table nd_rm_tab (nd_rec.c, built at the first record of an index), rec_apply runs it, nd_mode_compile reads typed ops; record prim/env/fog/blend/shroud typed; recorder shadow sh_drop/sh_word (nd_rec.c) with switch t4chk.
- Gates HC4b vs stored base 2aff8e99b: nd_host PASS; spot_mac01_det 6/6, battle_start_f182_cold 1/1, mdl5_iwa10_det 6/6, spot_mac00_det_pic 6/6 byte-equal; hc4_t4chk_mac01: f200 drops 3298 bad 0 (about 21 of 740 state ops per frame); hotset M1 hot 112.7 -> 112.5 KB, A 22.5 KB.
- Next: HC4c texture + R1/R2 (deletes tex_op, tb_ops, the op_* wrappers; nd_aux_state and nd_r13_texonly then read typed R13 ops), HC4d matrix/geometry + R3-R10 (deletes g2_same, word cases, rec_dl, row_DIRECT.md), HC4e typed gs* macros (then ModelRenderModes is a typed static table: nd_rm_tab goes), HC4f shape converter, HC5, HC6.
- Transition rule now: word lists (static, until HC4e) pass nd_op_xlate on core 0 per op; R13 words are typed on core 0 in nd_aux_state until HC4c.
- hot_order.ld: 5 stale names; regenerate (PM_FNCOV=1 lane, 3 fncov benches, hot_order.py) only after HC4d and HC4f. fncov M3 not run yet.
- Base shots for cmp: build3ds/basecache/2aff8e99b... (spot_mac01_det, battle_start_f182_cold, mdl5_iwa10_det, spot_mac00_det_pic). The spot_mac00_det scenario ingests under bench spot_mac00_det_pic: base and cmp that name (base spot_mac00_det builds and runs but stores nothing).
