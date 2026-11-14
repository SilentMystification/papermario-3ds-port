# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main a37368582, 2026-11-14. Written by the eighth workstream oracle (NSTREAM: design_lookahead 8, ws_NSTREAM1, ws_NSTREAM2).
## Goal
Remove the display list system completely (plan_endgame.md v3). Done only when `3ds/tools/elfgate.py` prints
`func 0 data 0 gfxnames 0` and every LISTCEN2 counter C1..C12 is 0.
FACT (`python 3ds/tools/listcen.py`, a37368582): C1 build 1166, C2 gfxpos 617, C3 0, C4 0, C5 data 22040, C6 data_arrays 2075, C7 gfxtype 2812,
C8 reader 757 (per file: design_lookahead 7), C9 model 15, C10 recdl 145.
FACT (`codemod.py --dry --geo`, b8fcf0e17): src/world src/battle value 26, root src/*.c value 78.
## Tools on main (all in 3ds/tools unless named)
- listcen.py (C1-C10, regexes :42-49), elfgate.py (C11-C12), codemod.py (MAP :38-49, STATE_MAP :53-73 `--state`, GEO_MAP :75-88 `--geo`, TEX_MAP
  :90-127 `--tex`, `--ntab`, `--dry`), tests/codemod/run.sh (45 cases). Setters: nd_st.h, nd_val.h, nd_xf.h, nd_geo.h/.c, nd_tx.h (header only).
  Host units: stunit 18/18, geounit 12/12, texunit 35/35, hunit 7/7, entapply 1/1.
- Entry form: nd_draw_list(list, entry) -> nd_rec_dl_e (nd_gtab.c) -> nd_gtab_run_e (nd_walk.c) = nd_psochk_draw + nd_gtab_run.
- entcap switch: entcap_rec (nd_walk.c:501) logs lists with a v1 table and no v2 table, at nd_gtab_run only. entapply.py writes the sites.
- build3ds/out/ntab_tables.txt, ntab_inherit.txt, ntab_unsup.txt: written by gen_gfx_tab.sh at each build.
- bench.py base <bench> builds the merge-base once when the store misses it (basecache keeps 5 commits). Drec bases: gold_12_drec and
  spot_mac01_drec, sbk14_drec at 930de29e1; fxpic_91_drec at d29deacc3. Shot det benches for state defs: ent_kmr03_det, sprshade_hos01_pic,
  battle_wheel_nowide_det.
## Open work
- Merged: EMIT (E1-E3, margin 322 KB), GEO (G0-G3), TEX2. Running: EMIT4, SCOMP, CBALL (check `git log --oneline -10 main`).
- Chain (same converter and test files, strictly serial): EMIT4 -> ws_CONV2C2 -> ws_GKEY1 -> ws_GKEY2 -> ws_GSET1 (tool and pilot fright_jar). All closed, Haiku.
- Parallel: ws_FXSELF after CONV2C2 and TEX2; ws_TEX T3 after CBALL, T4 after GEO3 (merged).
- Blocked (design_lookahead 6): effects recdl rest, root recdl 15 (page after `Merge TEX4`, ws_SCOMP rule).
- GSET rows after GSET1: one gset.d file each (design_lookahead 5). src/entity needs a block move in gset.py (arrays in the .c text): page after GSET1.
- Phase E (design_lookahead 7): no stage can start. NSTREAM designed (design_lookahead 8): the record keeps the typed unit bits (equal by construction).
  ws_NSTREAM1 starts now (setters, new nd_nr.h). ws_NSTREAM2 after `Merge GKEY2` and `Merge NSTREAM1` (NDC_NAPI run, nd_nr_run, nd_nr_exec).
  NS3 (core 0 local buffers, emit-argument form of nd_tx.h): write its page at `Merge TEX4`. Stage 2 core (pso lookup) is not Haiku (Sonnet needs the user).
## Facts that cost time
- Main moves fast: run `git log --oneline -10 main` before you write a page. Pages wrote "after Merge X" for groups that had merged.
- codemod.py gives a file's single ND_PSO_DESC to every list call of that file (codemod.py:14). Never run --state or plain codemod on src/effects.
- The v1 table points into the list (nd_gfx_conv.c:138 def u, :153 unit, :159 list). GSET must drop or repoint it.
- nd_ntab_find is device-only (nd_gtab.c `#ifndef ND_SCOMP_HOST`); the host tests search ndn_dev_idx or ndn_all themselves.
- listcen ARRAYS_RX also counts `Gfx v[n];` locals; GFXTYPE counts every `Gfx` word.
- No game file writes into a static Gfx array (grep `\]\.words\.w[01] *=` in src: record buffers and imgfx heap buffer only).
- nd_rect, nd_quad, nd_glyphs, nd_mesh exist only as plan names (plan_nativeapi.md:20-21). Core 0 runners (hud, glyph, sprite, overlay, nd_direct) build gbi
  macros into local buffers and run nd_walk_state on them: a core 1 setter cannot serve them (design_lookahead 8, NS3).
- Directory ownership: GEO G2 src/world src/battle, TEX T2 src/effects, SCOMP its 8 files, EMIT/EMIT4 the converter, nd_gtab.*, nd_walk.c, nd_ent.c.
## What to do first
1. Answer coder questions (SendMessage, 10 lines, file:line). Check GKEY1/GKEY2/GSET1 text against main after each merge of the chain (anchors quoted there).
2. After `Merge GKEY2`: check ws_NSTREAM2 quoted anchors against nd_rec.c and nd_walk.c. After `Merge TEX4`: write ws_NSTREAM3 (design_lookahead 8;
   read hud_element.c:540-616, sprite.c spr_flat_gfx, msg_draw.c:1766, screen_overlays.c:244, nd_direct.c:263, :332 for the shapes they use).
3. After `Merge GSET1`: the gset.py block move for src/entity and the next gset.d rows. After `Merge TEX4`: the root recdl page.
