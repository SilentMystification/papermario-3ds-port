# Design: the long pole after ws_C, ws_D and ws_ENTRY (oracle, 2026-11-14, main 8fbb0c1ae)

## 0. Numbers
- FACT (`python 3ds/tools/listcen.py`, 8fbb0c1ae): C1 build 2008, C2 gfxpos 1438, C5 data 22103, C6 data_arrays 2108, C7 gfxtype 2845, C8 reader 746,
  C9 model 15, C10 recdl 359. C1 in include/ 464 (gbi.h 382, gbi_custom.h 61): S9DEL4D deletes it. C5: src/entity 8112, assets/us 8053, src/world 3029,
  src/model.c 516, include 384, src/battle 359, src/pause 293, level_up* 416, starpoint* 199 (FACT, same run, `listcen group` lines).
- FACT (`codemod.py --dry --state src/*.c`, 8fbb0c1ae): root value 350, keep 74, other state 344, dl 54.
- FACT (scratch census of active src without src/world/dead, comments stripped, 8fbb0c1ae): calls that MAP, STATE_MAP or ws_GEO cover: 996 on
  gMainGfxPos++, 101 on a local packet. Calls no map covers: 357 on gMainGfxPos++, 53 local (section 2).
## 1. Items 1, 2 and 4: one setter per macro (ws_GEO, closed page). Choices and reasons
- Colour image: the call-site form is the set form nd_tgt_cimg(fmt, siz, width, img), not nd_target_begin(role, slot)/end. Reason: img_role decides
  the role per draw from the image AND other mode (nd_target.c:31-44: cycle type, Z_CMP/Z_UPD, VISCVG for fills), so a role at the call site is wrong
  for a later draw with another mode. The one writer of the target state is set_cimg (nd_target.c:100-108, from nd_walk.c:344). NTGT needs no Sonnet
  core: the sites move in ws_GEO. The later body change (S9DEL3) keeps the readers: img_role at nd_comb.c:212 and :245, nd_direct.c:84, nd_geom.c:197
  and :219, nd_rect.c:236 and :313-314, nd_set.c:211 and :239, nd_shape.c:713 and :766, off_screen_check nd_target.c:88-98. NdPsoDesc.zimg
  (nd_api.h:32) already carries the target's part of the decision.
- Depth image: removed. G_SETZIMG is a NOP unit (nd_op.h:114) and nd_gx_w drops it (nd_rec.c:457). The z buffer is pm_gpu's own.
- Vertex and triangles: slot calls nd_mesh_vtx(v, n, v0), nd_mesh_tri1, nd_mesh_tri2. Reason: the run-time sites load into slots other than 0
  (star.c:257, hieroglyphs.c:160, sleep_bubble.c:162-168, imgfx.c:1754-1757, whirlwind.c:253, nok_04/npc.c:315) and in loops with triangle runs, and
  a load transforms with the matrix of that moment. An absolute-index nd_mesh(vtx, n, idx, ni) cannot express that. The absolute form stays the form
  of the static tables (NDN_DRAW, nd_gtab.h:88, resolved by nd_vslot.c). Plan rows NXF2-4 become ws_GEO G0-G3.
- Lights: nd_lit_set1/2(name) (7 + 1 sites, all root files), nd_lit_lookat (lil_oink.c:184), with nd_xf_pnorm (6) and nd_xf_clip (3).
- Local-packet sites stay (codemod keep first-arg): cam_main.c:220, battle/16C8E0.c:868, crystal_ball_gfx.c:145-152, hud_element.c:610-612,
  animator.c:963 and :973. They build a list in a buffer. Choice: each becomes direct setter calls at the place that calls its buffer, one row per
  file after ws_GEO G3 and ws_TEX merge (the setters exist then). Reason: the buffer exists only to be called once in the same frame.
