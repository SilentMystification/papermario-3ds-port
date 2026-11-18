# plan_finish.md: every row from main today to the end proof, in waves (oracle, edit in place)
Goal: all listcen totals 0 and elfgate `func 0 data 0 gfxnames 0`. Design: design_nstate.md (S3-S6). Replaces plan_endgame.md:193-197 (Phase E).
Tags: FACT (source), GUESS, OPEN. All rows are Haiku unless section 3 names them.
## 1. Counters (main 8e5ad5a5e2, FACT: orchestrator brief 2026-11-18) and the waves that take them to 0
| counter | now | falls in |
|---|---|---|
| build 306, data 191 | all in include/ (FACT plan_walker_delete.md:4-5) | W14 DEL4D1-2 |
| gfxtype 230 | nd_walk.c, nd_ent.c, nd_shape.c, nd_rec.c, nd_recs.c, nd_cen.c, PR headers (grep) | W6-W14 |
| reader 496 (REN8 -79 gated, merge pending); core1 363 | walker, setters (grep) | W6-W8 about -190 GUESS, W9-W13 rest (deleted files) |
| model 11 | nd_comb.c 3, nd_tex_set.c 3, nd_rect.c 2, nd_set.c 1, nd_geom.c 1, nd_mtx_set.c 1 (listcen 8e5ad5a5e2) | W12 DEL2A-F |
| elfgate func 48, data 365 | func: nd_walk.c 12, nd_tex_set.c 6 and others (grep). data GUESS: ndg_ v1 tables and gMainGfxPos | W9-W13 |
| elfgate gfxnames | OPEN (not in the brief) | W15 reads it |
## 2. Rules of the waves
- A row gates drec byte-equal against `git merge-base HEAD main`, host PASS, dschk or snapchk not higher. It merges on its gate (no console check
  until the end, rule 8). A row whose blocker is not merged branches from the blocker's gate commit.
