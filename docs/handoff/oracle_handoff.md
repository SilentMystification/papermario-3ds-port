# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 2308ccc573, 2026-11-15. Twentieth oracle: ws_MOON2 (after Merge MOON1), ws_XFT (parallel). Nineteenth: ws_PAUSEH (after Merge HWFIX), ws_MOON1. OPEN:
(a) black coins: ws_COIN (after Merge HWFIX). FACT cause: stale dual_on from a shaded sprite (nd_tex_set.c:508); the item draw with shading on sets no tile 0 size
(item_entity.c:979-993), so decide is skipped (nd_decide.c:302), legacy mode 2 = texel * vertex colour 0. GUESS only maps with sprite shading. (b) moon air: ws_MOON2.
(c) map change stutter: ws_XFT, then a console prof run. FACT console 11-11 (build3ds/hw_xl_log.txt #806-#810): seamfix mac_00 103.9 ms inside
dlc 121.2 ms (seam_get in timed dlc_build, nd_shape.c:375), maxstep dlc 112.6, ent 3 (ent_reset is not the cost). FACT .dlc max 47841 B (sam_11),
.seam max 13536 B. Fix idea after the measure: read .seam and .dlc whole at pm_map.c:151 (one fread each, the .nds header once).
Eighteenth oracle: ws_ISTAR (Haiku, starpoint move, C5 -33 C6 -7 C7 -7 by dry run). Design cores, no page: ISEED2 (an entity-path list draws in nd_ent_case, nd_walk.c:166; its G_DL runs only when that refuses, :171; a seed has no C site: needs a seed source for gen_ntab_entries.py), firework.c:293 and firework_rocket.c:277 (GUESS state-only lists: entcap_so logs no tuple, nd_walk.c:611), dgb_01 (its light is .bss of gfx.o in pm_3ds_5672.map, no source value), model.c ND_RM_REC. PAL twin and JP lists (419 + 38 data) are a user choice for listcen. Thirteenth oracle: ws_ROOTCM, ws_GTIN (FACT listcen 40518d695: C1 805, C2 288, C5 12798, C6 1100, C7 1768, C8 777, C10 16).
Seventeenth oracle: ws_HWFIX extended (audit_hw2 F2 depth drain, F3 menu grey invalidate and PPF loop in group 1; group 3 = F1 sc_sig_now comma fix with WAIT: base reset). Sixteenth oracle (fdf73a739): ws_HWFIX (Haiku; audit_hw A1-A3, B1, B2, B4; B3 = A1; group 2 after Merge ISEED). FACT: memcheck margin counts static size only (mem_budget.json rule); heap chunks come out of leave_kb 3458. Fifteenth oracle (59cd90449): ws_ISEED (Sonnet; effect C-site seeds, converter call-unit seeds). FACT class of the 260 inherit lists: effects 171 = callees 54, C sites 41, seeded-unsup 35 (vertex slots 24, MOVEWORD 9), no reference 41; entity 69 = callees 24, entity-path refs 45 (nd_rec_ent_draw: entcap has no hook there); other 20. Next: ISEED2 (ent-path capture hook). Fourteenth oracle (d7d4d6197): ws_LIT1 (light units, level_up.c, starpoint.c). Next design cores (no Haiku page yet): dgb_01 (168 lists, 2833 data) needs GSET-W (gset.py BLOCK_OPEN :20 misses `Gfx N(x)[]`) and a source definition of its light (`extern Lights1 N(D_80253140_C109C0)` gfx.c:4 only, .bss in the map); inherit seed (260 lists, the seeds are C-site states: needs captures, not Haiku); model.c render modes (readers: nd_rec.c:139 nd_rm_build, model.c:1260 NdSDef defs for ND_RM_REC in animator.c, entity_model.c, item_entity.c:950, model.c:1616 and nd_recs.c:85 combine words); 114 root list calls with no entry. PAL-only files (level_up_letters_*, starpoint_{de,en,es,fr}: 419 data, no cmake builds them) are a listcen skip choice like src/world/dead. Twelfth oracle: FXENT3 fault answers, FXREST2 facts (below). Eleventh oracle: ws_GCONVW. Tenth: ws_FXENT3. Ninth oracle: ws_PSOLK, ws_RECROOT, ws_FXENT2, ws_GSET2, ws_GSET3; design_lookahead 5, 6, 9.
## Goal
Remove the display list system completely (plan_endgame.md v3). Done only when `3ds/tools/elfgate.py` prints
`func 0 data 0 gfxnames 0` and every LISTCEN2 counter C1..C12 is 0.
FACT (`python 3ds/tools/listcen.py`, a37368582): C1 build 1166, C2 gfxpos 617, C5 data 22040, C6 data_arrays 2075, C7 gfxtype 2812, C8 reader 757, C9 model 15.
FACT (Merge FXSELF 1149b5513): C10 recdl 57 (42 `nd_rec_dl(` in src/effects + 15 root nd_scomp_rec), build 1161, data 22012.
## Tools on main (all in 3ds/tools unless named)
- listcen.py (C1-C10, RECDL_RX :47), elfgate.py (C11-C12), codemod.py (MAP :38-49, STATE_MAP :53-73, GEO_MAP :75-88, TEX_MAP :90-127), selfapply.py (FXSELF),
  entcap.py + entapply.py (ENTRYFX), gen_ntab_entries.py, gen_pso_tab.py + gen_pso_main.c (nd_pso_tab.c, git-ignored, build.sh:114).
- Setters: nd_st.h, nd_val.h, nd_xf.h, nd_geo.h/.c, nd_tx.h; they write native records (nd_nr.h, NSTREAM1). Host units: stunit 18/18, geounit 13/13, texunit 36/36, hunit 7/7.
- Entry form: nd_draw_list(list, entry) -> nd_rec_dl_e (nd_gtab.c) -> nd_gtab_run_e (nd_walk.c) = nd_psochk_draw + nd_gtab_run. psochk line: nd_comb.c:230.
- Drec bases: gold_12_drec, spot_mac01_drec, sbk14_drec at 930de29e1; fxpic_91_drec at d29deacc3. psochk bench fxpic_20_psochk (bench.d/ENTRY1.json, log only).
  Capture benches fxent_a/b/c and fxchk_a/b/c (bench.d/ENTRYFX.json).
## Open work
- Merged: TEX4, RECROOT, NSTREAM2, NSTREAM3, GKEY2, GSET1-3, FXENT2, FXENT3 (20 sites; nd_rec_dl_k keyed form). PSOLK: check `git log` for its merge.
- Merged: GCONVW (75d442afb, ntab tables 1153). Ready page: ws_FXREST1 (start now; owns the 21 effect site files, 3ds/tools/fxrest.d/).
- Ready page: ws_FXREST2 (start now; 10 sites; psochk drops geom, plod, key_c, key_s; B0 666 replaced by B1 from its own capture, `psochk end` line).
  Gate: fxr_chk bucket-21 psochk line bad == 666 (psochk logs only the first checked draw of a 100-frame bucket, nd_comb.c:229-231), fxr_drec byte-equal to merge-base.
  FACT 3ds/tools/fxrest.d/cap.txt (column order nd_walker_desc, nd_comb.c:206-213). The tuples of one key differ in one place only:
  geom 00000205 vs 00000405: green_impact:203#0/#1, red_impact:210#0/#1, shockwave:342#2/#3. Last word high byte 0x33..0x50: misc_particles:331#2/#3/#6/#7 (GUESS plod).
  key_c d3ffd800/8ca98f00/46554800/0: ring_blast:141#0. Two real combine states (cw0 fc127e24/fc123224, oml 00551208/00504340): recover:240#0, recover:223, stat_change:274.
  kmiss: cold_breath:234 keys 2-5 and gather_energy_pink:224 keys 1,3 were not seen.
  First step: read nd_psochk.c (nd_psochk_cmp). For each varying field, decide whether it is a per-draw value (set by a setter, so drop it from the compare or the entry) or a pipeline (so add a branch key).
  Read each site's code for the key expression. chapter_change:163 stays nd_rec_dl (the user decides). The chomp_drop vtx fix is on main (b31e07f23).
- After GCONVW: GSET-W (move the joined lists; NDN_DECL(X) pastes ndd_##X, so an N(x) argument needs an expanded name, nd_gtab.h:65-66). Converter light support
  (MOVEMEM is unsup, nd_gfx_conv.c:475) decides dgb_01/gfx.c (168 lists), level_up*, starpoint* (GUESS by macro grep: their lists hold lights, rects or scissors).
  model.c (63 lists) are ND_RM_REC scomp defs (nd_rmdef.h): design before it joins.
- User decision (one part, design_lookahead 9): a draw with no pso table entry after S9DEL2A deletes the decide code.
- NOT NOW (user 2026-11-15): audit_hw2 F5 pm_mapdata.c:112-123 mallocs the map .data save at first entry (O3DS full commit: GUESS fail); fix later = boot reserve or memcheck sum.
- NOT NOW (user): psochk compares 16 of 20 NdPsoDesc fields (nd_psochk.c:12-16); geom, key_c, key_s, plod are per-draw values (FXREST2 choice). Never add them back.
- Later cheap test: gset_check (nd_gfx_conv.c:784-816) has no run-on edge check; moved lists D_09008CB0_3CA850 and D_09000B90_3FFDE0 have no END (unused today, audit_hw2 B).
## Facts that cost time
- libctru (objdump of libctru.a in pm3ds-build:1): gsp events are LightEvent RESET_STICKY; gspWaitForEvent(id, false) = wait then clear; the event thread runs gxCmdQueueInterrupt (citro3d onQueueFinish swaps) before LightEvent_Signal.
- nd_pso_build's table is uncoloured: prim 0, env 0, box_pat 0, mode5 0 (nd_decide.c:335-346). nd_comb_core reads prim/env only for cc, leg_mode never (dc_legacy writes it).
- Host gcc -m32 is x87 by default: a float decision (nd_decide.c:273) can differ from ARM VFP. Device tables need `-msse2 -mfpmath=sse` (ws_PSOLK step 5).
- nd_pso() had no device caller before PSOLK; NDN_PSO ops of v2 tables are not read on the device (only NDN_UNIT through nd_walk_state).
- entcap_rec skips lists with a v2 table (nd_walk.c:509) and state-only lists (gt_run returns at :606 first): FXENT3 adds the `entcap so` line.
- gen_gfx_syms.py scans text with no #if: a gfx source with `#if VERSION_JP` symbols (chapter_change) cannot join a set until FXENT3 item 6.
- codemod.py gives a file's single ND_PSO_DESC to every list call of that file (codemod.py:14). Never run --state or plain codemod on src/effects.
- The v1 table points into the list (nd_gfx_conv.c:138, :153, :159). GSET must drop or repoint it. nd_ntab_find is device-only (nd_gtab.c `#ifndef ND_SCOMP_HOST`).
- listcen ARRAYS_RX also counts `Gfx v[n];` locals; GFXTYPE counts every `Gfx` word.
- Core 0 runners (hud, glyph, sprite, overlay, nd_direct) build gbi macros into local buffers and run nd_walk_state on them (NS3).
## What to do first
1. Answer coder questions (SendMessage, 10 lines, file:line). Check each ready page's quoted anchors against main after the merge it waits for.
2. After `Merge PSOLK`: read its hand-back (lk, miss) and write ws_PSOCAP.
3. Answer FXREST2 coder questions (page ws_FXREST2.md).
- An uninitialised vtx field from nd_rec_data (nd_rec.c:482 does not clear the memory) makes the drec bytes depend on the stream layout. Such a gate fails only at that draw (chomp_drop, FXENT3).