## 2. The biggest C1 rest: texture, rectangle, viewport and colour calls (page ws_TEX.md, written 2026-11-14: header-only nd_tx.h macros)
- FACT (scratch census above): gDPSetTile 43, gDPLoadTextureTile 42, gSPTextureRectangle 38, gDPFillRectangle 26, gSPViewport 24,
  gDPLoadTextureTile_4b 24, gDPLoadTLUT_pal16 21, gSPScisTextureRectangle 20, gDPSetTextureImage 16, gDPLoadSync 16, gDPLoadTile 14,
  gDPSetFillColor 10, 30 more names with 8 or fewer. Idiom: design_state_calls.md table rows (nd_tex_img .. nd_tex_load_tlut16, nd_rect_tex,
  nd_rect_scis, nd_rect_fill, nd_xf_viewport, nd_val_fill/fog/blend; syncs removed). Files: new nd_tx.h, nd_tx.c (ws_C owns nd_st.* until R6).
  Same form as ws_GEO: G0 tool group now (no blocker), call-site groups after the owner merges. The oracle writes ws_TEX next.
## 3. CONV2C2: closed page ws_CONV2C2.md (oracle 2026-11-14; start at `Merge EMIT4`, same converter file; Haiku, 2 groups)
- Choice: a list of ntab_inherit with site pairs (gen_ntab_entries.py on src/effects: list, ND_PSO_DESC of its nd_draw_list) converts once per distinct desc,
  seeded with v2_seed (cw0..plod, has_comb, has_tex, cycle bits), table ndn_<list>_e<id>, NdNTab.entry = id + 1. One device entry per list (units do not
  depend on the seed). Seeded names go to ntab_pairs.txt, never to ntab_tables.txt (codemod --ntab gives ND_SELF to those). The host model seeds from
  ndn_desc[entry - 1]; the seeded fault is key_c ^ 1 in the model (no convertible list sets key_c: RARE is unsup), so every seeded table must fail.
- Device effect: nd_ntab_find finds these lists, so gt_body, walk_call (E3), the E2 skip and the EMIT4 flat path take their units. Gate fxpic_91_drec, gold_12_drec.
- FACT (oracle scratch scan, 2026-11-13 ntab files): 185 site lists, 182 of them inherit; about 203 pairs (GUESS). Callee-only inheriting lists (about 240,
  GUESS) keep no entry: their seed is the caller state at the call unit (v2_walk knows it). That is the next converter step after GKEY (section 5).

## 4. NEMIT: a v2 list reads no Gfx word on the device (blocker of GSET, C5, C6, C12; Haiku, no Sonnet core; oracle 2026-11-14)
- Choice: GSET needs only that the device takes every word of a moved list from its table. The NdNOp runner that replaces the walker (NDN_PSO ..
  NDN_DRAW into nd_native_submit) is the reader removal of plan section 6 phase E (C8), not a GSET blocker. Same words, so each step is draw-equal.
- ws_EMIT.md (closed page): E1 the device index carries the unit copy ndnu and nd_ntab_run walks it (nd_walk.c:594), E3 walk_call runs a v2 callee
  from it (:245), E2 a v2 list with no triangle skips ent_gdl. Then a list with ntri 0 whose callees are free reads no Gfx word (the notri count, E1).
- E4 = ws_EMIT4.md (closed page, after `Merge EMIT`). Choice: the entity path takes a v2 list's words through NdNDev.u (nd_ntab_flat in nd_gtab.c,
  ent_flat_call in nd_ent.c), the hash of a v2 list covers only its vertex span, and its gaps run as ent_sdef on k->fw (no v1 table pieces).
  Reasons: the converter cannot write a hash (the words hold link addresses, nd_gfx_conv.c:569); a converter flat copy costs about 75 KB
  (ndnu 74.5 KB, FACT pm_3ds.map) against the 310 KB margin; no game file writes a static Gfx array (grep). EMIT4 keeps the NOPs in ndnu, so
  v2_nu == n and the host test proves the flat words from ndnu. GSET then sets u = ndnu_<list>, nu = v2_nu: no device reader of the list is left on
  the entity path. A list whose callee has no device entry (host line `ntab flat ... words <n>`) keeps its words until the callee gets an entry.
