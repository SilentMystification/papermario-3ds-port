# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 48cb964cd4+, 2026-11-17. Counters FACT (listcen on b4de08e55b): build 311, gfxpos 3, recdl 5, reader 792, model 15; page moves since:
OPMOVE reader -62, FXSLOT2 recdl -5, FLAMEDL build -2 gfxpos -2, TST13 build -1 gfxpos -1, PSOFB model -2. Run listcen for fresh numbers.
## The chain: plan_walker_delete.md (stages; S3-S6 rows updated) and design_nstate.md (the S3-S6 design). Read both first.
- D1 A, D2 A. UD1 option 1 (user): core 1 keeps the N64 field model; listcen reports a class core1 outside the end proof.
  UD2 option 1: core 0 keeps a native transform stack. UD3 option 1: prof push at the NST1 gate. Sonnet yes: NST1, NDR0, NCONV1, NREC1.
- User 2026-11-16: a table miss uses a fixed fallback and one log entry (PSOFB). Option B 2026-11-17: gbi.h etc. stay until S7.
- HWFIX1 merged (nd_comb_core on a miss); user console check of 0f39de90cb: visual faults gone (FACT). TATTLEAR merged.
## Pages on main and their waits (chain: NST1 -> NST3 -> NST2A -> NST2B -> NST2C -> NST2D -> NST2E -> NST2F -> NDR0)
- Merged: PSOFB, NST1, NST3, NST2A, HWFIX1, BOTSCR. NdDs holds lt[], lk, vp_*, tm. dschk switch and dschk_base.txt exist.
- NSTO pages (oracle NSTO): NST2B model records (nd_ds_model; dschk adopts the groups a model cache hit skips), NST2C entity draw, sprite
  state, compiled def, particles (nd_ds_unit, nd_ds_feed), NST2D ranges (unit log of core 1 applies, marks, sub save/restore, R11 replay;
  dschk skips compares inside replays), NST2E the 6 direct-draw runners feed their fallback units, NST2F adopt after direct draws, box fold
  unfold values, shaded sprite dual_on on core 1; S3 end gate bad 0 epbad 0. Every NST2 row replaces fxrest.d/dschk_base.txt.
## Open work (in order)
1. Answer coder questions (HWFIX1 first, NST1, NST3, NST2A-F). epbad: core 0 bumps gv_epoch but not nd_st_epoch at
   every model record (nd_walk.c:156); fix is ws_NST2B edit 7. Other prologues without nd_st_epoch (nd_walk.c:184 ENT) count once their unit feeds ds_ep.
2. PSOFB psomiss: tex_on 2167585 = 0x211321 (f5d196a337 text). No cause by code read: model input is bool (nd_model.c:335), so nd_ws.tex_on. Writers: set_texture nd_tex_set.c:223 (via nd_op_apply nd_set.c:172, nd_scomp.c:195, :248) and nd_recs.c:195 (u8).
   Every NDO_TEXTURE encoder masks a to 0/1 (nd_op.h:89, nd_nr.h:15, nd_op.c:29). So a non-unit word with top byte 0x2A ran as a unit.
   GUESS paths: stale def units nd_scomp.c:241-248 (ent_sdef nd_ent.c:213 points into a heap list), stale nd_cgfx_rng range (nd_recs.c:99).
   OPEN: needs a user yes for one debug-log run (log unit pointer and pm_gbi_cur_op in set_texture when on > 1). The gate logs are gone.
3. NDR0 (Sonnet, design section 7) after the NST2F gate data. Reads it needs: gv_epoch_enter nd_geom.c:203-228, sync_combine_work
   nd_comb.c:312-333 (leg_mode and comb_mode5 are decision inputs and outputs), sync_render nd_set.c:210, ensure_tile_slow nd_tex_set.c:335,
   box_fold_sync nd_comb.c:374, nd_mesh_* are VTX/TRI state-run units (nd_geo.c:29-37), not commands. Then NCONV1 (section 8), NREC1 (9).
