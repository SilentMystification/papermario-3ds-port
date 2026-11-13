# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main b2442aa97, 2026-11-13. Written by the first workstream oracle.
## Goal
Remove the display list system completely (plan_endgame.md v3). Done only when `3ds/tools/elfgate.py` prints
`func 0 data 0 gfxnames 0` and every LISTCEN2 counter C1..C12 is 0.
FACT (listcen.py at b2442aa97): C1 build 3612, C2 gfxpos 3042, C3 build_n64 272, C4 gfxpos_n64 280, C5 data 22103, C6 data_arrays
2108, C7 gfxtype 2845, C8 reader 732, C9 model 15, C10 recdl 361. ELF last measured: func 50, data 4152, gfxnames 1806 (HANDOFF.md).
FACT (codemod.py --dry src): value 1605, keep 81, state 1515, dl 496 (nd_rec_dl 321, gSPDisplayList 175). Keep reasons: no-entry
477, first-arg 76, ctx 17, macro 5, stmt 2. Root src/*.c: value 325, state 899.
## Tools on main (all in 3ds/tools unless named)
- listcen.py (C1-C10; `--group <prefix>` per file), elfgate.py (C11-C12), codemod.py (7 value shapes, `--dry`), tests/codemod/run.sh.
- nd_host_test.sh: the host tests in docker; host.d/*.sh run last in the same block (/tmp/ndcomb.o, /tmp/ndlogs.c exist there).
- bench.py base/run/cmp (drec benches: gold_*_drec, fxpic_*_drec, spot_mac01_drec, sbk14_drec; spot_mac00w_det on CODEMODW1).
- gold.py (coverage verdict), covsites.py, entcap.py (branch ENTRYCAP), unifdef3ds.py and ppcmp.sh (N64DEL), memcheck.py.
- gen_pso_tab.py scans ND_PSO_DESC names in src (a duplicate name is an error). psochk switch: nd_comb.c:219, line every 100 frames.
## Open work (pages written 2026-11-13; each page names its start branch)
- ws_HUNIT.md: branch CODEMODW1. nd_xf.c to nd_rec_state, new nd_val_test.c (macro stream vs native, walker log, nd_r11_state).
- ws_A.md: branch WSA after HUNIT. codemod on src/effects (4 groups, v 992), then world, entity, pause/filemenu/battle/evt after N64DEL2.
- ws_B.md: CONV2F gate (branch CONV2F), ENTRYCAP gate (branch ENTRYCAP), then ENTRY1 entry form (nd_rec_dl_e, nd_gtab_run_e, psochk).
  Stops after group 3. Next oracle task: the ENTRY group page (426 inheriting lists) and later the GSET page (C5, C6).
- ws_C.md: N64DEL2 (branch N64DEL2, gate only), N64DEL3, N64DEL4, then nd_st setters + codemod --state + root groups R1-R6.
- design_state_calls.md: the setter idiom per state shape (orchestrator adds its INDEX.md line).
- Other branches: CONV2MIX (ahead 1; plan says MIX closed, its 16 lists stay v1: orchestrator decides delete or merge),
  SEEDPROBE1 (cancelled: delete), BOTHUD (on hold, user), FPS60FL (60 fps freeze, open), FSELLAG (console check owed).
## Facts that cost time
- The macro path records the typed unit into the open NDC_STATE run (nd_rec.c:442-459). main's nd_val.c and nd_xf.c record NDC_NAPI
  records instead. nd_r11_walk (nd_model.c:274-299, :296 `default: return 0;`) rejects NDC_NAPI in a model custom gfx range: the
  mac_00 water cause (code read). CODEMODW1 03d1bfe66 moves nd_val.c to nd_rec_state. nd_xf.c still uses NAPI (HUNIT fixes it).
- 12 effect files already use nd_val/nd_xf on main (CODEMODFX1-11). Their drec gates passed with the NAPI form.
- codemod.py does not see `#ifdef TARGET_3DS`: it rewrites calls in N64 blocks too. src/effects has no TARGET_3DS line (N64DEL1).
  The other subdirs have N64 blocks until N64DEL2 merges, root files until N64DEL3: so ws_A groups 5-7 wait for N64DEL2.
- codemod.py adds no #include: the coder adds nd_val.h, nd_xf.h (and nd_st.h) after the last include.
- Directory ownership: ws_A = src subdirs (value calls), ws_B = 3ds/src gtab files, nd_gfx_conv, explosion.c pilot,
  ws_C = include/, root src/*.c, nd_st.*, codemod STATE_MAP. codemod.py is shared: CONV2F (ws_B) edits it first, ws_C after.
- N64DEL2 must not merge main before its gate: the row diffs against 6fce31e6f.
- C1 counts `g[DS]P...(` (listcen.py:42), C2 every gMainGfxPos (:43), C8 NDO_ and G_ words in 3ds/src (:48). A value or state
  rewrite moves C1 and C2 by exactly the codemod count: the exact gate of every group.
- nd_rec.c has never been linked on the host before HUNIT. Its doubles are listed in ws_HUNIT.md (code read, not linked yet).
## What to do first
1. Read SendMessage questions from coders; answer in 10 lines with file:line.
2. After HUNIT merges: confirm `hunit ok 7/7` and the CODEMODW1 cmp lines in runs.jsonl; then ws_A can start.
3. After ws_B group 3: write ws_ENTRY.md (entcap capture per directory, ND_PSO_DESC names pe_<file>_<n>, psochk bad 0).
4. After ws_C R6: write the readers and recdl page (C8, C10).
