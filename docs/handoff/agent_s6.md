# Agent S6: entities, items, animators draw from records natively

## STATE
S6-1 PASS (2026-11-06, row S6, main folder, nds1). Record and check for entity models (G_PM_ENT, 4 words) committed. No draw change: the dispatch skips the record, the old words draw.
Next: S6-2 (native entity draw) not started. Named open items below.

## S6-1 (done)
- include/gbi_custom.h: G_PM_ENT 0x11, G_PM_ENT_WORDS 4. Record words: w0 = G_PM_ENT << 24 | renderMode << 16 | fog bit; w1 = model->gfx.displayList. word 1: segment id (D_80154374) and physical vertex array (0 when none). word 2: G_MTX flags and the matrixStack entry address. word 3: w1 = render mode list (the G_DL the switch emitted; 0 when none).
- src/entity_model.c appendGfx_entity_model (TARGET_3DS, not the USE_IMAGE path): the record is written before the matrix op, word 3 is set after the render mode switch, entcheck_draw runs after the model DL and pop.
- entcheck_draw (debug switch `entcheck`): per draw, searches the walker words after the record for the matrix op, the segment op (only with a vertex array), the model DL (G_DL with the list pointer) and the render mode list (G_DL or G_PM_RM with its w1). Field codes: 1 matrix, 2 segment, 3 model DL, 4 render mode list. Line every 300 frames: `entcheck f<n> recs <n> eq <n> diff <n> dfield <n>` and `entdf` by field.
- 3ds/src/pm_gbi.c gbi_run_body: case G_PM_ENT skips G_PM_ENT_WORDS - 1 words.
- Scenarios entcheck_mac01 and entcheck_kmr03 (azt_scenarios.json) and benches (bench_defs.json, default false).
- Gates: entcheck_mac01 eq 2041 diff 0 dfield 0 (f300). entcheck_kmr03 eq 4302 diff 0 dfield 0 (f300). Pictures viewed (right by eye): mac_01 Toad Town (star sign, post office, Mario and NPCs), kmr_03 (Mario, fence, coin block at left).
- Budget S6: 2 Azahar starts of 8 used (both entcheck). Build: 2276 (mac_01 run), 3DS build md5 map table 437b90ef unchanged.
- Deviation (plan default): the check is a word search on the walker's own stream, not a native key compare. The native key (list, segment base) of S6-2 is the first real compare; S6-1 only proves the record fields are the words the walker sends.
- Not done in S6-1: NdObj count per map (the row asked it logged). Open: S6-2 needs the native key first.

## Open (named, not done)
- S6-2 native entity draw: the record arm must draw the model DL natively (nd_dlc static compile keyed on (list, segment base), nd_native_submit). Classes to name when hit: dynamic vertex array (vertexArray != nullptr, the B and D draws: vertex data is CPU side, not static, class `dyn_vtx`), fog (entity_fog_enabled, class `fog`), USE_IMAGE (separate path, not in record), reflect (second draw, class `reflect`), render mode DL refusal (class `rm_list`).
- S6-3 shadows and item entities: not started.
- S6-4 animators: not started.
