# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 1a014d6057, 2026-11-18. Counters FACT (listcen 8e5ad5a5e2): build 306, data 191, gfxtype 230, reader 496 (REN8 -79 gated), model 11, core1 363.
Run listcen for fresh numbers.
## The plan: plan_finish.md (15 waves, 50 Haiku rows, width and stop file per wave), design_nstate.md (S3-S6 design). Read both first.
- D1 A, D2 A. UD1 option 1 (user): core 1 keeps the N64 field model; listcen reports a class core1 outside the end proof.
  UD2 option 1: core 0 keeps a native transform stack. UD3 option 1: prof push at the NST1 gate. UD4: nd_comb_core stays on core 1 for a miss.
- User 2026-11-16: a table miss uses a fixed fallback and one log entry (PSOFB). Option B 2026-11-17: gbi.h etc. stay until S7.
- User 2026-11-17: as many parallel Haiku coders as the plan allows. Goal: every listcen total 0, elfgate func 0 data 0 gfxnames 0.
## Merged on main: NST1, NST3, NST2A-J, FALLBK, NDR0A-E2, NDR1B, NDR5, NCONV1A, REN1-REN7, MREN, PSOFB, HWFIX1, BOTSCR, BBRACE, DEL4D34, SHAPECACHE.
## Pages on main (oracle FIN), the snap chain (plan_finish.md W4)
- Running: NDR0F1 with fix ws_NDR0F1B (rnd rs = nd_snap_rpend set in sc_compare outlived a rectangle; draw_tri now scopes it), REN8, NST2K, HOSTTMP.
- Waiting: NDR0F2 (render apply: nd_render_apply, nd_sr_same, nd_snap_render) || NDR0G1 (NdSnapBind b[2] in the snap, snapchk b0 b1);
  then NDR0G2 (nd_bsrc macros ND_BTD/BWR/BSZ/BSH in the bind functions) || NDR3 (glyphs; nd_rec_napi_s: header arg 1 keeps the arm, ndc_snapc not a memo command);
  then NDR1C (rect snap always via nd_snap_rect with the zcol role; rc_render; rect binds from the snap).
- FACT: before NDR1C no drec run arms a rectangle (nd_rec.c:309 behind PMDL snapchk). NDR1B open item answered: no row (handback text).
- NST2H keys (FACT, a06c9a7b4c): spot 10130 2022d 1033e 1012a 1012b; wscmb 20130 1033e 10130 2022d 20220; axns and fxchk_a 20220 20121 10130.
- NDR5 gap (answered): no row. load_mtx guards the source itself (nd_mtx_set.c:134 mtx_bad).
## Open work (in order)
1. Answer coder questions of the snap chain (pages name the gate and the stop).
2. NDR2 direct draws: core 1 needs the pso words of each record (pd and prim are built in the core 0 runners: hud_rect_direct, cplx_quad_run,
   nd_trect_run, nd_fill_run, screen_overlays.c:108). Design: a pure pso-of-record function per kind for both cores, a core 1 snap on a copy of
   nd_ds with the pso words (omh mask, oml low 3 bits kept, combine via the nd_ds.c combine case, prim, env, dual_on 0), the record by nd_rec_napi_s.
3. NDR4 sprites: pm_gpu_spr_build (pm_gpu.c:4575) reads nd_ws (prim, env, omode, combine, dual_on) and gs.r_mix: give it the snap fields.
4. NCONV1D, NCONV2, NREC1A-5, DEL rows: no page yet. Their blocker is the snap chain (W4) and NDR2/NDR4 (W5).
5. Stream size: NdSnapChk is about 640 bytes (GUESS). The compact NdSnap of design_nstate.md section 7 is a later row.
6. DEL1D: dead declarations nd_walk_state at nd_api.h:13 and nd_direct.c:252 (no caller outside nd_walk.c; host stubs define it).
## Facts that cost time
- A crash at map load: read last_log.txt for `shape load fail` and count the lane romfs/shape files before any code theory.
- Core 1 (logic thread, pm_logic.c) runs at the same time as core 0 (walker). Det runs are serial: a race does not show in a bench.
  Shared statics of a core 1 call need a lock or a per-core copy (NDR0D: pm_comb.c build statics, nd_decide.c table).
- Every setter unit passes nd_rec_nr (nd_rec.c:419); core 0 runs it in nd_walk_state (nd_walk.c:394).
- The F3 globals (conv_k4, dual_on, comb_set ...) have struct-field and parameter twins in 20+ files: never rename them by sed.
- Core 0 never resets nd_ws (nd_state.c:11 static init only): nd_ds must not reset either.
- listcen: reader = NDO_ and G_ op tokens in 3ds/src/*.c, 3ds/include/*.h (comments stripped, :52). model = definitions of MODEL_NAMES (:197-218).
  gfxtype = Gfx in src, include, 3ds/src (:296; 3ds/include not counted). Count tokens by the same regex before a page states a number.
- Host seed scripts sed text of source files: before a rename row, grep 3ds/tests/host.d/*.sh for sed lines on the row's files (hunit.sh, ns3unit.sh, stunit.sh).
- box_fold_sync (nd_comb.c:372) folds tst1 on core 0 at a new epoch only (nd_geom.c:339). SETCOMBINE resets box_folded (nd_comb.c:534).
- pm_k4 (pm_comb.c:417) is set with conv_k4 at nd_set.c:149 only on the device; nd_model.c:208 decides with conv_k4 0 while pm_k4 holds the walker value.
- nd_rec_dl (nd_gtab.c:110): v2 table, else v1 table, else the raw list (gSPDisplayList). gtabrun log lines give the split.
- psochk scenes (azt.d debug): fxpic_20_psochk, fxchk_a/b/c, fxr_cap, fxr_chk, iseed_chk, fxcull_drec, fxdig_drec, miscp_drec, imgovl_*.
- snapchk benches: nimgm_strip_snapchk, wscmb_pause_snapchk (bench.d/NDR0A.json). dschk: the 4 scenes of azt.d/NST3.json.
- bench.py run does not build (bench.py:1218-1225): `sh 3ds/build.sh` before a capture run.
- A new debug switch of a gate scene must be on main before the gate row (the base is the merge-base).
- Hooks: SendMessage text must not hold the words "log line" (no_guess_hook.js). No sleep loops (no_poll_hook.js).
- Other oracles edit this file and the orchestrator commits stray files: read `git log -- <file>` and the file before a write.
- Core 0 leaves state that core 1 cannot follow: a model cache hit (nd_recs.c:198) skips lights, tile/load units, panner sizes, filter bits;
  a direct draw (nd_direct.c:103-116) writes pso state and binds without tile ops. dschk adopts core 1 values there and counts adiff.
## What to do first
1. Read plan_finish.md, design_nstate.md and the ws_* pages on main. Answer coder questions.
2. Write NDR2 (open work 2), then NDR4. Check `git log -5 -- docs/handoff` first: two oracles write pages.
