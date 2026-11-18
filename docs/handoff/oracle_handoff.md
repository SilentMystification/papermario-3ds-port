# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 4abca1ff8a, 2026-11-18. Counters FACT (listcen on main 4abca1ff8a): build 308, data 191, gfxtype 246, reader 490, model 11, core1 351; elfgate func 46 (brief).
Run listcen for fresh numbers.
## The plan: plan_finish.md (15 waves, 50 Haiku rows, width and stop file per wave), design_nstate.md (S3-S6 design). Read both first.
- D1 A, D2 A. UD1 option 1 (user): core 1 keeps the N64 field model; listcen reports a class core1 outside the end proof.
  UD2 option 1: core 0 keeps a native transform stack. UD3 option 1: prof push at the NST1 gate. UD4: nd_comb_core stays on core 1 for a miss.
- User 2026-11-16: a table miss uses a fixed fallback and one log entry (PSOFB). Option B 2026-11-17: gbi.h etc. stay until S7.
- User 2026-11-17: as many parallel Haiku coders as the plan allows. Goal: every listcen total 0, elfgate func 0 data 0 gfxnames 0.
## Merged on main: NST1, NST3, NST2A-H, FALLBK, NDR0A-E1, NDR5, NCONV1A, REN1-REN7, MREN, PSOFB, HWFIX1, BOTSCR, BBRACE.
## Pages on main (oracle FIN)
- Running: NDR0F1, NST2J (a41cc4d4d4 crashes in Azahar at map model lookup; no code path from the diff; clean rebuild asked).
  Running: NDR1B (75024c7929, rect compare after box_fold_sync + snap combine). Waiting: REN8 (NST2J), NST2K (NST2J), NDR0F2/G (NDR0F1 data).
- NDR0E2 data (FACT 4df41cf45a): rect snapchk bad 486 = fold names (rect compares before box_fold_sync). NST2I: epbad all 2090a (rule 9, NAPI).
- Two oracles write pages: read `git log -5 -- docs/handoff` and `ls docs/handoff` before a page (NDR0E3 was a duplicate of NDR1B, dropped a6cd9f0033).
- NST2H keys (FACT, a06c9a7b4c): spot 10130 2022d 1033e 1012a 1012b; wscmb 20130 1033e 10130 2022d 20220; axns and fxchk_a 20220 20121 10130.
- NDR5 gap (answered): no row. load_mtx guards the source itself (nd_mtx_set.c:134 mtx_bad); gbi_cold_bad_mtx is a warning with list context.
## Open work (in order)
1. Answer coder questions (NDR0E1: sl and ls names mean scm_replay leaves sl_* stale, nd_comb.c:180-188; then E2 must not write sl_*).
2. NDR1B 75024c7929 written (after NDR0E2 merge: texrect compare after box_fold_sync nd_rect.c:240, then nd_snapchk_tri; NDR0E2 nimgm bad 486 = this). From NDR0F1 data: NDR0F2 (render apply in gv_epoch_enter, keep zmark order nd_set.c:243, nd_geom.c:221),
   NDR0G (binds: ensure_tile, dual_bind nd_geom.c:212-213 from td in the snap). Then W5: NDR2 direct (nd_pso_decide nd_direct.c:69 desc form), NDR4, NCONV1D.
3. NST2I rule 9 (every command sets ep first) masks rules 1-8 in the dschk word: epbad then counts quiet commands only (key 9xx, op).
   The per-draw ep (S4) needs sc_lost precision: NST2J measures it (sclbad, sigbad). From its scl lines: the next sc row (ranges, glyphs, box pattern).
4. Dead declarations for DEL1D: nd_walk_state at nd_api.h:13 and nd_direct.c:252 (no caller outside nd_walk.c; host stubs define it).
5. After REN5-8 and MREN: no rename row is left. Files with NDO_ that stay: none outside the deleted set (nd_op.h, nd_set.c, nd_walk.c,
   nd_dlc.c, nd_shape.c, nd_tex_set.c, nd_gtab.c, nd_mtx_set.c 1, nd_geo.c REN5). Comment-only NDO_ in nd_int.h, nd_target.c, nd_scomp.h, nd_gtab.h, nd_api.h.
## Facts that cost time
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
2. Write the next row from each hand-back: NDR1B -> NDR1C (rect render and binds), NDR0F1 -> NDR0F2 and NDR0G, NST2J -> sc fix row from its scl lines.
