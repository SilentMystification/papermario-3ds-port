# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main b8fcf0e17, 2026-11-14. Written by the fifth workstream oracle (ws_EMIT4, ws_SCOMP, ws_GEO and ws_TEX refresh).
## Goal
Remove the display list system completely (plan_endgame.md v3). Done only when `3ds/tools/elfgate.py` prints
`func 0 data 0 gfxnames 0` and every LISTCEN2 counter C1..C12 is 0.
FACT (`python 3ds/tools/listcen.py`, b8fcf0e17): C1 build 1328, C2 gfxpos 758, C3 0, C4 0, C5 data 22103, C6 data_arrays 2108, C7 gfxtype 2845,
C8 reader 756, C9 model 15, C10 recdl 167 (effects 127, world 12, entity 10, battle 3, root files 15).
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
- ws_EMIT (branch EMIT, coder running): E1-E3. Device entry u = the list itself, nu = n until GSET (memory: ndnu costs 73 KB).
- ws_EMIT4.md (page, closed, Haiku): after `Merge EMIT`. Entity path words through NdNDev.u, ndnu keeps NOPs, host flat test.
- ws_SCOMP.md (page, closed, Haiku, start now): 18 nd_scomp_rec defs to setter calls (entity S1, hos S2, battle S3 after `Merge GEO2`).
- ws_GEO.md: G2 (src/world src/battle) and G3 (root) start now. ws_TEX.md: T2 effects starts now, T3 after GEO2 and SCOMP, T4 after GEO3.
- CONV2C2 (design_lookahead 3): write the closed page next. Blocker: `Merge EMIT4` (same converter file).
- crystal_ball_gfx.c (7 nd_scomp_rec sites): design open. gsSPLookAtX/Y alone and gDPSetHilite1Tile/2Tile have no setter or map entry.
- Effects recdl 127: 13 multi sites (entapply skipped) and the lists no fxtest capture reached. Root recdl 15: model.c, entity_model.c, item_entity.c,
  animator.c, cam_main.c, background_gfx.c, msg.c, sprite.c, status_icons.c. Both need a page (oracle read per site kind first).
- GSET (design_lookahead 5): after EMIT4 and the set's CONV2C2. Readers C8 and S9DEL: plan section 6 phase E needs C1, C2, C10 at 0.
## Facts that cost time
- Main moves fast: run `git log --oneline -10 main` before you write a page. Pages wrote "after Merge X" for groups that had merged.
- codemod.py gives a file's single ND_PSO_DESC to every list call of that file (codemod.py:14). Never run --state or plain codemod on src/effects.
- The v1 table points into the list (nd_gfx_conv.c:138 def u, :153 unit, :159 list). GSET must drop or repoint it.
- nd_ntab_find is device-only (nd_gtab.c `#ifndef ND_SCOMP_HOST`); the host tests search ndn_dev_idx or ndn_all themselves.
- listcen ARRAYS_RX also counts `Gfx v[n];` locals; GFXTYPE counts every `Gfx` word.
- No game file writes into a static Gfx array (grep `\]\.words\.w[01] *=` in src: record buffers and imgfx heap buffer only).
- Directory ownership: GEO G2 src/world src/battle, TEX T2 src/effects, SCOMP its 8 files, EMIT/EMIT4 the converter, nd_gtab.*, nd_walk.c, nd_ent.c.
## What to do first
1. Answer coder questions (SendMessage, 10 lines, file:line).
2. Write the CONV2C2 page (design_lookahead 3) so it is ready at `Merge EMIT4`.
3. Design crystal_ball_gfx.c, then the root and effects recdl pages.
