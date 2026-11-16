# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main e91cff3feb, 2026-11-16. Oracle DL2 (display list work) wrote ws_CGFX, ws_DLSITE1, ws_MODELRM and the GSETR addendum.
Goal: remove the N64 display list system. Done when 3ds/tools/elfgate.py prints func 0 data 0 gfxnames 0 and every listcen counter is 0.
## Counters (FACT, listcen on main e91cff3feb)
build 722, gfxpos 201, data 3016, data_arrays 277, gfxtype 885, recdl 15, reader 785, model 15.
## Running and ready work
- GSETR (worktree build3ds/wt/GSETR, HEAD a6b5c569f3 WIP): curtains.c left gset.d only. Pass values: `gset moved 26 files 9 keep 0`;
  listcen vs build3ds/out/gsetr_lc0.txt: data -212, data_arrays -26, gfxtype -27. Steps 3-5 not run; the orchestrator relaunches it.
  Merge note: main has the ws_GSETR.md addendum, the branch deletes the page: resolve by delete.
- ws_CGFX and ws_DLSITE1: after Merge GSETR, parallel. ws_MODELRM (Sonnet): after Merge CGFX (shared 3ds/src/nd_rec.c).
- Oracle DL3 pages, start now, parallel, Haiku: ws_GBIHDR (+ _code: gbiprune.py, build -156 data -193 gfxtype -32), ws_HOLD1 (data -61),
  ws_PRA31 (gen_gfx_syms.py Matrix4s, data -311 if 36 move). Waiting: A(x) after Merge GSETR and PRA31 (gset.py:30-31, gen_gfx_syms.py:14);
  npc_merlee after Merge GSETR (gset.py); tst_13 after Merge MODELRM (model.c:4326); FXSEED (Sonnet) needs the user's yes.
## Facts that cost time
- gset.py holds a line with `&X`, `X[`, sizeof, BranchList or SetCustomGfx (hold_line, gset.py:158-160). A held caller fails gset_check
  (nd_gfx_conv.c:917-920) for every moved callee. BLOCK_OPEN (gset.py:31) needs `[] = {`: only src/curtains.c:143 has `[] ={`.
- curtains.c needs a row: `&TheaterGfx` (:276), `&TheaterInitGfx` (:257, :301), `&NoControllerSetupTexGfx` (:302) are held.
- codemod dry: `python 3ds/tools/codemod.py --dry --state --ntab build3ds/out/ntab_tables.txt src`. `dl` = rewritten + kept DL sites.
  Main: 139 dl, 123 DL keeps (110 gSPDisplayList no-entry, 7 first-arg, 6 nd_rec_dl no-entry). "no-entry" = no ND_PSO_DESC in the file
  and the list is not in ntab_tables.txt (codemod.py:200-205, :291-294). Main no-entry by group: entity 21, screen_overlays 16, world/area_hos 9,
  curtains 5, battle/common 5, entity_model 4, world/area_mac 4, effects/flame 4, others 1-3. Most use a variable list (dlist, gfx, ENTITY_ADDR).
- Scenario and bench defs go in per-row files 3ds/tools/azt.d/<ROW>.json and bench.d/<ROW>.json (example MESH1). bench.py base builds the merge-base.
- nd_rec_list (nd_rec.c:528) records a static SetCustomGfx list as commands. nd_rec_dl (nd_gtab.c:108) finds a moved key by nd_ntab_find.
- nd_lkey.h: a reader of a moved key (NDN) reads e->u. lkey census allow file 3ds/tests/lkey/allow.txt.
- Windows python cannot open /c/... paths: pass `cygpath -m` paths.
## FXSEED (oracle FX1, main 93a5a7bd50; detail build3ds/out/fxseed_62.txt in the main folder)
- FACT: 62 lists, 552 data, 62 arrays in assets/us/effects/gfx stay, 13 sources. ws_LCNOBUILD drops the 2 unbuilt ones (-20 data, -2 arrays).
- Gap a (Sonnet): quizmo_stage 7 lists "lit draw after a light unit" (nd_gfx_conv.c:554, LIT1 refused lit draws); 24 more quizmo lists are
  not gset ok through call edges (gset_check nd_gfx_conv.c:899-920). Design needs the device lit-draw path. No page yet.
- Gap c: page ws_FXREN (+ patch), after Merge AXNS. FACT oracle host run: the 6 fire_breath/peach_star_beam lists (with X_body) become
  gset ok, no unsup line. Next: a move page for them (src wrapper blocks in src/effects/gfx, FXHOLD shape).
- Gap f: page ws_FXHOLD (+ patch), start now. FACT oracle run: listcen data -24, data_arrays -3, gfxtype -4; lkey census ok.
- Gaps b, d, e wait: b tile size on a caller texture (8 lists), d RARE/SCISSOR/FILLCOLOR/RECT ops, e flame render to texture.
## Other families (from oracles 31-32, still open)
- A(x) namespace (kzn2 lava_piranha, sam2 dlist): readers, wait for the LKEY census.
- npc_merlee.c is #included by dro_02/npc.c:53: gset needs an include-twin rule (gset.py edit).
- pra_31 stairs_skele.c: animator skeleton nodes, MESHST-like design.
- tst_13 main.c:301: mdl_project_tex_coords (model.c:4326) writes into N(shockwave_gfx): a list writer, after MODELRM.
- C7 effects: 50 arrays read by a local `Gfx* dlist = X[i]` (FXTYPE next step: local handle rule).
## Open (non display list)
- File select flip lag and pause pane flip: draw_box ROTSCALE off-screen windows (draw_box.c:760-830). FACT hw_menu1_log.txt: 9 full images
  per flip frame (f6560-f6569), band clear about 376 KB a frame (pm_gpu.c:2233). OPEN: per-frame time (needs a console prof capture). Next name FSELLAG3.
- Bug oracle 38: ws_STALEOBJ (+ _code) ready: build.sh sync kept host mtimes, so a worktree build without PM_LANE left older objects in
  the shared volume; build 5877 lacks STARTLR2 and MOON7 (FSKIP is in). FSKIP2 waits for the user's row choice.
- With logging off, event lines reach no log.txt (pm_log.c:600, :888). Not fixed.
## What to do first
1. Answer coder questions of GSETR, CGFX, DLSITE1, MODELRM (SendMessage, file:line).
2. After Merge GSETR: re-run the codemod dry on main; write DLSITE2 for the next group with a static list name.
