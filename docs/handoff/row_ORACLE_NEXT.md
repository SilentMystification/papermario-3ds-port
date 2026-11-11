# Oracle handoff (2026-11-11, outgoing oracle at about 600K context)

A replacement oracle starts here: read CLAUDE.md, orchestrator_rules.md, plan_hotcode.md (sections 2 and 5), then this file.

## Running or committed rows (state as given by the orchestrator)
- HC8 (Haiku): sprite/entity fast paths; pm_gpu.c pm_gpu_spr_build/pm_gpu_grange, nd_model.c sprite memo, nd_ent.c ent_run, nd_shape.c
  dlc_box_out/dlc_try. Step 1 census done; byte gates spot_mac01_det, mdl5_iwa10_det (not battle_start_f182_cold).
- HC5 (Opus), KZN04 non-TEV part (Sonnet), LOADMTX (Haiku, nd_mtx_set.c guard), SYNC60 (Haiku, pm_gpu_begin_wait time pacing at
  60 FPS): started. TOOLB8 (bench.py base_fill accepts a build with HEAD's inputs): written.
- HCCUT1 (Haiku, written now): rect_sig_check body cold, ndm_cmp and pm_gpu_ndm_end cold (pm_gpu.c only); ready, runs beside the above.

## Waiting rows
- HC6 (Opus): after HC8. HC7b: after HC5 (box_pat known on core 1). KZN04 TEV part: after HC8.
- STEREOFX: needs the user's ESCAPE crash.txt from the XL (P5 hang: a GX entry whose interrupt never comes; P6 stray store).
- SHEEP: needs a console prof push of the battle item and HC8 (row_SHEEP.md lists the paths).

## Open work (not yet rows)
1. DEADCODE: run the listing: the current map (build3ds/pm_3ds.map, 2026-11-11) plus hotset.py's call graph (H.model(''): fns, names,
   calls, cg); a function with no caller and no address taken (check .rodata/.data relocations with objdump -r) is dead. Write Haiku
   rows by file. pm_port.h:102 still names nd_walk_run.
2. Class cuts from M1-hot (main map, hot only, top functions): E 20.1 KB (ndm_cmp 6672 and rect_sig_check 2140: HCCUT1; nd_raw_native
   2504, nd_tev_build 1988, nd_native_submit 1936, build_words 1072, nd_tmpl_build 1008); H 15.1 KB (nd_model_state_in 3260,
   nd_tev_build 1988, pm_gpu_spr_build 1800, nd_comb_decide 1248, nd_model_state_spr 996, dual_bind 844); G 12.0 KB (nd_texrect 1644,
   box_pattern_rect 1328, tex_raw_piece 968, dual_bind 844, pm_gpu_tex_rect 772, sync_render 704, tile_texel 684, rect_to_image 640);
   C 11.3 KB (rec_run 1432, pm_gpu_model_hit 1172, nd_tmpl_build 1008, pm_gpu_ndm_end 728 (HCCUT1), pm_gpu_model_key 716, nd_tex_get
   688); F 10.6 KB (gv_tri 1620, nd_geom_vtx 1508, pm_gpu_gtrip 1452, nd_gv_params 952, gv_matrix_id 888). Next: read each large
   function for rare arms behind debug switches (as HCCUT1) and write byte-equal cold-split rows by place.
3. Stale rows: STALE headers were added (picture steps: BATWIDE, FX1, MEM, MOVE, PAUSEDR; deleted pm_gbi.c names: AUDIT, BG, BTRANS,
   DEADCODE, FX1, MOVE, SBKW, SIZE, STALE2, UB1). Each needs a re-read: launch, needs a user fact, or close (delete). Also AUDDROP,
   BATUI, KEYMISS, TEXSTALE, and the others in docs/handoff/row_*.md with user facts (AUDNAP, BATWIDE, BTRANS, FFOG, FFOREST, HOMEHANG,
   INTERIOR, INTROSNAP, LINRACE, PAUSEFLK, SAVETITLE, SBLK2, WATERPAN, ...).
4. Cold-boot nondeterminism (LOADMTX note, battle_start_f182_cold base "two runs differ"): a thread race (ROM stream, MAPX-4 prefetch
   pm_map.c:177-181, LINRACE linear allocator) is the lead; not proven. Uninitialized memory is ruled out (a warm restart reboots).

## Decisions made (user, through the orchestrator)
- load_mtx: guard and warn (row LOADMTX). 60 FPS: pace by time (row SYNC60). HC5, HC6: Opus approved. KZN04: Sonnet approved,
  non-byte-equal shading accepted. ESCAPE2/3: START+SELECT and Close game are two separate exit mechanisms.
- Byte gates: spot_mac01_det, mdl5_iwa10_det, spot_mac00_det (or _pic); battle_start_f182_cold is not a byte gate until the race is found.
- Rows may share a file when they edit different functions; each row names the places other rows edit and merges main first.

## Harness facts learned
- A census bench (fncov_*) has no base. bench.py records a run under the bench that started it (TOOLB5). base lanes keep the ELF and
  last_crash.txt goes to build3ds/out (TOOLB7). "crash symbols" rc 2 comes from UnmappedAccess lines of the emulator log.
- M1 counts cold callees; use M1hot (TOOLHOT) and M3 (hotset.py --fncov on census logs) for class targets.
