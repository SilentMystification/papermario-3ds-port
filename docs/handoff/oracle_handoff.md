# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main b0763f2abf+, 2026-11-17. Counters FACT (orchestrator brief): build 308, data 191, gfxtype 251 (246 on branch GFXT1), reader 772,
model 15, elfgate func 48 data 365. Run listcen for fresh numbers.
## The plan: plan_finish.md (15 waves, 47 Haiku rows, width and stop file per wave), design_nstate.md (S3-S6 design). Read both first.
- D1 A, D2 A. UD1 option 1 (user): core 1 keeps the N64 field model; listcen reports a class core1 outside the end proof.
  UD2 option 1: core 0 keeps a native transform stack. UD3 option 1: prof push at the NST1 gate. Sonnet yes: NST1, NDR0, NCONV1, NREC1.
- User 2026-11-16: a table miss uses a fixed fallback and one log entry (PSOFB). Option B 2026-11-17: gbi.h etc. stay until S7.
- HWFIX1 merged (nd_comb_core on a miss); user console check of 0f39de90cb: visual faults gone (FACT). TATTLEAR merged.
## Pages on main and their waits (main bc5e681339: NST2A-F, FALLBK, NDR0A, NCONV1A merged; no console check until S9GATE)
- Merged: PSOFB, NST1, NST3, NST2A-E, HWFIX1, BOTSCR. Done on branches: GFXT1, PSOCOV, BASELOG, OPTIONS (6efc8f900d).
- NSTO pages (oracle NSTO): NST2B model records (nd_ds_model; dschk adopts the groups a model cache hit skips), NST2C entity draw, sprite
  state, compiled def, particles (nd_ds_unit, nd_ds_feed), NST2D ranges (unit log of core 1 applies, marks, sub save/restore, R11 replay;
  dschk skips compares inside replays), NST2E the 6 direct-draw runners feed their fallback units, NST2F adopt after direct draws, box fold
  unfold values, shaded sprite dual_on on core 1; S3 end gate bad 0 epbad 0. Every NST2 row replaces fxrest.d/dschk_base.txt.
## Open work (in order)
1. Answer coder questions (HWFIX1 first, NST1, NST3, NST2A-F). epbad: core 0 bumps gv_epoch but not nd_st_epoch at
   every model record (nd_walk.c:156); fix is ws_NST2B edit 7. Other prologues without nd_st_epoch (nd_walk.c:184 ENT) count once their unit feeds ds_ep.
2. Non-list walk (FACT, NST2D wscmb base variants): each frame the walker runs bss as units, past t_blk0 (pm_dspaud.c:52, a tick)
   and pm_f127 (pm_gpu.c:1725) (base ELF basebin/07b0785e17cf38b8). The tick top byte picks the unit: 0x39 MTX (variant a, stored base),
   0x2D COMB / RARE (variant b: key_s 0x100, sticky, all pso lookups miss). GUESS: psomiss tex_on 0x211321, 0x421862 = a tick with top byte 0x2A.
   Producer FACT: pm_gbi_fallback dl (nd_walk.c:770, raw gbi words, no NDO_END; caller pm_main.c:464). Fix page ws_FALLBK (2ac5fcf03a).
3. FIN pages on main: NDR0B, RENFS (REN1-5), NDR0C, NDR5, NST2G. NCONV1B and C dropped (plan_finish.md section 4). Next: NDR0D after NDR0B
   and NDR0C data (tile0_white nd_comb.c:354 on core 1 td[0], box_fold_sync :372; core 1 miss path: nd_comb_core writes pm_comb_vprim
   nd_decide.c:234, pass vprim instead). NST2G names drive NST2H if bad or epbad stay above 0. dschk_base.txt before NST2G predates FALLBK.
4. Write each page from the snapchk and dschk names of the row before it. NDR0E is Sonnet only if the batch cut moves (plan_finish.md section 4).
## Facts that cost time
- Every setter unit passes nd_rec_nr (nd_rec.c:399); core 0 runs it in nd_walk_state (nd_walk.c:390). reader, model, data, build fall only in S4-S7 (no row free of chain files: nd_cen.c callers are in nd_walk.c, nd_recs.c).
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
- Option rows: ws_OPTIONS written (79a07c54a5). Its edit is done on branch OPTIONS (6efc8f900d, host speedx2 ok), not merged to main.
- Bottom screen blank: BOTSCR merged (pm_log.c:905 cause).
## What to do first
1. Read plan_finish.md, design_nstate.md and the ws_NST2* pages. Answer coder questions.
2. Write NDR0 when the NST2F gate passes (Open work 3). Then the S4 pages of plan_finish.md section 2 (NDR1-6, NCONV1).
