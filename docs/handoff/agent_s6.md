# Agent S6: entities, items, animators draw from records natively

## STATE
S6-2b PASS (2026-11-06, row S6b): vertex segment static check (entcheck only). mac_01 f300: ndobj line: vtx draws static 255, dyn 0, new 1; keys static 1, dyn 0, ovf 0. Gate entcheck diff 0 and texchk diff 0 held. No draw change.
Next: S6-2c (native switch of entity draws). Not started.

## S6-2b (done, row S6b)
- entity_model.c entcheck_vtxseg: per draw with a vertex array, key (list, array). The G_VTX range (off + n * 16 on the vertex segment) hashed once per frame. Equal on two frames: static (sticky dynamic). Line ndobj f<n> vtx draws static/dyn/new, keys static/dyn, ovf.
- Deviation: NdObj compile not run. nd_dlc_compile refuses texture ops (NDR_OP), so the entity list needs the state-op split first (S6-2c). The keys count is the candidate NdObj count per map.
- Budget S6 after reset: 3 starts (retry 5 diagnostic, gate run, S6-2b run).

## S6-2a (done, row S6b)
- nd.h: NdTexDec, nd_dlc_tex_decode (codes NDTX_NONE, NDTX_OK, NDTX_TIMG, NDTX_TILE, NDTX_MULTI, NDTX_WIN, NDTX_PAL, NDTX_NOLB, NDTX_NOBIND). nd_dlc.c: the decode, walker rules of the form entity lists use: SETTIMG, SETTILE tile 0 and 7, LOADTILE tile 7 (the walker ld_valid form, with the 4 bit window in bytes: ld_w * 2) or LOADBLOCK on tile 7, SETTILESIZE tile 0 (origin 0, height above 1), LOADTLUT tile 7 for CI (palette bank of tile 0).
- entity_model.c: census splits tex into texok (decode NDTX_OK). Under entcheck only: end record G_PM_ENT with G_PM_ENT_END (gbi_custom.h, w0 bit 8), written after the model DL.
- pm_gbi.c: pm_gbi_ent_texchk at the end record: walker td[0] (src, fmt, siz, w, h, stride, origin, pal and pal_n for CI) against the decode. Line texchk f<n> ent <n> eq <n> diff <n> nat <n> fb <codes 2 to 8>. Diagnostics texbad, texwin.
- Deviation: the row says compare unit words (nd_tex_get words vs the walker slot). The compare is on the pm_tex_load arguments (td[0]), which fix the unit words; nd_tex_get is not called in the check (it loads textures, a side effect).
- Found (census): every entity texture list uses LOADTILE tile 7 (gsDPLoadTextureBlock here loads with G_LOADTILE), not LOADBLOCK. The 4 bit form counts the window in bytes (ld_w = w / 2).
- Gate run: entcheck_mac01 (bench tex_eq 3543, tex_diff 0). Budget S6: 8 starts of 8 used before the window fix (S6-2a retry 1 to 4 and 5 were FAILED shots of the decode: TILE, TILE, WIN). Budget reset once (run_azahar -ResetBudget S6, 2026-11-06). Starts after the reset: 2 (retry 5 diagnostic, gate run).
- Not checked: picture (no draw change; the check runs in entcheck builds only).

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

## S6-2c to S6-4 (not done, named fallbacks, row S6b)
- ent_native_switch (S6-2c): the native entity draw needs an entity record producer for nd_model (the S3N path is map records with the texture handle desc from the header, not from the list). Open parts: (1) texture: NdTexDec of the list (texok 1793 of 1793 at mac_01 f300) to a handle-style unit set (nd_tex_get words) per draw; (2) state: the list state ops (SETCOMBINE, SETOTHERMODE, GEOMETRYMODE, TEXTURE, render mode list) are not split out of the list (nd_dlc_compile refuses them, NDR_OP); (3) geometry: keys static 1 at mac_01 (255 draws), so the static part is one object; (4) combine and fog per entity (entity_fog_enabled, PM_CC_42 path) need the S3N B10 combine decide; (5) the walker word compare (entcheck eq) must be extended to the native draw before the old words go.
- Dynamic vertex class (ent_dynvtx): not seen at mac_01 (dyn 0); keep as named fallback for other maps.
- reflect (second draw), USE_IMAGE (separate path, not in record), and render mode list refusal (rm_list): fallback to the walker, count when hit.
- S6-3 (shadows, item entities): blocked on S6-2c (same switch).
- S6-4 (animators, src/animator.c appendGfx_animator, vertex buffer per frame via animator_copy_vertices_to_buffer): dynamic vertices (ent_dynvtx) by design; battle_start picture not checked. Blocked on S6-2c and on a dynamic vertex path.
- Status: S6-2a and S6-2b passed (commits cae261f8e, 1f31155b2). S6-2c, S6-3, S6-4 INCOMPLETE: no entity draw is native, no picture change.