- Rows of one wave share no file, or only hunks 20+ lines apart that both pages name. Only one row per wave writes fxrest.d/dschk_base.txt (named in the page).
- Core 1 code reuses no name of listcen MODEL_NAMES (listcen.py:196-198) or of the elfgate func line (elfgate_patterns.txt:1).
- nd_comb_core writes pm_comb_vprim (nd_decide.c:234, :244). A core 1 call (UD4) passes vprim and writes no core 0 global (NDR0D edit).
## 3. Waves (50 rows: REN6-8, MREN and NST2H added, DEL3A6 done by REN6; width = rows that run at once; the stop file blocks a wider split)
| wave | rows (files) | needs merged | rows | stop file |
|---|---|---|---|---|
| W1 now | NDR0A snapchk (page b60f70c281: nd_snap*.c, nd_geo.c, nd_geom.c 1 line, nd_rec.c, nd_psolk.c); NCONV1A ND_FS_ ids and listcen core1 (page 03045f4a0d: nd_fs.h, listcen.py, host only). Running: NST2F, FALLBK (oracle NSTO) | - | 2 | nd_ds.c (NST2F), nd_rec.c (NDR0A) |
| W2 | NDR0B td on core 1 (5200d6d63a: nd_ds.c, nd_dschk.c); NDR0C role and render inputs (dfb1afe9d5: nd_snap*.c, nd_geo.c 3 lines); REN1-REN4 (a78c6d0139: nd_st.c+nd_val.c+nd_xf.c; nd_tx.h; nd_at.h; nd_nr.h). Then NDR5 transform record (fd8361c31e: nd_xf.c after REN1, nd_rec.c, nd_api.h); NST2G dschk with FALLBK (caa2711877: nd_dschk.c after NDR0B, nd_walk.c 1 line) | NST2F, NDR0A, NCONV1A, REN1, NDR0B | 8 | nd_dschk.c, nd_xf.c |
| W3 | NDR0D box fold in the snap, core 1 miss path with its own decision table, build lock, vprim and k4 parameters (page f45c77160f: nd_snap*.c, nd_decide.c, pm_comb.c, tests); REN5 nd_geo.c; REN6 nd_ds.c, nd_seam.c, 6 unit headers (796ae535ae, reader -65); REN7 nd_recs.c, nd_model.c, nd_scomp.c, nd_ent.c (e5445e717c, reader -140). Then REN8 nd_rec.c after NDR5 and NST2H (c229b712e9, reader about -73); MREN decide, decide_miss, tex_win0/1 renamed after NDR0D and REN6 (f1c6cf10ef, model -4); NST2H dschk epbad source keys after NDR5 (c19b1b7efa: nd_rec.c, nd_walk.c, nd_dschk.c) | NDR0B, NDR0C (merged) | 7 | nd_decide.c (NDR0D, MREN), nd_ds.c (REN6, MREN), nd_rec.c (NDR5, NST2H, REN8) |
| W4 | snap chain, pages on main (oracle FIN 2026-11-18): NDR0F1 + fix NDR0F1B (running: render block compare, rpend scoped to draw_tri); then NDR0F2 render apply (nd_set.c, nd_snap.c, nd_geom.c) parallel with NDR0G1 bind blocks in the snap (nd_snap*, test); then NDR0G2 bind apply (nd_bsrc macros: nd_tex_set.c, nd_comb.c dual_bind) parallel with NDR3 glyphs (nd_rec_napi_s keeps the arm: nd_rec.c, nd_api.h, msg_draw.c, nd_direct.c glyph); then NDR1C rectangles (snap always, zcol role, render and binds) | NDR0F1; NDR3 also REN8, NST2K | 2 at a time | nd_snap.c, nd_rec.c |
| W5 | NDR2 direct draws (nd_direct.c nd_pso_decide .. nd_direct_end, hud_element.c, draw_img_util.c, msg.c, screen_overlays.c, 190B20.c, state_title_screen.c; core 1 snap of nd_ds with the pso words, runner records by nd_rec_napi_s); NDR4 sprite records (sprite.c :467, nd_rec.c nd_rec_spr, pm_gpu.c pm_gpu_spr_build reads nd_ws); NCONV1D core 1 cuts each ndnu_ unit array into state units and draw groups (nd_gfx_conv.c, nd_gtab.c). OPEN: no page yet (design read pending) | NDR1C, NDR3 | 3 | nd_rec.c, nd_direct.c |
| W6 | NCONV2 delete the NDN_UNIT path (nd_walk.c gt_run :634, nd_ntab_run); NREC1A model record payload as FS ops on core 1 (nd_rec.c nd_rec_model, nd_ds.c nd_ds_model); NREC3 map shapes (nd_shape.c, nd_dlc.c); NREC5 compiled defs (nd_scomp.c) | W5 | 4 | nd_rec.c |
| W7 | NREC1B core 0 model runner reads snaps (nd_recs.c, nd_model.c); NREC2 entity (nd_ent.c, nd_rec.c :265 :278) | NREC1A | 2 | nd_rec.c |
| W8 | NREC4 particles (nd_rec.c :329, nd_walk.c :453 :500) | NREC2, NCONV2 | 1 | nd_rec.c, nd_walk.c |
| W9 | DEL3A nd_rec_nr records no unit (nd_rec.c :399), delete nd_dschk.c, snapchk; DEL1B nd_ent.c ENT_GBI; DEL1C nd_dlc.c; DEL5A nd_shape.c list parse; DEL5B v1 tables (nd_gtab.h NdGTab, nd_gtab.c nd_rec_dl, nd_gfx_conv.c v1, gen_gfx_tab.sh); DEL3B nd_scomp.c exec (:341 :347) | W8 | 6 | nd_rec.c |
| W10 | DEL1A caller arms (nd_rec.c NDC_STATE, st_put, nd_rec_state; nd_recs.c, nd_model.c, pm_phase.c, pm_nusys.c :89 :100) | W9 | 1 | nd_walk.c |
| W11 | DEL1D nd_walk.c walker and op arms, nd_cen.c, nd_c2haz.c (D2 A) | W10 | 1 | nd_walk.c |
| W12 | DEL2A nd_comb.c, nd_decide.c (rename for core 1); DEL2B nd_set.c; DEL2C nd_tex_set.c, nd_texwin.c; DEL2D nd_rect.c; DEL2E nd_geom.c gv part; DEL2F nd_mtx_set.c, nd.h NdWs fields | W11 | 6 | nd.h |
| W13 | DEL3C nd_op.h, nd_gbi.h, the last reader headers, gMainGfxPos (variables.h:111, game_stubs.c:9) | W12 | 1 | nd_op.h users |
| W14 | DEL4D1-2: gDP/gSP/gsDP/gsSP definitions and Gfx prototypes to 3ds/tools/gconv_inc (FACT gen_gfx_tab.sh:14): gbi.h; gbi_custom.h. DEL4D34 (gs2dex.h, gt.h, sp.h, gu.h: unused Gfx declarations, gt.h deleted) has no blocker: page now | W13 (DEL4D34 none) | 2 | one header per row |
| W15 | S9GATE: listcen all 0, elfgate 0 0 0, the full drec set, then the user's end checks (section 5) | W14 | 1 | - |
Now (main 1a014d6057): merged NDR1B (rect combine apply; FACT nd_rec.c:309: rect snaps only with snapchk, so no drec gate ran it; NDR1C fixes), NST2J, SHAPECACHE. Running: NDR0F1 (fix page NDR0F1B), REN8, NST2K, HOSTTMP. Pages waiting: NDR0F2, NDR0G1, NDR0G2, NDR3, NDR1C (WAIT lines in each page).
NDR1B open item (answered): the DEPTHQ and rect_to_image returns (nd_rect.c:232-233) drop the snap, no row: no GPU primitive there. DEL2D gives rect_to_image core 1 data.
Stream size (GUESS): NdSnapChk is about 640 bytes after NDR0G1. A snap per glyph or rectangle is stopped by NDR3 (nd_rec_cmds). The compact NdSnap (design section 7) is a later row.
## 4. Sonnet rows split into Haiku rows, and rows Haiku cannot do
- NDR0 (Sonnet) is NDR0A-E. A, B and C are measured steps: snapchk and dschk counts name each missing input. D and E follow from those counts.
- NCONV1 (Sonnet) is NCONV1A and NCONV1D. NCONV1B and C are dropped: the ndnu_ unit arrays already hold the field-set words (ids equal ND_FS_, NCONV1A
  asserts), and core 1 applies them since NST2A (nd_ds.c nd_ds_list, v2 table: nt->u). NCONV1D needs NDR0E. No list has a new error case.
- NREC1 (Sonnet) is NREC1A-B. The record layout stays. Only the payload units become FS ops, so the 3 writers (model.c, entity_model.c, animator.c) do not change.
- NDR0E (answered 2026-11-18): NDR0D gave snapchk bad 0 (3601590c71), so NDR0E is Haiku, split in measure (E1) and apply (E2) rows.
- Every other row is Haiku with a closed page.
## 5. Console checks (orchestrator rule 8): none until the end
- The user's checks run at the end: the UD3 prof push, verify_list.md, then the soak list (G10, G11, FACT plan_endgame.md:100).
- An urgent fault is an exception: the orchestrator asks the user for a console. A failed end check: push the row tips by halves (GUESS 3 pushes).
## 6. Decisions
- UD4 (answered): keep nd_comb_core on core 1 for a miss. DEL2A renames decide and decide_miss (nd_decide.c:172, :214) on core 1.
- Sonnet rows have a standing yes. They are used only where section 4 names them.
