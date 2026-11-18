# plan_finish.md: every row from main today to the end proof, in waves (oracle, edit in place)
Goal: all listcen totals 0 and elfgate `func 0 data 0 gfxnames 0`. Design: design_nstate.md (S3-S6). Replaces plan_endgame.md:193-197 (Phase E).
Tags: FACT (source), GUESS, OPEN. All rows are Haiku unless section 3 names them.
## 1. Counters (main, FACT: orchestrator brief 2026-11-17) and the waves that take them to 0
| counter | now | falls in |
|---|---|---|
| build 308, data 191 | all in include/ (FACT plan_walker_delete.md:4-5) | W14 DEL4D1-4 |
| gfxtype 251 (246 after GFXT1, FACT e2a942d6f3) | nd_walk.c, nd_ent.c, nd_shape.c, nd_rec.c, nd_recs.c, nd_cen.c, PR headers (grep) | W6-W14 |
| reader 771 (REN4 gate) | walker, setters (grep) | W3 REN5-8 about -287 (oracle token counts), W6-W8 about -190 GUESS, W9-W13 rest (deleted files) |
| model 15 | nd_comb.c 3, nd_decide.c 2, nd_tex_set.c 3, nd_texwin.c 2, nd_rect.c 2, nd_set.c, nd_geom.c, nd_mtx_set.c (grep) | W3 MREN -4, W12 DEL2A-F rest |
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
| W4 | NDR0E1 snap carries the coloured combine decision, nd_comb_apply, snapchk vs core 0 and GPU state (7c8aeb0f21: nd_comb.c, pm_gpu.c, nd_snap*). Then NDR0E2 mesh triangles take the snap combine, snap always recorded (nd_rec_st), snapchk at texture rectangles (b47413357f: nd_rec.c, nd_snap.c, nd_rect.c). Then NDR0F1 render block and mix in the snap, compare after sync_render (b31daf9ca6: nd_snap*, nd_set.c, nd_geom.c). Next from their data: NDR0F2 render apply in gv_epoch_enter (no second pm_gpu_set_zmark(0): nd_set.c:243 vs nd_geom.c:221), NDR0G binds (ensure_tile, dual_bind at nd_geom.c:212-213), NDR1B texrect apply | NDR0D (merged) | 1 each, serial on nd_snap.c | nd_snap.c, nd_rec.c |
| W5 | NDR1 rectangles (nd_tx.h :50-56, nd_rect.c); NDR2 direct draws (nd_direct.c, hud_element.c, msg_draw.c, screen_overlays.c); NDR4 sprite records (nd_rec.c nd_rec_spr, nd_walk.c nd_walk_spr, sprite.c :278); NCONV1D core 1 cuts each ndnu_ unit array into state units and draw groups, one snap per group (nd_gfx_conv.c, nd_gtab.c) | NDR0E | 4 | nd_rec.c, nd_walk.c |
| W6 | NDR3 glyphs (nd_direct.c :212); NCONV2 delete the NDN_UNIT path (nd_walk.c gt_run :634, nd_ntab_run); NREC1A model record payload as FS ops on core 1 (nd_rec.c nd_rec_model, nd_ds.c nd_ds_model); NREC3 map shapes (nd_shape.c, nd_dlc.c); NREC5 compiled defs (nd_scomp.c) | W5 | 5 | nd_rec.c |
| W7 | NREC1B core 0 model runner reads snaps (nd_recs.c, nd_model.c); NREC2 entity (nd_ent.c, nd_rec.c :265 :278) | NREC1A | 2 | nd_rec.c |
| W8 | NREC4 particles (nd_rec.c :329, nd_walk.c :453 :500) | NREC2, NCONV2 | 1 | nd_rec.c, nd_walk.c |
| W9 | DEL3A nd_rec_nr records no unit (nd_rec.c :399), delete nd_dschk.c, snapchk; DEL1B nd_ent.c ENT_GBI; DEL1C nd_dlc.c; DEL5A nd_shape.c list parse; DEL5B v1 tables (nd_gtab.h NdGTab, nd_gtab.c nd_rec_dl, nd_gfx_conv.c v1, gen_gfx_tab.sh); DEL3B nd_scomp.c exec (:341 :347) | W8 | 6 | nd_rec.c |
| W10 | DEL1A caller arms (nd_rec.c NDC_STATE, st_put, nd_rec_state; nd_recs.c, nd_model.c, pm_phase.c, pm_nusys.c :89 :100) | W9 | 1 | nd_walk.c |
| W11 | DEL1D nd_walk.c walker and op arms, nd_cen.c, nd_c2haz.c (D2 A) | W10 | 1 | nd_walk.c |
| W12 | DEL2A nd_comb.c, nd_decide.c (rename for core 1); DEL2B nd_set.c; DEL2C nd_tex_set.c, nd_texwin.c; DEL2D nd_rect.c; DEL2E nd_geom.c gv part; DEL2F nd_mtx_set.c, nd.h NdWs fields | W11 | 6 | nd.h |
| W13 | DEL3C nd_op.h, nd_gbi.h, the last reader headers, gMainGfxPos (variables.h:111, game_stubs.c:9) | W12 | 1 | nd_op.h users |
| W14 | DEL4D1-2: gDP/gSP/gsDP/gsSP definitions and Gfx prototypes to 3ds/tools/gconv_inc (FACT gen_gfx_tab.sh:14): gbi.h; gbi_custom.h. DEL4D34 (gs2dex.h, gt.h, sp.h, gu.h: unused Gfx declarations, gt.h deleted) has no blocker: page now | W13 (DEL4D34 none) | 2 | one header per row |
| W15 | S9GATE: listcen all 0, elfgate 0 0 0, the full drec set, then the user's end checks (section 5) | W14 | 1 | - |
Width now (main cbdf65c35f): merged NDR0E2 (snapchk bad 486 at texture rectangles: fold timing), NST2I (epbad all key 2090a: quiet NAPI commands), DEL4D34.
Running: NDR0F1, NST2J (Azahar crash at a41cc4d4d4: clean rebuild asked). Running: NDR1B (rect compare after box_fold_sync, then the snap combine). Waiting: REN8 (after NST2J);
NDR0F2, NDR0G (from NDR0F1 data); NST2K (page: dschk keys a quiet native command by its runner address; after NST2J).
Critical path: NDR0F1 -> NDR0F2/NDR0G -> W5. W5 rows (NDR1-4, NCONV1D) need the full mesh snap run (combine, render, binds).
W10 DEL3A6 is done by REN6; DEL2A and DEL2C keep no rename part (MREN).
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
