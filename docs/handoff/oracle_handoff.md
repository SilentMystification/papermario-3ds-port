# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main d3cbe34bb3, 2026-11-17. NEXT3 wrote ws_MDLTYPE, ws_BARRICADE. DESIGN3 wrote ws_GPOS1, ws_FXCULL, ws_MISCP, ws_FXSLOT1,
ws_TST13, ws_IMGOVL. Merged since NEXT2: BLANKET, FLTGT, GTSMALL, FXDIG, VCFIX.
Counters FACT (listcen on 3b890c6d7f, after FLTGT): build 312, gfxpos 8, data 191, data_arrays 0, gfxtype 284, recdl 11, reader 792, model 15.
Goal: elfgate.py prints func 0 data 0 gfxnames 0 and every listcen counter is 0 (include/PR/gbi.h definitions go in stage 4D).
## Pages on main (Haiku, disjoint files)
- ws_IMGFXT: gfxtype -9 (imgfx.c, imgfx.h, common_structs.h). Coder running.
- ws_MDLTYPE (NEXT3): gfxtype -16 (model.c, model.h, functions.h, HiddenPanel.c:10, tst_13 main.c:6).
- ws_BARRICADE (NEXT3): gfxtype -6, host test barricade.sh.
- ws_GPOS1: gfxpos -4, gfxtype -2. Deletes the 3 task-start gMainGfxPos writes (value is always nd_rec_top), moves the definition
  to game_stubs.c (host tests define their own: not nd_rec.c), nuGfxTaskStart(ucode, flag). Gate spot_mac01_drec.
- ws_FXCULL: recdl -3 (green_impact, red_impact, shockwave: one desc per site from cap2.txt). New bench fxcull_drec.
- ws_MISCP: recdl -1 (misc_particles keyed switch on variation, 6 descs from cap2.txt). New bench miscp_drec.
- ws_FXSLOT1: fxslot=<n> switch, fxtest entry star_shimmer (153), warptest switch (pm_warp.c:846), shimmer line -> nd_rec_dl,
  capture fxslot_cap -> fxrest.d/cap3.txt (5 sites). build -1, gfxpos -1, recdl +1.
- ws_TST13 (after MDLTYPE, FXSLOT1): tst_13 main.c:306 capture (warptest) and entry. build -1, gfxpos -1.
- ws_IMGOVL (after IMGFXT): imgfx.c overlay nd_rec_dl capture in the monstar battle, entry. recdl -1.
## Open work
1. FXSLOT2 page (after FXSLOT1 merges and cap3.txt exists): `python 3ds/tools/entapply.py 3ds/tools/fxrest.d/cap3.txt` gives 5
   sites (chomp_drop 2, disable_x 2, shimmer 1); add ntab_ent.d/FXSLOT2.txt with src/status_star_shimmer.c (src root file);
   gate fxslot_drec (already in FXSLOT1) byte-equal, psochk bad equal. recdl -5. A multi line: design a key from cap3.txt.
2. After GPOS1, FXSLOT2, TST13: gfxpos 0 and src build 0 (FACT, listcen.py:291-292 scan src and include only). gMainGfxPos then
   lives in 3ds/src (nd_rec.c stray_word/nd_gx_w redirect, nd_gtab.c:127 fallback, nd_cen.c census) and variables.h:111.
   It leaves with the nd_gbi.h redirect (stage 3). Host tests nd_geo/st/tx/val/at_test.c use the macro form on gMainGfxPos.
3. USER QUESTION sent to main (stage 4D): option A move gDP/gSP/gs definitions and Gfx out of include into a walker-only header
   (build about -300, data -191 by scan set; listcen must then count that header) or option B keep the headers until the walker goes.
4. reader 792, model 15, walker gfxtype in 3ds/src: Phase E. Every static list still runs its NDN_UNIT words on the walker
   (nd_gtab.h:104, CONV2D2); no native emit from NdPsoOut exists. That is a Sonnet design core, not a Haiku page.
## Facts that cost time
- fxrest.d/cap.txt is the FXREST1 capture: geom and plod were still in the tuple (old "multi 2" and "multi 30"). cap2.txt is
  the FXREST2 capture: entcap zeroes geom, key_c, key_s, plod (nd_walk.c:537); psochk skips them (nd_psochk.c:12-16). Use cap2.
- conv_k4 (w7) is sticky state from earlier draws: 0x7f in multi-effect runs, 0 in a one-effect run (FXDIG). psochk compares it.
- entcap logs a list with a v2 table only with the switch entcapt (nd_walk.c:533, :644). NDN(x) lists all have one (ndd_x).
- entapply edits every site of its input file: filter the entcap.py output to the row's sites first. It needs the plain line
  `nd_rec_dl(...);` with no trailing comment (entapply.py:32). gen_ntab_entries.py:17 reads only a named desc (no `*e`).
- bench.py run does not build (bench.py:1218-1225): build the lane with `sh 3ds/build.sh` before a capture run.
- A new debug switch used by a gate scene must be on main before the gate row: the base is the merge-base (FXSLOT1 then FXSLOT2).
- shockwave_main maps every type to unk_00 2..5 (shockwave.c:31-43): D_E004AE60[0..1] never run.
- fxtest index = FXLIST line - 33 (pm_fxtest.c). Slot 44, start 400, spawn at k 2, clear at k 40 (slot_len - 4 after FXSLOT1).
- host.d scripts need gcc: run them only through nd_host_test.sh. PM_HOSTSEL=all on rowloop lines (host cache fault).
- lkey_cen.py:71-72 fails a listed file whose reader count drops: a page that removes `.words.w*` reads edits allow.txt.
- Coders often hand back before the oracle reply arrives. Tell the orchestrator to resume them with the reply.
## What to do first
1. Answer coder questions (GPOS1, FXCULL, MISCP, FXSLOT1, TST13, IMGOVL, MDLTYPE, BARRICADE, IMGFXT).
2. Write ws_FXSLOT2 when cap3.txt is on main.
3. Read the user's answer to the stage 4D question before any include page.