## 5. GSET: the Gfx arrays leave the device (pages ws_GKEY1, ws_GKEY2, ws_GSET1 written 2026-11-14, oracle 7; chain EMIT4 -> CONV2C2 -> GKEY1 -> GKEY2 -> GSET1)
- Key: a moved list X has no device symbol. Its key is &ndd_X (`const NdNDev ndd_X`, aligned 8 because ent_gdl and rec_inline refuse a key with a & 7: nd_ent.c:417,
  nd_rec.c:391). ndd_X.list = &ndd_X, u = ndnu_X, nu = v2_nk. An unmoved list keeps key X, u = X. NDN(X) = `((Gfx*)&ndd_X)`: handles keep their Gfx* type, so
  initializers (blast.c:25) and nd_draw_list calls need no type edit. NDN_DECL(X) replaces `extern Gfx X[];`.
- GKEY1: standalone ndd_<list>, pointer index (sort moves pointers), field n -> fl (the state-only flag: gt_state_only read words on core 1, nd_gtab.c:102).
- GKEY2: every device read of a list with an entry takes nt->u: nd_rec_dl records nd_ntab_rec (key nt->list), walk_call depth 0 runs nd_ops_run(nt->u, nt->nu, 1)
  (nd_walk_state stops at the same unit, nd_walk.c:365-382), rec_inline takes the entry words with the end at nu, nd_lstcen skips entries. Both indices sort in
  nd_tab_init before pm_lg_start (pm_main.c:539): core 1 now looks up too. Draw-equal before any move.
- GSET1: an effect list is an asset include line (water_splash.c:6), so a move is that line going into the twin 3ds/tools/gfxin/<path>. The converter knows
  moved symbols from gen_gfx_syms (twin text), hides their v1 table, idx line and NdNTab behind ND_HOST_GFX (the host test keeps the v1 model), writes
  call units into moved callees as &ndd_<callee>, and writes gset_ok.txt: entry, not NDG_F_PARTS (the particle run needs v1, nd_rec.c:281-288), every callee
  with an entry (else the ent path reads words: EMIT4 ent_flat_w), every caller ok (an unmoved caller keeps a reference). listcen skips assets that only
  twins include (listcen.py:249 counts all assets). Pilot fright_jar (fxpic_91_drec draws it).
- Next GSET rows (oracle 9, 2026-11-14): ws_GSET2 (effects rest, groups a and b by gconv_src.d rows, after `Merge GSET1`), ws_GSET3 (src/entity/model, gset.py block
  mode: a `Gfx X[] = {` .. `};` block goes to the twin, NDN_DECL(X) stays; after `Merge GSET2`). World, model.c render modes, battle, pause, level_up and starpoint
  lists are not in the converter set (gconv_src.d has only effects and S7ENT). Choice for world (13 files with `Gfx N(x)[]`, FACT grep): gen_gfx_syms expands N(x) to
  <NAMESPACE>_x from the map header's `#define NAMESPACE` (include/macros.h:45), and a world source joins as its own row GCONVW. Page GCONVW after `Merge GSET3`
  (not closed now: the host compile of a world TU is not tried yet; that is its first group). Map shapes: GSET-MAP (plan section 7).
## 6. Discoveries (for plan_endgame.md section 7)
- listcen counts src/world/dead (3 files, build 30, gfxpos 32: FACT listcen), but maps.cmake builds no file of it. Choice: listcen.py skips
  src/world/dead after ws_D and ws_GEO G2 merge. Reason: no build or drec can prove an edit there.
- Plan rows to change: NTGT is no Sonnet core (section 1). NXF2-4 are ws_GEO. NEMIT is ws_EMIT plus E4 (Haiku), before GSET1-6.
- The 25 world, entity and battle recdl sites are nd_scomp_rec defs (S6E2, S6M6, S5BTL1), not list entries: entcap never sees them
  (it logs nd_gtab_run lists only, nd_walk.c:501-505). Choice: each def becomes setter calls in unit order (ws_SCOMP.md, 18 sites). The 7 sites of
  mac_01/crystal_ball_gfx.c: ws_CBALL.md (closed; nd_lit_lookat for the X/Y pair, new nd_tex_hilite1/2 in nd_tx.h; after `Merge GEO2`).