4. Add the listcen class core1 (UD1) in the first row that adds ND_FS_ ids (NCONV1).
## Facts that cost time
- Every setter unit passes nd_rec_nr (nd_rec.c:375); core 0 runs it in nd_walk_state (nd_walk.c:390). reader falls only in S5-S7.
- The F3 globals (conv_k4, dual_on, comb_set ...) have struct-field and parameter twins in 20+ files: never rename them by sed.
- Core 0 never resets nd_ws (nd_state.c:11 static init only): nd_ds must not reset either.
- fxslot_cap psochk (6ff72ad823): lk 5185 miss 26091 lkbad 0. Most decisions miss the table today. Keys come only from ND_PSO_DESC.
- psochk scenes (azt.d debug): fxpic_20_psochk, fxchk_a/b/c, fxr_cap, fxr_chk, iseed_chk, fxcull_drec, fxdig_drec, miscp_drec, imgovl_*.
- cap.txt is the FXREST1 capture (geom and plod in the tuple). Use cap2.txt: entcap zeroes geom, key_c, key_s, plod (nd_walk.c:537).
- Loop lists: a list drawn more than once in a loop can enter later draws with other state (MISCP 1b, 7b25b8b3af). leg_prev too
  (nd_comb.c:214, :183, :306). Give 2 descs (first draw, later draws) when the capture shows 2 tuples. Shimmer `multi 2`: MISCP 1b form.
- conv_k4 (w7) is sticky: 0x7f in multi-effect runs, 0 in a one-effect run (FXDIG). psochk compares it.
- entcap logs a list with a v2 table only with the switch entcapt (nd_walk.c:533, :644). NDN(x) lists all have one (ndd_x).
- entapply edits every site of its input: filter the entcap.py output first. It needs a plain `nd_rec_dl(...);` (entapply.py:32).
- bench.py run does not build (bench.py:1218-1225): `sh 3ds/build.sh` before a capture run.
- A new debug switch of a gate scene must be on main before the gate row (the base is the merge-base).
- fxtest index = FXLIST line - 35 (pm_fxtest.c:35 is index 0; flame :62 = 27). host.d scripts run only through nd_host_test.sh. PM_HOSTSEL=all on rowloop lines.
- lkey_cen.py:71-72 fails a listed file whose reader count drops: a page that removes `.words.w*` reads edits allow.txt.
- Hooks: SendMessage text must not hold the words "log line" (no_guess_hook.js). No sleep loops (no_poll_hook.js).
- Other oracles edit this file and the orchestrator commits stray files: read `git log -- <file>` and the file before a write.
- Core 0 leaves state that core 1 cannot follow: a model cache hit (nd_recs.c:198) skips lights, tile/load units, panner sizes, filter bits;
  a direct draw (nd_direct.c:103-116) writes pso state and binds without tile ops. dschk adopts core 1 values there (NST2B, NST2F) and counts adiff.
- dschk copies nd_ds at rec_take of a command: every core 1 apply of a record goes after its nd_rec_* call.
## Console reports 2026-11-17 (build 6449 = main 60182a2edd, FACT, rule 1: code read only)
- 60 FPS row: paged ws_FPS60MENU (9ef4f1fd8d): git revert of FSKIP2 1e7ddd7ea1 (row renamed to "Frame skip", pm_options.c:421 :675).
- OPEN, page ws_OPTIONS not written: (1) bottom screen blank sometimes; (2) some Select menu option rows change nothing.
  Facts read so far: bottom console init pm_gpu.c:1902 (consoleInit GFX_BOTTOM, window LOG_TOP), only after C3D_Init; C3D_Init failure path :1799.
  Bottom draw: pm_gpu_hud_flush (pm_gpu_ui.c:155-195) returns while menu_on or !consoleInitialised. Menu draw pm_gpu_ui.c:54-74 (consoleClear at close).
  Only gfxFlushBuffers call: pm_gpu.c:1925 (menu_grey_top path). The top swaps at pm_gpu.c:1973 and pm_main.c:90; no bottom swap call (grep).
  Log levels: pm_log.c:353-383 (livelevel, disklevel, conlevel; conlevel off = no log text on the bottom, GUESS). Options defaults pm_options.c:14-18.
  Rows: tab_ids pm_options.c:418-445, labels :639-690, values :692+, flips pm_options_flip :298-335 and the act switch near :784 and :886.
  To do: list each row id, its pm_opt field and every reader of that field (grep), git log -S on the field names over 002498da73..main.
## What to do first
0. Write ws_OPTIONS from the OPEN block above (coordinator brief: switches prof and log profiles, GPOS1, NST hooks, TATTLEAR).
1. Read design_nstate.md, plan_walker_delete.md and the ws_NST2* pages. Answer coder questions.
2. Open work 2, then 3.
