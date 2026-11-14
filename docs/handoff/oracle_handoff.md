# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main d1bc5fa39, 2026-11-14. Written by the fourth workstream oracle (ws_TEX, ws_EMIT).
## Goal
Remove the display list system completely (plan_endgame.md v3). Done only when `3ds/tools/elfgate.py` prints
`func 0 data 0 gfxnames 0` and every LISTCEN2 counter C1..C12 is 0.
FACT (`python 3ds/tools/listcen.py` at 6fbd4d6c6): C1 build 2008, C2 gfxpos 1438, C3 0, C4 0, C5 data 22103, C6 data_arrays 2108,
C7 gfxtype 2845, C8 reader 746, C9 model 15, C10 recdl 359 (effects 319, world 12, entity 10, battle 3, root files 15).
FACT (codemod.py --dry --state per dir, 6fbd4d6c6): rewrites effects 103, world 131, entity 39, evt 5, battle 9, pause 19, filemenu 19.
## Tools on main (all in 3ds/tools unless named)
- listcen.py (C1-C10, `--group`), elfgate.py (C11-C12), codemod.py (`--state` STATE_MAP :52-72, `--ntab <ntab_tables.txt>` ND_SELF,
  `--dry`), tests/codemod/run.sh (26 cases). nd_st.h/nd_st.c: 18 setters (stunit 18/18). nd_val, nd_xf (hunit 7/7).
- Entry form (ENTRY1): nd_draw_list(list, entry) -> nd_rec_dl_e (nd_gtab.c:114) -> nd_rec_dl; dl_ent set gives the 4-word record
  nd_gtab_run_e (nd_walk.c:641) = nd_psochk_draw (nd_comb.c:219) + nd_gtab_run. Draws are equal by construction; the desc is only checked.
- entcap switch: entcap_rec (nd_walk.c:501) logs `entcap l <list> s <site> d <11 words>` for a list with no v2 table; entcap.py names
  them (addr2line). 1024 keys. psochk switch prints `psochk f<n> draws <d> bad <b>` per 100 frames and `psochk bad <field> ...` (8 max).
- build3ds/out/ntab_tables.txt (922), ntab_inherit.txt (426), ntab_unsup.txt (24): written by gen_gfx_tab.sh:18-23 at each build.
- fxtest=<n,n,...> runs effects one per 44 frames from frame 400 (pm_fxtest.c:179-180), 128 effects. fxcen_c0..c7 (16 each, turbo) took
  7-21 s (FACT, runs.jsonl).
- Drec bases (bench.py cmp uses the merge-base unless `--commit`): spot_mac01_drec, sbk14_drec, gold_12_drec at 930de29e1;
  spot_mac00w_det at a0e0414d8; gold_33/55/63_drec, fxpic_91_drec at d29deacc3; fxpic_89_drec at 2e6ac0825 (FACT, runs.jsonl).
## Open work
- ws_C (branch WSCR4, coder running): root src/*.c state calls R4-R6. Owns root src/*.c, include/, nd_st.*.
- ws_ENTRY.md (branch ENTRYFX, page 2026-11-13): effects state calls + 89 ND_SELF sites (E1), entapply.py (E2), entcap capture in 3
  fxtest chunks and the entry descs (E3). Owns src/effects, entapply.py, tests/entapply, azt.d and bench.d ENTRYFX.json.
- ws_D.md (branch WSD, page 2026-11-13): state calls of world, entity, evt, battle, pause, filemenu (222 rewrites, 3 groups).
- ws_GEO.md (page 2026-11-14, not launched): G0 setters nd_geo.h/.c, geounit, codemod --geo (no blocker); G1-G3 call sites (161) after
  ENTRYFX, WSD, WSCR4 merge. design_lookahead.md: the choices for target, mesh, lights, CONV2C2, NEMIT, GSET (needs an INDEX.md line).
- ws_TEX.md (page 2026-11-14): T0 nd_tx.h + texunit now, T1 codemod --tex after `Merge GEO`, T2-T4 call sites after GEO1-3.
- ws_EMIT.md (page 2026-11-14, no blocker, Haiku): unit copy in the device index, v2 callees, no-triangle lists skip ent_gdl.
- Blocked: root-file recdl (15) and root entries: wait for ws_C R6. World/entity/battle entries (25 sites): need capture scenes per
  site (fxtest covers only effects); write after ws_ENTRY E3 merges. CONV2C2 (design_lookahead 3): after E3 and `Merge EMIT`.
  GSET (C5, C6): after EMIT, E4 (design_lookahead 4, oracle writes it next) and the set's CONV2C2. No Sonnet core is left.
  Readers C8 and the S9DEL stages: plan section 6 phase E needs C1, C2 and C10 at 0 for the deleted code first.
## Facts that cost time
- codemod.py gives a file's single ND_PSO_DESC to every nd_rec_dl/gSPDisplayList of that file (codemod.py:14, :142-144). After ENTRY E3
  many effect files have one desc: never run codemod on src/effects again unless that rule is removed first.
- explosion.c: :191 and :204 lists are self-contained (ntab_tables), :198 D_090008F0_344860 inherits (ntab_inherit.txt:117).
- A converted site calls nd_rec_dl from nd_rec_dl_e, so entcap names it as 3ds/src/nd_gtab.c:114, not the game line.
- entcap_rec skips state-only lists (nd_walk.c:604-611 return before :612) and lists with a v2 table (:505).
- codemod.py does not see `#ifdef`; C3, C4 are 0 now, so no N64 block is left. codemod adds no #include.
- C1 counts `g[DS]P...(` (listcen.py:42), C2 gMainGfxPos (:43), C10 `nd_rec_dl(` and `nd_scomp_rec(` (:47), C8 (:48-49).
  A rewrite moves C1 and C2 by exactly the codemod value: the exact gate of every group. nd_draw_list does not count in C10.
- Directory ownership: ws_C root src/*.c, include/, nd_st.*; ws_ENTRY src/effects; ws_D world, entity, evt, battle, pause, filemenu.
- codemod --geo clears MAP (ws_GEO G0), so it never touches a list call: safe on src/effects after E3. --state or plain runs are not.
- Unmapped C1 after ws_GEO: 357 on gMainGfxPos++ and 53 local (texture, rectangle, viewport, colour; design_lookahead 2). listcen counts
  src/world/dead (build 30, gfxpos 32), which maps.cmake does not build.
## What to do first
1. Answer coder questions (SendMessage, 10 lines, file:line).
1b. Write the E4 page (design_lookahead 4): read nd_ent.c ent_compile :253-340, ent_lookup :343, ent_draw :449, ent_run :482 first.
2. After ws_ENTRY E3 merges: write the world/entity/battle entry page (scenes per site from gold.py coverage) and then CONV2C2.
3. After ws_C R6 merges: write the root recdl and entry page (15 sites) with ws_C's files.
