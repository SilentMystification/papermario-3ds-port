# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 461b70e216+, 2026-11-18. Counters FACT (row gates): reader 630 (REN6 gate), model 15, elfgate func 48 data 365 (brief 2026-11-17).
Run listcen for fresh numbers.
## The plan: plan_finish.md (15 waves, 50 Haiku rows, width and stop file per wave), design_nstate.md (S3-S6 design). Read both first.
- D1 A, D2 A. UD1 option 1 (user): core 1 keeps the N64 field model; listcen reports a class core1 outside the end proof.
  UD2 option 1: core 0 keeps a native transform stack. UD3 option 1: prof push at the NST1 gate. UD4: nd_comb_core stays on core 1 for a miss.
- User 2026-11-16: a table miss uses a fixed fallback and one log entry (PSOFB). Option B 2026-11-17: gbi.h etc. stay until S7.
- User 2026-11-17: as many parallel Haiku coders as the plan allows. Goal: every listcen total 0, elfgate func 0 data 0 gfxnames 0.
## Merged on main: NST1, NST3, NST2A-G, FALLBK, NDR0A-D, NDR5, NCONV1A, REN1-REN6, PSOFB, HWFIX1, BOTSCR.
## Pages on main (oracle FIN)
- Running: NDR0E1 7c8aeb0f21. Gated, merge pending: NST2H a06c9a7b4c, MREN.
- Waiting: NST2I 461b70e216 (after NST2H: dschk compares the gv_epoch move; core 1 ep rules 6 tex/fill/primz/scissor/rare, 7 MW not LIGHTCOL,
  8 RECT and MISC not MODVTX, 9 every command), NDR0E2 b47413357f (after NDR0E1 and NST2I), REN8 (after NDR0E2), NDR0F1 b31daf9ca6 (after NDR0E2).
- NST2H keys (FACT, a06c9a7b4c): spot 10130 2022d 1033e 1012a 1012b; wscmb 20130 1033e 10130 2022d 20220; axns and fxchk_a 20220 20121 10130.
- NDR5 gap (answered): no row. load_mtx guards the source itself (nd_mtx_set.c:134 mtx_bad); gbi_cold_bad_mtx is a warning with list context.
## Open work (in order)
1. Answer coder questions (NDR0E1: sl and ls names mean scm_replay leaves sl_* stale, nd_comb.c:180-188; then E2 must not write sl_*).
2. From NDR0E2 data: NDR1B (texrect apply). From NDR0F1 data: NDR0F2 (render apply in gv_epoch_enter, keep zmark order nd_set.c:243, nd_geom.c:221),
   NDR0G (binds: ensure_tile, dual_bind nd_geom.c:212-213 from td in the snap). Then W5: NDR2 direct (nd_pso_decide nd_direct.c:69 desc form), NDR4, NCONV1D.
3. NST2H keys name each epbad source (NST2I adds site ff = command op, rules 6-9): side 1 core 0 site (1 op_typed, 2 MOVEMEM, 3 op_ctl, 4 model), side 2 core 1 rule (1 change, 2 sclost,
   3 MOVEMEM, 4 model, 5 list marker), low byte the unit id. After NST2I: NST2J sets ds_sclost only at the commands whose core 0 runner clears
   sc_ok (F6 list: nd_walk.c:437 :692, particles, nd_rect.c:122, nd_direct.c:184, nd_recs.c:243 :292 :329); TEXTURE skip also reads sc_sig.
4. After REN5-8 and MREN: no rename row is left. Files with NDO_ that stay: none outside the deleted set (nd_op.h, nd_set.c, nd_walk.c,
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
2. Write the next row from each hand-back: NDR0E1 -> E2 go, NDR0E2 -> NDR1B, NDR0F1 -> NDR0F2 and NDR0G, NST2I -> NST2J.
