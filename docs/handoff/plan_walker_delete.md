# plan_walker_delete.md: the chain from "game code without list calls" to "no walker" (oracle PHASEE, 2026-11-17)
Edit in place. The oracle writes each row page (ws_<ID>.md) from a code read of main when the row's blockers merged.
## 1. State (FACT, listcen on main d74594f376)
build 312 (include 308, flame.c 2, status_star_shimmer.c 1, tst_13 main.c 1), gfxpos 4 (flame.c 2, shimmer 1, tst_13 1),
data 191 (all gbi.h), data_arrays 0, gfxtype 251, recdl 6, reader 792, model 15.
Gap (FACT, src/effects/flame.c:188, :211): two `gSPDisplayList(gMainGfxPos++, NDN(...))` lines. No page covers them.
FXSLOT1, FXSLOT2 and TST13 give gfxpos 2, not 0 (FACT, sum of their pages). Row FLAMEDL (section 3, S0) takes the last 2.
## 2. What the walker is today (FACT, code read)
- Every game state call writes one N64 typed unit (NDO_*) into the stream: nd_st.c:13-75, nd_val.c:14-31, nd_geo.c, nd_xf.c, the
  inline setters of nd_tx.h and nd_at.h, all through nd_nr (nd_nr.h:11). The unit layout is the gbi word layout (nd_nr.h:2-3).
- Core 0 runs the units in nd_walk_state (nd_walk.c:390-407) into the walker state nd_ws. A draw then decides on core 0:
  sync_combine (nd_comb.c:320), sync_render (nd_set.c:210), ensure_tile (nd_tex_set.c:441), set_tilesize (:140), tex_win0/1
  (nd_texwin.c:11, :83), load_mtx (nd_mtx_set.c:130), gv_tri (nd_geom.c:297), nd_texrect and nd_fillrect (nd_rect.c:219, :306).
- Every combine decision passes nd_comb_pick (nd_comb.c:240, callers :276 and :302): a table hit (nd_pso_find, nd_psolk.c:18) or
  nd_comb_core on a miss (:241). The table keys come only from ND_PSO_DESC text in src and 3ds/src (build.sh:190).
- Static lists: each v2 table holds the typed units of its list (NDN_UNIT, nd_gtab.h:104). gt_run runs them on the walker
  (nd_walk.c:624, :633). Other record kinds also end in the walker: NDC_MODEL, NDC_SPR, NDC_ENT, NDC_TEXRECT (nd_rec.c:597-600),
  map shapes (nd_shape.c, nd_dlc.c), particles (nd_parts_run, nd_walk.c:452), compiled defs (nd_scomp.c).
- Game calls (FACT, grep of src and include, this session): 3101 native API calls. State setters: nd_pso_* 825, nd_val_* 633,
  nd_tex_* 221. Draw calls: nd_draw_list 439, nd_xf_push 384, nd_xf_pop 291, nd_mesh_* 94, nd_rect_* 77.
