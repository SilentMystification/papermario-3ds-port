# Handoff DIRECT (epic: the game calls the native producers directly)

STATE
- DIRECT completion runs as row HC4 (worktree build3ds/wt/HC4, branch HC4; design plan_hotcode.md section 5, row_HC4.md). Main 09dd42691 merged.
- Done: HC4a scalar ops, HC4b combine and dedupe, HC4c texture ops, HC4d1 geometry ops (7dd9a9f08). HC4d2 b0f5ca3a3: control ops NDO_CALL/RECT/MISC (op_ctl, noinline; op_geo noinline again); nd_walk_state runs typed ops only (a static list word is typed by nd_op_xlate first); W5 sprite geo typed by sprite.c; W6 NDO_CALL in the NDC_ENT payload (ent_draw, nd_r11_op read it). R8 is dead code; R4 moves to HC4f, R5/R6/R10/pm_gbi_fallback to HC4e (plan 5.7 table).
- Gates HC4d2: nd_host PASS; vs base 808deabb4 spot_mac01_det 6/6, battle_start_f182_cold 1/1; DIRECT set vs base 09dd42691: mdl5_iwa10_det 6/6, nok11_det 31/31, gameover_fps60 40/40 byte-equal. Not run (budget): tour10_det, spot_mac00_det_pic.
- hotset M1 hot 111.8 -> 110.9 KB, class A 19.3 -> 11.6 KB (nd_walk_state 1868 -> 364 B; op_ctl 1620 B cold path). Target A <= 4 KB: set_tilesize 2360, op_setcombine 1292, tex_tile1 1188, nd_op_xlate 912 are the big A members.
- Base shots: build3ds/basecache/808deabb4 (spot_mac01_det, battle_start_f182_cold), 09dd42691 (mdl5_iwa10_det, nok11_det, gameover_fps60). spot_mac00_det base: bench name spot_mac00_det_pic.
- Open at the end of HC4d: hot_order.ld regenerate (PM_FNCOV=1 lane, 3 fncov benches, hot_order.py; 15 stale names) and fncov M3: not run (Azahar budget spent after one reset).
- Next: HC4e (S1: typed gs* and local g* forms; ent, dlc, sprite rmList, R5, R6, R10 read typed ops; the core 0 nd_op_xlate call goes), HC4f, HC5, HC6.