- recdl effects: FXSELF merged (85 sites, Merge 1149b5513). 42 `nd_rec_dl(` lines stay in src/effects (FACT grep), 13 of them multi (FACT Merge ENTRYFX). Cause of the
  missed captures (code read): entcap_rec skips any list with a v2 table (nd_walk.c:506; CONV2C2 gave inheriting lists with a pair one entry), and a list with no NdGTab
  never reaches nd_gtab_run. ws_FXENT2 (after `Merge NSTREAM2`): filter by stated entry, an `entcap nt` line for no-table lists, recapture, entapply, then
  build3ds/out/fxent2_rest.txt classes (multi, nt, seen, unseen) feed the hand rows.
- recdl root 15: ws_RECROOT (after `Merge TEX4`; the ws_SCOMP rule, value units take the g macro arguments of v[k]). It owns sprite.c until merged (NS3 waits).
## 7. Phase E deletions (S9DEL): readers C8 756 per file and the trigger of each stage (FACT listcen 3eba999c4; no stage can start today)
- Blocker for stages 1D and 3: NSTREAM (section 8, Haiku pages ws_NSTREAM1, ws_NSTREAM2; NS3 page at its trigger). FACT (listcen a37368582): C1 1166, C2 617,
  C10 145, C5 22040, C6 2075, C8 757.
- Order 1 -> 2 -> 5 -> 3 -> 4 (plan section 6). Per stage: trigger, files (reader hits), grep terms that must return nothing after the stage.
  1 runners. Trigger C1 0, C2 0, C10 0 (1A-1C need no NSTREAM step; 1D: section 8). A nd_rec.c 80, nd_recs.c 48, nd_model.c 33, nd_shape.c 12, pm_phase.c 1; B nd_ent.c 24; C nd_dlc.c 27;
    D nd_walk.c 81, nd_cen.c 42 (census of list words). Terms: `nd_walk_state|nd_ops_run|walk_call|nd_range_run|ENT_GBI|ent_flat|ent_run|ent_compile|dlc_try|dlc_build|nd_lstcen`.
  2 state model. Trigger stage 1 merged, NPSO3 table lookup on the device, NTGT. nd_set.c 39, nd_direct.c 11, nd_tex_set.c 7, nd_mtx_set.c 3, model hits C9 15
    (nd_comb.c 3, nd_tex_set.c 3, nd_decide.c 2, nd_rect.c 2, nd_texwin.c 2, nd_geom.c 1, nd_mtx_set.c 1, nd_set.c 1). Terms: `sync_combine|nd_comb_decide|sync_render|set_tilesize|ensure_tile|img_role|off_screen_check|gv_tri`.
  5 static list parse. Trigger C6 0 and GSET-MAP merged. nd_gtab.c 6, nd_seam.c 9 (GUESS: shape seams), nd_shape.c list parse, gen_gfx_tab.sh v1 output. Terms: `NdGTab|gt_ops|ndg_|NDG_`.
  3 typed stream. Trigger stages 1, 2, 5 merged. nd_op.h 173, nd_op.c 62, nd_scomp.c 49, nd_st.c 13, nd_gbi.h 9, nd_val.c 7, nd_tx.h 6, nd_xf.c 6, nd_geo.c 4,
    nd_cmd.h 2, nd.h 1, nd_api.h 1. Terms: `NDO_|NDC_STATE|nd_rec_state|nd_scomp_exec|op_typed|nd_op_apply`.
  4 Gfx type. Trigger C5 0 and stage 3 merged. C7 2845, gbi.h, gbi_custom.h, gs2dex.h, gt.h. Terms: plan section 4 grep.
- Gate of every stage row: build rc 0, nd_host PASS, the terms return nothing in 3ds/src 3ds/include src include, elfgate func falls, drec byte-equal
  (gold_12_drec, fxpic_91_drec, spot_mac01_drec, sbk14_drec). Rows: one file or one symbol family; callers first, the definition last.
