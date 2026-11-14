# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 1149b5513, 2026-11-14. Written by the ninth workstream oracle (pages ws_PSOLK, ws_RECROOT, ws_FXENT2, ws_GSET2, ws_GSET3; design_lookahead 5, 6, 9).
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
- Running (check `git log --oneline -10 main`): GKEY1. TEX T3 merged (914805368); T4 gate failed (reg 0x1200 -4.0 vs -0.0, branch TEX4WIP), so RECROOT waits. Chain: GKEY1 -> GKEY2 -> GSET1 -> GSET2 -> GSET3 (same converter, strictly serial).
- Ready pages: ws_PSOLK (start now), ws_NSTREAM2 (after GKEY2, NSTREAM1 merged), ws_GKEY2, ws_GSET1, ws_GSET2, ws_GSET3, ws_RECROOT (after `Merge TEX4`),
  ws_FXENT2 (after `Merge NSTREAM2`).
- To write: NS3 at `Merge TEX4` and `Merge RECROOT` (sprite.c owner); PSOCAP after `Merge PSOLK` (design_lookahead 9); GCONVW after `Merge GSET3` (design_lookahead 5);
  the effects hand rows from FXENT2's build3ds/out/fxent2_rest.txt.
- User decision (one part, design_lookahead 9): a draw with no pso table entry after S9DEL2A deletes the decide code.
## Facts that cost time
- Main moves fast: run `git log --oneline -10 main` before you write a page.
- nd_pso_build's table is uncoloured: prim 0, env 0, box_pat 0, mode5 0 (nd_decide.c:335-346). nd_comb_core reads prim/env only for cc, leg_mode never (dc_legacy writes it).
- Host gcc -m32 is x87 by default: a float decision (nd_decide.c:273) can differ from ARM VFP. Device tables need `-msse2 -mfpmath=sse` (ws_PSOLK step 5).
- nd_pso() had no device caller before PSOLK; NDN_PSO ops of v2 tables are not read on the device (only NDN_UNIT through nd_walk_state).
- entcap_rec skips lists with a v2 table (nd_walk.c:506) and no-table lists never reach it: FXENT2 fixes both.
- codemod.py gives a file's single ND_PSO_DESC to every list call of that file (codemod.py:14). Never run --state or plain codemod on src/effects.
- The v1 table points into the list (nd_gfx_conv.c:138, :153, :159). GSET must drop or repoint it. nd_ntab_find is device-only (nd_gtab.c `#ifndef ND_SCOMP_HOST`).
- listcen ARRAYS_RX also counts `Gfx v[n];` locals; GFXTYPE counts every `Gfx` word.
- Core 0 runners (hud, glyph, sprite, overlay, nd_direct) build gbi macros into local buffers and run nd_walk_state on them (NS3).
- Directory ownership: chain pages own the converter, nd_gtab.*, nd_walk.c, nd_rec.c; PSOLK owns nd_comb.c, nd_decide.c, nd_psolk.c; RECROOT the 8 root files.
## What to do first
1. Answer coder questions (SendMessage, 10 lines, file:line). Check each ready page's quoted anchors against main after the merge it waits for.
2. After `Merge PSOLK`: read its hand-back (lk, miss) and write ws_PSOCAP. After `Merge FXENT2`: hand rows from fxent2_rest.txt.
3. After `Merge TEX4` and `Merge RECROOT`: write ws_NSTREAM3 (design_lookahead 8). After `Merge GSET3`: write ws_GCONVW.
