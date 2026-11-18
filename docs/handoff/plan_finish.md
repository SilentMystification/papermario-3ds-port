# plan_finish.md: every row from main today to the end proof (oracle, edit in place)
Goal: all listcen totals 0 and elfgate `func 0 data 0 gfxnames 0`. Design: design_nstate.md (S3-S6). Stages: plan_walker_delete.md.
This table replaces the Phase E row names of plan_endgame.md:193-197. Tags: FACT (source), GUESS, OPEN. H = Haiku, S = Sonnet.
## 1. Counters (main, FACT: orchestrator brief 2026-11-17) and the stage that takes each to 0
| counter | now | falls in |
|---|---|---|
| build 308, data 191 | all in include/ (FACT plan_walker_delete.md:4-5) | S7 DEL4D |
| gfxtype 251 (246 after GFXT1, FACT e2a942d6f3) | nd_walk.c, nd_ent.c, nd_shape.c, nd_rec.c, nd_recs.c, nd_cen.c, PR headers (grep) | S6, S7 |
| reader 772 | 3ds/src walker, setters, nd_ds.c 32 (grep) | S5 about -40 GUESS, S6 about -190 GUESS, S7 rest |
| model 15 | nd_comb.c 3, nd_decide.c 2, nd_tex_set.c 3, nd_texwin.c 2, nd_rect.c 2, nd_set.c, nd_geom.c, nd_mtx_set.c (grep) | S7 DEL2A-F |
| elfgate func 48 | nd_walk.c 12, nd_tex_set.c 6, nd_ent.c 3, nd_set.c 3, nd_target.c 2, pm_nusys.c 2, others (grep) | S4 NDR6, S7 |
| elfgate data 365 | GUESS: ndg_ v1 tables of nd_gfx_tab.c plus gMainGfxPos | S7 DEL5B, DEL3C |
| elfgate gfxnames | OPEN (not in the brief) | S8 reads it |
S3 moves no listcen counter (FACT design_nstate.md:99). Its counter: dschk bad 0 and epbad 0 in the 4 dschk scenes.
## 2. Rows by stage (34 rows to do: H 31, S 3; S rows have the user's yes, FACT design_nstate.md:121)
Rule: each row gates drec byte-equal against `git merge-base HEAD main`. Every row of S3-S7 keeps drec equal, so a row may branch
from an unmerged row branch. Parallel rows never write fxrest.d/dschk_base.txt: the last row of the stage writes it.
S3 rest (3 rows; chain, shared files nd_ds.c, nd_direct.c, dschk_base.txt). NST3 merged (FACT oracle_handoff.md:11).
| row | model | files | blocker |
|---|---|---|---|
| NST2B, NST2C | H | done: branch NST2B 80d31a92ce, NST2C d374ade498 (gates PASS, FACT git log). Merge only | - |
| NST2D | H | ws_NST2D (nd_ds.c, nd_rec.c, nd_walk.c 2 lines, model.c 2 lines, nd_dschk.c). Step 1 done: b1975eb0c6 | NST2C |
| NST2E | H | ws_NST2E (6 runner files, nd_direct.c, nd_api.h). Facts checked on branch NST2D: all lines match | NST2D |
| NST2F | H | ws_NST2F (nd_ds.c, nd_ds.h, nd_dschk.c, nd_direct.c 2 lines). Facts checked on branch NST2D | NST2E |
NST2G only if NST2F ends with bad or epbad above 0 (OPEN, 0 planned). Outside the chain: FALLBK (nd_walk.c 3 lines, oracle NSTO; fixes the wscmb drec variants),
GFXT1, PSOCOV, BASELOG, OPTIONS (done on branches, merge only).
S4 draws (7 rows). Blocker: NST2F gate. NDR0 defines every NdSnap field. NDR1-6 only call it: no edit of nd_snap.c or nd_ds.c.
| NDR0 | S | new nd_snap.h, nd_snap.c (core 1 make, core 0 nd_snap_run), nd_ds.c pso dirty bit, lights and viewport blocks, nd_geo.c nd_mesh_tri1/2, nd_geom.c, new host test | NST2F |
| NDR1 | H | nd_tx.h nd_rect_* macros (:50-56) to snap records, nd_rect.c new runner (walker arm stays) | NDR0 |
| NDR2 | H | nd_direct.c (nd_direct_bind, 6 runners), hud_element.c, msg_draw.c, screen_overlays.c | NDR0 |
| NDR3 | H | nd_direct.c nd_glyph_draw (:212), msg_draw.c glyph run | NDR2 (same files) |
| NDR4 | H | sprite records: nd_rec.c nd_rec_spr (:255), nd_walk.c nd_walk_spr, sprite.c spr_state_run (:278) | NDR0 |
| NDR5 | H | transform (UD2 native stack): nd_xf.c, nd_at.h NDXF macros, nd_mtx_set.c runner | NDR0 |
| NDR6 | H | target role on core 1: nd_target.c (img_role, off_screen_check), nd_geo.c nd_target_beginw (:16) | NDR0 |
Parallel after NDR0: NDR1, NDR2, NDR4, NDR5, NDR6 and NCONV1 (disjoint files). NDR3 after NDR2.
S5 static lists (2 rows). Gate benches fxops_drec, fxcull_drec, entcape_kmr03_drec (FACT plan_walker_delete.md:32).
| NCONV1 | S | nd_gfx_conv.c NDN_FS and NDN_DRAW output, nd_gtab.h, nd_gtab.c core 1 run, nd_ds.h enum NdFs, nd_ds.c nd_ds_fs, listcen.py class core1 (UD1) | NDR0 |
| NCONV2 | H | delete the NDN_UNIT path: nd_walk.c gt_run (:634), nd_ntab_run, nd_ntab_rec, nd_gtab_test.c | NCONV1 |
S6 records (5 rows). Gate benches vcfix_hos01_drec, spot_mac01_drec, sbk14_drec (FACT plan_walker_delete.md:33).
| NREC1 | S | model record: nd_recs.c, nd_model.c, nd_rec.c nd_rec_model, model.c, entity_model.c, animator.c, nd_rmtab.c | NCONV1 (needs NdFs only) |
| NREC2 | H | entity: nd_ent.c, nd_rec.c nd_rec_ent, nd_rec_ent_draw (:265, :278) | NREC1 |
| NREC3 | H | map shapes: nd_shape.c, nd_dlc.c | NREC1 |
| NREC4 | H | particles: nd_rec.c nd_parts_add (:329), nd_walk.c nd_parts_run (:453, :500) | NREC2 (nd_rec.c), NCONV2 (nd_walk.c) |
| NREC5 | H | compiled defs: nd_scomp.c (ent_sdef user nd_ent.c:213 stays NREC2) | NREC1 |
S7 delete (16 rows; the orchestrator tags `pre-list-removal` first, FACT plan_endgame.md:198). Blocker: S4, S5, S6 merged.
| DEL3A | H | setters write nd_ds by ND_FS, no unit: nd_rec.c nd_rec_nr (:399), nd_nr.h, nd_st.c, nd_val.c, nd_geo.c, nd_xf.c, nd_tx.h, nd_at.h, nd_ds.c keys; delete nd_dschk.c; the 9 host tests of R4 | S6 |
| DEL1A | H | nd_rec.c NDC_STATE, st_put, sh shadow, nd_rec_state, caller arms; nd_recs.c, nd_model.c, pm_phase.c, pm_nusys.c gbi_walk, gbi_task_w (:89, :100) | DEL3A |
| DEL1B, DEL1C, DEL5A | H | nd_ent.c ENT_GBI path; nd_dlc.c and its CMake line; nd_shape.c list parse | DEL3A |
| DEL5B | H | v1 tables: nd_gtab.h NdGTab, nd_gtab.c nd_rec_dl, nd_gfx_conv.c v1 output, gen_gfx_tab.sh (elfgate data ndg_) | DEL3A |
| DEL3B | H | nd_scomp.c nd_scomp_exec, nd_scomp_exec_own (:341, :347) | DEL3A |
| DEL1D | H | nd_walk.c walker and op arms, nd_cen.c, nd_c2haz.c (D2 A: census goes with the walker) | wave of DEL1A-DEL3B |
| DEL2A-DEL2F | H | 2A nd_comb.c, nd_decide.c (UD4 first); 2B nd_set.c; 2C nd_tex_set.c, nd_texwin.c; 2D nd_rect.c; 2E nd_geom.c gv part; 2F nd_mtx_set.c, unread NdWs fields | DEL1D |
| DEL3C | H | nd_op.h, nd_gbi.h, the last reader headers, gMainGfxPos (variables.h:111, game_stubs.c:9) | DEL2 wave |
| DEL4D | H | gDP/gSP/gsDP/gsSP definitions and Gfx prototypes of gbi.h, gbi_custom.h, gs2dex.h, gt.h, sp.h, gu.h move to 3ds/tools/gconv_inc (host converter path, FACT gen_gfx_tab.sh:14) | DEL3C |
Core 1 code reuses no name of listcen MODEL_NAMES (listcen.py:196-198) or of the elfgate func line (elfgate_patterns.txt:1).
S8 (1 row): S9GATE (H): listcen all 0, elfgate 0 0 0, the full drec set, the console soak (G10, G11).
## 3. Critical path: 15 rows
NST2D, NST2E, NST2F, NDR0, NCONV1, NREC1, NREC2, NREC4, DEL3A, DEL1A wave, DEL1D, DEL2 wave, DEL3C, DEL4D, S9GATE.
Off the path: NDR1-6, NCONV2, NREC3, NREC5 (they must merge before DEL3A). GUESS 1.5 to 3 h per path row: 25 to 45 h.
## 4. Console checks (orchestrator rule 8): one per stage, by batch
- Each stage merges as one batch: the orchestrator merges its branches in row order in one sitting, builds once, runs the stage
  benches (1 build, 2 Azahar runs), then writes the ready line. The first merge of the next stage waits for the user's check.
- Work never waits: rows of the next stage branch from the tip of the current batch branch.
| check | after | gates the merge of | content |
|---|---|---|---|
| K3 | NST2F merged (NST2B-F) | NDR0 | UD3 prof push (core 1 cost, R1) plus verify_list.md |
| K4 | NDR0-6 merged | NCONV1 | verify_list.md (all draw kinds now snaps) |
| K5 | NCONV1-2 merged | NREC1 | effect and list scenes. Option: fold K5 into K6 (S6 branches from the S5 tip) |
| K6 | NREC1-5 merged | DEL3A | map, model, entity, particles |
| K7 | DEL3A-DEL4D merged | S9GATE | the soak list |
| K8 | S9GATE | end | G10, G11 (FACT plan_endgame.md:100) |
- A failed check: push the row tips of the stage by halves (GUESS 3 pushes for 7 rows) to name the row.
## 5. Decisions open for the user
- UD4 (answered): keep nd_comb_core on core 1 for a miss. DEL2A renames decide and decide_miss (nd_decide.c:172, :214) on core 1.
  Sonnet rows NDR0, NCONV1 and NREC1: standing yes.
