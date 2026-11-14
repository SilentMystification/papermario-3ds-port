# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main b1d32df25, 2026-11-14. Twelfth oracle: FXENT3 fault answers, FXREST2 facts (below). Eleventh oracle: ws_GCONVW. Tenth: ws_FXENT3. Ninth oracle: ws_PSOLK, ws_RECROOT, ws_FXENT2, ws_GSET2, ws_GSET3; design_lookahead 5, 6, 9.
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
## Facts that cost time
- Main moves fast: run `git log --oneline -10 main` before you write a page.
- nd_pso_build's table is uncoloured: prim 0, env 0, box_pat 0, mode5 0 (nd_decide.c:335-346). nd_comb_core reads prim/env only for cc, leg_mode never (dc_legacy writes it).
- Host gcc -m32 is x87 by default: a float decision (nd_decide.c:273) can differ from ARM VFP. Device tables need `-msse2 -mfpmath=sse` (ws_PSOLK step 5).
- nd_pso() had no device caller before PSOLK; NDN_PSO ops of v2 tables are not read on the device (only NDN_UNIT through nd_walk_state).
- entcap_rec skips lists with a v2 table (nd_walk.c:509) and state-only lists (gt_run returns at :606 first): FXENT3 adds the `entcap so` line.
- FXENT2 capture logs are gone (worktree deleted). The 28 unseen lists are inherit or unsup lists (FACT build3ds/out/ntab_*.txt of 11-14), not v2 tables.
- gen_gfx_syms.py scans text with no #if: a gfx source with `#if VERSION_JP` symbols (chapter_change) cannot join a set until FXENT3 item 6.
- codemod.py gives a file's single ND_PSO_DESC to every list call of that file (codemod.py:14). Never run --state or plain codemod on src/effects.
- The v1 table points into the list (nd_gfx_conv.c:138, :153, :159). GSET must drop or repoint it. nd_ntab_find is device-only (nd_gtab.c `#ifndef ND_SCOMP_HOST`).
- listcen ARRAYS_RX also counts `Gfx v[n];` locals; GFXTYPE counts every `Gfx` word.
- Core 0 runners (hud, glyph, sprite, overlay, nd_direct) build gbi macros into local buffers and run nd_walk_state on them (NS3).
- Directory ownership: chain pages own the converter, nd_gtab.*, nd_walk.c, nd_rec.c; PSOLK owns nd_comb.c, nd_decide.c, nd_psolk.c; RECROOT the 8 root files.
## What to do first
1. Answer coder questions (SendMessage, 10 lines, file:line). Check each ready page's quoted anchors against main after the merge it waits for.
2. After `Merge PSOLK`: read its hand-back (lk, miss) and write ws_PSOCAP.
3. Answer FXREST2 coder questions (page ws_FXREST2.md).
- An uninitialised vtx field from nd_rec_data (nd_rec.c:482 does not clear the memory) makes the drec bytes depend on the stream layout. Such a gate fails only at that draw (chomp_drop, FXENT3).