## 8. NSTREAM: the native record stream (oracle 8, 2026-11-14, main a37368582). Pages ws_NSTREAM1 (start now), ws_NSTREAM2 (after `Merge GKEY2`, `Merge NSTREAM1`)
- Record form (choice): NdNr is 8 bytes, k = kind << 24 | a (24 bits), v = b. The kinds are the NDO_* ids and the fields keep the HC4 unit layout bit for bit
  (nd_op.h:11-51). Reasons: (1) every NSTREAM step is byte-equal by construction, and the host units prove it by memcmp; (2) the core 0 state model reads these
  fields (nd_set.c:159-184, nd_tex_set.c nd_set_*), and stage 2 replaces that model, so a new packing now edits code that stage 2 deletes; (3) none of the 757
  readers needs an edit for NSTREAM. A pso-level record (NdPsoDesc id per draw) is not equal by construction: that is stage 2 (below, not Haiku).
- What NSTREAM removes: NS1 the gbi word build and the ND_X0 translation in the producers (st_rec nd_st.c:13, geo_rec nd_geo.c:13, nd_tx_rec nd_tx.h:14); NS2 the
  NDC_STATE command (nd_cmd.h:19, st_put nd_rec.c:342, stream_run :578, nd_r11_walk nd_model.c:285) and the list executor name on record runs; NS3 the gbi macros
  of the core 0 local buffers. Stage 3 renames NDO_ to NDR_ and deletes ND_X0, ND_X1, nd_op_xlate and nd_op_n64: that part of the C8 fall is a rename for the kept
  kinds, and its commit says so.
- Kinds (a, b; writer; core 0 handler). State: OMH, OML (sft | len << 8, value; nd_pso_rm..combkey, som, zsrc, pipe; set_om nd_set.c:83), GEOM (kept bits, set bits;
  nd_pso_geom_*; set_geom :32), COMB (cw0 low 24, cw1; nd_pso_comb, cc; op_setcombine :175), TEXTURE (on, s << 16 | t; nd_tex_on; nd_set_texture :172), SCISSOR
  (10.2 corners; nd_pso_scis, scisf; set_scissor :46), RARE (KEYR, KEYGB, CONV; set_rare :148), PRIMZ (z << 16 | dz; set_primdepth :64), MW (kind << 16 | arg:
  NUMLIGHT, FOG, SEG, NONE for pnorm and clip; set_mw :107). Colour: PRIM (minlod << 8 | frac, rgba), ENV, FOG, BLEND, FILL (nd_val_*; :74, :99, :43, :71, :69).
  Bind: TIMG, TILE, TILESIZE, LOADBLOCK, LOADTILE, LOADTLUT (nd_op.h:29-34; nd_tex_*, nd_val_tile_size; nd_set_* nd_set.c:176-181). Its stage 2 form is NdBindDesc
  (nd_api.h:54-63). Transform: MTX (0x38 << 16 | flags, Mtx*), POPMTX (64), MOVEMEM (len, offset, index: light, lookat, viewport; nd_xf_*, nd_lit_*; load_mtx,
  gbi_popmtx, gbi_movemem nd_walk.c:291-296). Mesh: VTX (n << 12 | (v0 + n) << 1, Vtx*), TRI1, TRI2, MISC MODVTX (nd_mesh_*; nd_geom_vtx, draw_tri nd_walk.c:281-290).
  Target and draw: MISC CIMG (nd_tgt_cimg; gbi_setcimg), RECT fill (nd_rect_fill, sfill; nd_fillrect), MISC ND (G_PM_ND; nd_native_rec), MISC LST (state-only lists).
  List only, deleted at stage 1D: CALL (b = list key), END, DROP (t4chk), the two RDPHALF units after a texture RECT inside a list.
- Reused records: NdParts (its units are NdNr; NS2 runs pre and post with nd_nr_exec), NdTRect and NdFillRec (NDC_NAPI already; their fallback buffers go in NS3),
  NDC_TEXRECT (nd_rect_tex, scis: unchanged). FACT (grep 3ds src include): nd_rect, nd_quad, nd_glyphs and nd_mesh do not exist (plan_nativeapi.md:20-21 names
  only). NSTREAM adds none: a mesh is VTX and TRI records (slot loads, section 1).
