# diag_toadt: producer ranking, mac_01 (Azahar, direction only)

Epic TOADT, rows T0, T1, T2b. Lane toadt. Census fields: mdlsum rows (ops, draws, tris, binds, closes, all per frame).
Views: warp (mac01, window f299, 300 frames) and tour (mac01t, window f899 = frames 600 to 899, all on mac_01).

## Tour view, total 2315 ops per frame (mdlsum f899)
| rank | producer (function or bucket) | ops/frame | share | draws | closes | tex binds | tris |
|---|---|---|---|---|---|---|---|
| 1 | appendGfx_entity_model (src/entity_model.c:244) | 795 | 34.3 % | 30 | 0 | 0 | 146 |
| 2 | appendGfx_npc, incl. sprite_component (src/npc.c:835) | 490 | 21.2 % | 27 | 2 | 0 | 54 |
| 3 | task tail after last task (0xF0FE, not a function) | 332 | 14.3 % | 0 | 19 | 0 | 0 |
| 4 | model 0006 (census ID) | 37 | 1.6 % | 0 | 1 | 6 | 0 |
| 5 | appendGfx_player (src/77480.c) | 20 | 0.9 % | 1 | 0 | 0 | 2 |
| 6 | model 0049 | 9 | 0.4 % | 0 | 0 | 1 | 0 |

- Other rows in the top 12: 9 to 8 ops each (models 0043, 0048, 0064, 0098, 0044, 0066), 0.3 % to 0.4 % each.
- Unattributed: 62 ops per frame (2.7 %). Models in window: 89, static 79.
- Task other (sum of all function rows) is about 1308 ops per frame (56 %). It has 58 draws and 202 tris in total.
- sprite_component is not a task pointer. It runs inside appendGfx_npc and appendGfx_player.
  appendGfx_npc: 490 ops / 27 draws = 18.1 ops per draw. Non-empty closes: 2 per frame / 27 draws = 0.07 per draw.
  Closes for the whole function, not only sprite draws. Sprite split needs its own mark (T2 row).
- appendGfx_entity_model: 795 ops / 30 draws = 26.5 ops per draw, 0 closes, 0 binds. Cost is op count, not closes.

## Warp view, total 1165 ops per frame (mdlsum f299, scenario mac01)
- appendGfx_entity_model 271 ops (23.3 %, 10 draws). appendGfx_npc 80 (6.9 %, 4 draws, 1 close).
- appendGfx_item_entity (src/item_entity.c, 0x00225890) 69 (5.9 %, 2 draws).
- Tail 0xF0FE 95 (8.2 %, 0 closes). Model 0006 32 (2.7 %, 5 binds, 4 closes).

## Top 3 by share and the first change to try
1. appendGfx_entity_model, 34.3 % (tour). Ops per draw 26.5, no closes. First: add a per-op-class count for this function (G_MTX, G_VTX, G_TRI, other) under phase. Then change: skip a repeated gSPMatrix when the shadow matrix is equal (hw_guidelines rule 6), at the gSPMatrix calls near src/entity_model.c:269.
2. appendGfx_npc, 21.2 % (tour). Ops per draw 18.1, closes per draw 0.07. First: split sprite_component draws from the rest of appendGfx_npc (T2 mark in src/sprite.c spr_appendGfx_component). Change only after T2 shows sprite ops per draw above 10.
3. Task tail (0xF0FE), 14.3 %, not a function. It holds 19 closes per frame, 19 of the 22 closes in the top rows. First: give the ops after the last task loop their own mark by caller (__builtin_return_address in execute_render_tasks callers) to name the owner. The change follows the name.

## Checks
- T0 gate (mac01): total 1165, PPM byte-equal to build3ds/azt/shots_toadbase/mac01 (T2b run, 2 shots). Model 00b7 out of top 12.
- T2b gate (mac01): total 1165 unchanged. PPM byte-equal to the baseline.
- T2b gate (mac01t): total 2315 at f899, same as T1. Gate 1900 to 2200 still fails (above limit). Not a pass.
- Azahar only. Console share for these functions is not measured. Run the T2b build on console (phase on) before a change is judged.
