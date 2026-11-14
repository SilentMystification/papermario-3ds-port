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
## 3. CONV2C2: v2 tables for the inheriting lists (blocker: `Merge EMIT4`, same converter file; ENTRY E3 is merged; Haiku, 3 chunks)
- Writer 1, chunk C2a: new 3ds/tools/gen_ntab_entries.py <src dir> > <work>/ntab_entries.txt. Per `nd_draw_list(<arg>, <desc>)` with desc not
  ND_SELF: arg `\w+` gives one pair; arg `<arr>[...]` gives one pair per element of the initializer of `<arr>` in the same file; any other arg prints
  `gne skip <file>:<line>`. The 20 numbers of the desc come from the `ND_PSO_DESC(<desc>, ...)` text of the file (the entapply.py form, ws_ENTRY E2).
  Line: `<list> <desc> <file>:<line> <20 numbers>`. Test first: 3ds/tests/gne/run.sh (form of tests/codemod/run.sh), seeded fault.
- Writer 2, chunk C2b: nd_gfx_conv.c reads the file as a 6th argument (usage :664). New v2_seed(V2St*, const NdPsoDesc*): cw0, cw1, omh, oml, geom,
  key_c, key_s, conv_k4, tex_on from the desc. has_comb = 1, omh_set |= 3 << G_MDSFT_CYCLETYPE, has_tex = 1 (the three groups of
  v2_inherit_check, :369-380). A list of ntab_inherit with entries converts once per distinct desc into table ndn_<list>_<desc>, entry = v2_desc_id
  (:257). A seeded list that still inherits or hits v2_unsup stays in ntab_inherit.txt with the reason. Names go to a new ntab_pairs.txt, never to
  ntab_tables.txt (codemod --ntab gives ND_SELF to its names, codemod.py:232). ndn_dev_idx keeps one entry per list: the units do not depend on
  the entry. Host test first: 3ds/tests/host.d/conv2c2.sh, a fixture list that inherits comb and cycle; seeded fault (no cycle seed) must fail.
- Chunk C2c: gen_gfx_tab.sh:17-18 passes the file, :23 copies ntab_pairs.txt. Device effect: nd_ntab_find now finds these lists, so gt_body
  (nd_walk.c:596-600) runs the unit copy instead of gt_ops. Gate: CONV2E compare `bad 0` with the same seed in the v1 model, drec cmp
  gold_12_drec, gold_33/55/63_drec, fxpic_89_drec, fxpic_91_drec byte-equal. World and entity lists: after their entry page (capture scenes).
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
## 5. GSET: the Gfx arrays leave the device (blocker: NEMIT and CONV2C2 of the set; Haiku, one set per row)
- Tool (Haiku, test first): new 3ds/tools/gset.py <3ds/tools/gset.d/<set>.txt>. (1) Moves each `Gfx X[] = {...};` of a listed file into the
  host-only twin 3ds/tools/gfxin/<same path>, whose first line is `#include "<path>"` (Vtx and textures stay on the device). (2) Rewrites the uses:
  `nd_draw_list(X, e)` -> `nd_draw_tab(NDN(X), e)`, `extern Gfx X[];` -> `NDN_DECL(X);`, an initializer element X -> NDN(X). Any other use prints
  `gset keep <file>:<line>` (struct member types Gfx* -> const NdNTab*: hand edit in the same row). (3) gen_gfx_tab.sh takes GCONV_SRC from the
  gfxin twins of gset.d. The converter writes `const NdNTab ndn_X` per moved list. NDN/NDN_DECL go next to NDG (nd_gtab.h:63-64).
- A row: one gset.d file, the tool run, at most 10 keep lines by hand. Gate: G1 host differential, drec byte-equal, listcen C5 and C6 fall by the
  set's counts, elfgate gfxnames falls. Order (largest C5 first): src/entity, assets/us effects, src/world, model.c render modes, battle, pause,
  level_up and starpoint, the rest. Map shapes in the ROM blob: GSET-MAP (plan section 7), not this tool.
## 6. Discoveries (for plan_endgame.md section 7)
- listcen counts src/world/dead (3 files, build 30, gfxpos 32: FACT listcen), but maps.cmake builds no file of it. Choice: listcen.py skips
  src/world/dead after ws_D and ws_GEO G2 merge. Reason: no build or drec can prove an edit there.
- Plan rows to change: NTGT is no Sonnet core (section 1). NXF2-4 are ws_GEO. NEMIT is ws_EMIT plus E4 (Haiku), before GSET1-6.
- The 25 world, entity and battle recdl sites are nd_scomp_rec defs (S6E2, S6M6, S5BTL1), not list entries: entcap never sees them
  (it logs nd_gtab_run lists only, nd_walk.c:501-505). Choice: each def becomes setter calls in unit order (ws_SCOMP.md, 18 sites). The 7 sites of
  mac_01/crystal_ball_gfx.c wait for a design of gsSPLookAtX/Y alone and of gDPSetHilite1Tile/2Tile in a value packet (no codemod map entry).