- Writers and the step that changes each: setters nd_st.c, nd_geo.c, nd_val.c, nd_xf.c, nd_tx.h (NS1). Recorder st_put nd_rec.c:327-349, nd_rec_state :366,
  rec_inline :390-403, nd_rec_list :516-544, nd_parts_add :302, nd_scomp.c:149 (NS2). Core 0 built units nd_walk.c:553, :558, :606 (NS2), pm_gbi_fallback :754-764
  (stage 1D). Core 0 local buffers hud_element.c:565, :616, msg_draw.c:1766, screen_overlays.c:244, sprite.c:354, nd_direct.c:263, :332 (NS3). Converter ndnu
  nd_gfx_conv.c:568 (no edit: same bits). Static Gfx arrays (GSET). The gbi redirect nd_gx_w nd_rec.c:442 (C1 and C2 to 0).
- Readers: no edit (same bits). After NS2 the record runs enter by nd_nr_run (stream) and nd_nr_exec (parts, gt_ops, ent pieces, scomp defs, ntab units, LST).
  nd_walk_state keeps the list runs only: walk_call :247, nd_ops_run :409, gt_call :540, the debug call :616, and the NS3 buffers until NS3. The stage of each
  reader file: section 7.
- NS3 (choice; page at its trigger): the nd_tx.h and nd_st.h shapes get an emit argument. Body `NDTX_<NAME>(O, ...)` writes with `O(k, v)`; `nd_tex_load_tile(...)` is
  `NDTX_LOAD_TILE(nd_nr, ...)`; the core 0 form `nd_tex_load_tile_at(...)` is `NDTX_LOAD_TILE(ND_NR_AT, ...)`, `ND_NR_AT(k, v)` writes `nrp[0] = k; nrp[1] = v; nrp += 2`
  into the site's local `uint32_t* nrp`; the site calls nd_nr_exec(buf, n). Reason: core 1 records while core 0 runs these runners, so one shared sink pointer
  races. Host test: each _at shape against the gbi macro on a local packet (nd_gx_w writes the typed words there, nd_rec.c:443). Moves C1 by the local packet
  calls of the 5 files.
- Triggers. NS1: now. NS2: `Merge GKEY2` and `Merge NSTREAM1`. NS3: `Merge TEX4` (root files) and `Merge NSTREAM2`. Stage 1D: C1 0, C2 0, C10 0, stages 1A-1C
  merged, GSET done (C5 0, C6 0: every CALL unit names a table with an entry), NS3 merged. Then nd_nr_exec takes the arms of nd_walk_state minus END, branch, DROP
  and op_unknown, its CALL arm runs the callee entry (the list start of nd_ops_run, then nd_nr_exec(nt->u, nt->nu)), and nd_walk_state goes. Stage 3: stage 1D,
  C5 0 (no gs macro uses ND_X0), C10 0 (nd_scomp gone).
## 9. Stage 2 core: the pso table lookup (oracle 9, 2026-11-14, main 1149b5513). Page ws_PSOLK (Haiku, start now)
- Choice: a content lookup, not an id per draw. sync_combine_work (nd_comb.c:280) and then nd_comb_decide (:258) ask nd_pso_find with the walker's NdCombIn. A hit
  takes the table's NdCombOut plus nd_pso_fix (leg 4: mode5 and box_pat of the walker; cc from the walker colours; dual). Equal by construction: nd_comb_core reads
  prim and env only for cc, leg_mode never, box_pat and mode5_prev only on the leg 4 return (nd_decide.c:91-126, :294-308). The table is built with SSE floats
  (x87 extended precision can flip `use`, nd_decide.c:273). Device proof: under psochk each hit also computes and counts lkbad.
- Next steps (pages after Merge PSOLK): PSOCAP (each miss logs its walker NdPsoDesc once, a tool writes them as ND_PSO_DESC lines of a new committed file, so the
  table covers the gate scenes; miss falls to 0 there), then the same lookup for the render state (nd_render_state is pure: sync_render). S9DEL2A deletes the decide
  code. The one part only the user decides: what a draw does with a state that has no table entry once the decide code is gone (a fixed fallback mode and a log
  line, or keep nd_comb_core as a cold fallback on the device).
