# Agent S6: entities, items, animators draw from records natively

## STATE
S6-1 PASS (2026-11-06, row S6, main folder, nds1). S6-2 to S6-4 not done: the S6-2 census shows every entity draw uses a texture op in its DL (named class tex, see below). Record and check for entity models (G_PM_ENT, 4 words) committed. No draw change: the dispatch skips the record, the old words draw.
Next: S6-2 (native entity draw) not started. Named open items below.

## S6-1 (done)
- include/gbi_custom.h: G_PM_ENT 0x11, G_PM_ENT_WORDS 4. Record words: w0 = G_PM_ENT << 24 | renderMode << 16 | fog bit; w1 = model->gfx.displayList. word 1: segment id (D_80154374) and physical vertex array (0 when none). word 2: G_MTX flags and the matrixStack entry address. word 3: w1 = render mode list (the G_DL the switch emitted; 0 when none).
- src/entity_model.c appendGfx_entity_model (TARGET_3DS, not the USE_IMAGE path): the record is written before the matrix op, word 3 is set after the render mode switch, entcheck_draw runs after the model DL and pop.
- entcheck_draw (debug switch `entcheck`): per draw, searches the walker words after the record for the matrix op, the segment op (only with a vertex array), the model DL (G_DL with the list pointer) and the render mode list (G_DL or G_PM_RM with its w1). Field codes: 1 matrix, 2 segment, 3 model DL, 4 render mode list. Line every 300 frames: `entcheck f<n> recs <n> eq <n> diff <n> dfield <n>` and `entdf` by field.
- 3ds/src/pm_gbi.c gbi_run_body: case G_PM_ENT skips G_PM_ENT_WORDS - 1 words.
- Scenarios entcheck_mac01 and entcheck_kmr03 (azt_scenarios.json) and benches (bench_defs.json, default false).
- Gates: entcheck_mac01 eq 2041 diff 0 dfield 0 (f300). entcheck_kmr03 eq 4302 diff 0 dfield 0 (f300). Pictures viewed (right by eye): mac_01 Toad Town (star sign, post office, Mario and NPCs), kmr_03 (Mario, fence, coin block at left).
- Budget S6 (S6-1 gates): 2 Azahar starts of 8 used (both entcheck). Build: 2276 (mac_01 run), 3DS build md5 map table 437b90ef unchanged.
- Deviation (plan default): the check is a word search on the walker's own stream, not a native key compare. The native key (list, segment base) of S6-2 is the first real compare; S6-1 only proves the record fields are the words the walker sends.
- Not done in S6-1: NdObj count per map (the row asked it logged). Open: S6-2 needs the native key first.

## S6-2 census (diagnostic, entcls line, debug switch entcheck, commit with this note)
Priority per draw: vtxseg (model has a vertex array, the B and D draws), fog, reflect, tex (a texture op in the model DL: SETTIMG, LOADBLOCK, LOADTLUT, SETTILE, SETTILESIZE, LOADTILE, TEXTURE), nested (G_DL in the DL), other (op outside the native set), static.
- entcheck_mac01 f300 (window 1, 2049 draws): static 0, vtxseg 256, fog 0, reflect 0, tex 1793, nested 0, other 0.
- entcheck_kmr03 f300 (window 1, 4319 draws): static 0, vtxseg 2033, fog 0, reflect 0, tex 2286, nested 0, other 0.
- Result: no entity draw is static. Every draw is vtxseg or tex. The S6-2 switch (native draw of the model DL, nd_dlc static compile) cannot draw any entity draw as the DL is now. The tex class is the whole problem (100 percent of draws without a vertex array). Named blockers:
  - class `tex`: the texture load words sit inside the model DL (not in a record). Native needs the texture handle desc (S3N B4 form) from those words (SETTIMG address, TLUT, tile state). Same work as S3L-3 aux and the R13 forms, per entity model. Plan default: decode the DL texture block into a handle desc (new), not done.
  - class `vtxseg`: the B and D draws use a CPU-side vertex array (dynamic, not static, the DLC static path refuses). Plan default: keep walker for these (named fallback), native only the static part. The vtxseg draw still has tex (both classes overlap in the census), so this alone does not unlock S6-2 either.
- Not done: S6-2 switch (G_PM_RM rewrite delete, native entity draw), S6-3 (shadows, item entities), S6-4 (animators). Each one depends on the tex class. No picture change from S6-1 (the record is skipped by the dispatch).
- Budget S6: 4 Azahar starts of 8 (entcheck_mac01 x2, entcheck_kmr03 x2). No reset.

## Open (named, not done)
- S6-2 native entity draw: the record arm must draw the model DL natively (nd_dlc static compile keyed on (list, segment base), nd_native_submit). Classes to name when hit: dynamic vertex array (vertexArray != nullptr, the B and D draws: vertex data is CPU side, not static, class `dyn_vtx`), fog (entity_fog_enabled, class `fog`), USE_IMAGE (separate path, not in record), reflect (second draw, class `reflect`), render mode DL refusal (class `rm_list`).
- S6-3 shadows and item entities: not started.
- S6-4 animators: not started.