- nd_op.c is host only (CMakeLists.txt:349-351). It holds 62 reader tokens (FACT, listcen).
## 3. Stages (order). "Pre" = what must be true before the stage starts.
| stage | rows (model) | files | counter move | gate benches |
|---|---|---|---|---|
| S0 now | OPMOVE (Haiku, page on main) | nd_op.c to 3ds/tools, 7 host.d scripts, nd_host_test.sh, CMakeLists.txt | reader -62 (FACT) | spot_mac01_drec |
| S0 now | PSOSEEN (Haiku, page next) | nd_comb.c:240 miss log, new 3ds/tools/psoseen.py, new 3ds/src/nd_pso_seen.c | none (pk_miss falls, OPEN) | 6 psochk scenes of azt.d ENTRY1, ENTRYFX, FXCULL, FXDIG, FXENT3, ISEED |
| S0 after FXSLOT1 | FLAMEDL (Haiku) | src/effects/flame.c :188 :211: capture form, entcap, entry | build -2, gfxpos -2 (FACT) | fxops_drec |
| S1 pso table only | PSOFB (Haiku) | nd_comb.c:241 fixed fallback and one log line (user 2026-11-16), nd_decide.c split: device helpers (nd_pso_in_eq, nd_pso_flags, nd_pso_fix, nd_const_rgb, nd_comb_tst) to nd_psolk.c, the rest to 3ds/tools/nd_decide.c (gen_pso_tab.py:129, build.sh:211, host.d coin.sh, conv2c.sh, decide.sh) | model -2 (FACT, nd_decide.c 2) | the 6 psochk scenes, miss 0 |
| S2 design | NSTATE (oracle design, Opus) | docs/handoff/design_nstate.md: the core 1 draw state, the native draw record, the v2 field-set op | none | none |
| S3 setters | NST1..NST4 (Haiku) | nd_st.c, nd_val.c, nd_geo.c, nd_xf.c, nd_tx.h, nd_at.h, nd_nr.h: write core 1 fields, no NDO unit | reader about -95 (GUESS, listcen of these files) | spot_mac01_drec, wscmb_pause_drec |
| S4 draws | NDR1..NDR4 (Haiku) | nd_geom.c, nd_rect.c, nd_direct.c, nd_model_set.c: run the native record (pso id, bind, colours) | model -6 (nd_geom 1, nd_rect 2, nd_comb 3) GUESS | spot_mac01_drec, nimgm_anim_drec, axns_sam2_drec |
| S5 static lists | NCONV1 (Sonnet, reason below), NCONV2 (Haiku) | nd_gfx_conv.c v2 output: field-set ops and draws, no NDN_UNIT; gt_run (nd_walk.c:633) | reader about -40 (GUESS) | fxops_drec, fxcull_drec, entcape_kmr03_drec |
| S6 records | NREC1..NREC5 (Haiku, after NSTATE) | nd_recs.c and nd_model.c (R0, R13), nd_walk.c nd_walk_spr, nd_ent.c ENT_GBI, nd_shape.c and nd_dlc.c, nd_parts_run | reader about -190 (GUESS) | vcfix_hos01_drec, spot_mac01_drec, sbk14_drec |
| S7 delete | S9DEL rows of plan_endgame.md:193-197 (Haiku) | nd_walk.c op arms, nd_op.h, nd_set.c, nd_scomp.c, nd_tex_set.c tile model, nd_mtx_set.c list stack, nd_cen.c, nd_gbi.h, gMainGfxPos (variables.h:111), then gbi.h, gbi_custom.h, gt.h (user option B 2026-11-17: headers go here) | reader to 0, model to 0, build 308 to 0, data 191 to 0, gfxtype to 0 | the full drec set, then the end proof |
| S8 end | S9GATE (Haiku) | none | elfgate func 0 data 0 gfxnames 0 | plan_endgame.md:80-90 |
Pre per stage:
- S0: none. OPMOVE and PSOSEEN share no file with FXSLOT1/2, TST13, IMGOVL, MISCP (FACT, their pages).
- S1: PSOSEEN merged. psochk `miss 0` in the 6 scenes on main.
- S2: user decision D1 (section 4). S3 to S6: design_nstate.md on main.
- S5: CONV2 tables complete (nd_gfx_conv.c). NCONV1 is Sonnet: one algorithm over 922 tables (FACT, plan_endgame.md:159), its error
  cases are not known per list before it runs (the CONV2C reason).
- S7: reader 0 for the file of the row except its own grep terms. build 0 and gfxpos 0 in src (S0 rows merged).
## 4. User decisions
- D1 end form of the 1679 state setter calls (FACT, section 2). Option A: the setters write one core 1 draw state (NdPsoDesc fields,
  2 NdBindDesc, colours). Each draw finds its pso id in the table on core 1 and records one native draw record. Game code stays.
  Core 0 decides nothing. Option B: plan v3 section 2 as written: a static ND_PSO_DESC and NdBindDesc at every draw. The setters
  leave the game. About 84 rows of plan Phase C and D (FACT, plan_endgame.md:224). Oracle advice: A. It fits the user decision of
  2026-11-16 (table lookup, fixed fallback, one log line). Risk of A: a state that no scene reached draws with the fallback.
- D2 the walk census (nd_cen.c 601 lines, walk_dbg_* nd_walk.c:110-140, t4chk :329, lstcen :222, nd_c2haz.c). The ELF ban list
  names gbi_.* and nd_lstcen (plan_endgame.md:84-87). Option A: delete them with the walker. The prof lines of pm_gpu stay.
  Option B: one Haiku row rebuilds a census of native draw records first. Oracle advice: A.
## 5. Gates and budget
Every row: `sh 3ds/tools/rowloop.sh --row <ID> --bench <bench> --why "<text>"` byte-equal against the merge-base, host PASS,
listcen of the row's class lower by the stated move. A row with a psochk scene: the `psochk end` bad and miss values not higher.
A row with more than 2 benches runs them in 2 rowloop calls (1 build, at most 2 Azahar runs per call).
## 6. Risks
- R1 (S1, A): a key that no bench scene reached draws with the fallback. The log line names it. The console soak finds the rest.
- R2 (S3-S4): core 1 does the lookup per state change. GUESS: one binary search per changed key. PROF rows on the console decide.
- R3 (S3): the skip test nd_op_same (nd_walk.c:280) and gv_epoch (:283) shape the draw batches today. The native record must keep
  the same batches, else drec differs. design_nstate.md names the rule.
- R4 (S3): 9 host tests use NDO units (stunit, geounit, texunit, hunit, nmdl0, ns3unit, nxf, nd_cmd_test, nd_gv_test). Each S3
  row rewrites the test of its setter file in the same commit.
- R5 (S6): the replay rule (nd_cmd.h:5): eye 2 and interp replay the stream. A record holds values, never a pointer to core 1 state.
