# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main a6b7a1b09b, 2026-11-17. PHASEE wrote plan_walker_delete.md, ws_OPMOVE, ws_PSOSEEN. DIAG1 wrote ws_FXSLOT2.
Counters FACT (listcen on d74594f376): build 312, gfxpos 4, data 191, data_arrays 0, gfxtype 251, recdl 6, reader 792, model 15.
Merged after that: MISCP (recdl -1), FXSLOT1 (build -1, gfxpos -1, recdl +1).
Goal: elfgate.py prints func 0 data 0 gfxnames 0 and every listcen counter is 0.
## The chain to the end: docs/handoff/plan_walker_delete.md (stages S0-S8, decisions D1 D2, risks). Read it first.
- D1, D2: orchestrator chose A (core 1 draw state, table lookup per draw; walk census deleted with the walker). The user may override.
- User 2026-11-16: a table miss uses a fixed fallback and writes one log entry. nd_comb_core leaves the device.
- User option B 2026-11-17: gbi.h, gbi_custom.h, gt.h stay until the walker goes (stage S7).
## Pages on main (Haiku)
- ws_OPMOVE (no wait): git mv 3ds/src/nd_op.c to 3ds/tools (host only), 10 script paths, CMakeLists.txt. reader -62.
- ws_PSOSEEN (no wait): nd_comb.c:241 miss arm logs `psoseen <ND_PSO_DESC fields>` once per key; psoseen.py writes nd_pso_seen.c.
- ws_FXSLOT2 (recdl -5), ws_TST13 (build -1, gfxpos -1), ws_IMGOVL (recdl -1).
## Open work (in order)
1. FLAMEDL page (FXSLOT1 merged): src/effects/flame.c:188 `gSPDisplayList(gMainGfxPos++, NDN(D_09000918_3544C8))` (state only:
   loads, cycle type) and :211 `NDN(D_090008F8_3544A8)` (4 vertices, 2 triangles, inherits the preset combine). Form: FXDIG and
   FXSLOT1 (git show of the deleted pages): nd_rec_dl capture form, entcap run, entapply, entry desc. Bench fxops_drec runs flame
   (FLTGT gate). Read pm_fxtest.c for the flame index. build -2, gfxpos -2. With TST13 and FXSLOT2: gfxpos 0, src build 0.
2. PSOFB page (after PSOSEEN): nd_comb.c:241 miss gives a fixed fallback NdCombOut and one log entry. All decisions pass
   nd_comb_pick (nd_comb.c:240, callers :276 :302). Split nd_decide.c: nd_pso_in_eq, nd_pso_flags, nd_pso_fix, nd_const_rgb,
   nd_comb_tst stay on the device (to nd_psolk.c). Check them for calls into the static decide code first. The rest goes to
   3ds/tools/nd_decide.c. Path readers: gen_pso_tab.py:129, build.sh:211, host.d coin.sh:6 :8, conv2c.sh:9, decide.sh:5. model -2.
3. S2: write docs/handoff/design_nstate.md (Opus oracle): core 1 draw state, native draw record, when core 1 resolves the pso id,
   the batch rule that keeps drec equal (nd_op_same nd_walk.c:280, gv_epoch :283), the v2 field-set op instead of NDN_UNIT
   (nd_gtab.h:104). Then the S3-S6 Haiku pages.
4. gMainGfxPos after src gfxpos 0: nd_rec.c redirect, nd_gtab.c:127, nd_cen.c, variables.h:111. It leaves in S7 with nd_gbi.h.
## Facts that cost time
- Every setter writes an NDO typed unit through nd_nr (nd_nr.h:11). Core 0 runs it in nd_walk_state (nd_walk.c:390). The game
  has no gbi call left, but the stream is still the N64 state model. reader and model fall only with the new stream (S3-S6).
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
- fxtest index = FXLIST line - 33 (pm_fxtest.c). host.d scripts run only through nd_host_test.sh. PM_HOSTSEL=all on rowloop lines.
- lkey_cen.py:71-72 fails a listed file whose reader count drops: a page that removes `.words.w*` reads edits allow.txt.
- Hooks: SendMessage text must not hold the words "log line" (no_guess_hook.js). No sleep loops (no_poll_hook.js).
- Other oracles edit this file and the orchestrator commits stray files: read `git log -- <file>` and the file before a write.
- Coders often hand back before the oracle reply arrives. Tell the orchestrator to resume them with the reply.
## What to do first
1. Answer coder questions (OPMOVE, PSOSEEN, FXSLOT2, TST13, IMGOVL).
2. Write ws_FLAMEDL, then ws_PSOFB when PSOSEEN is merged.
3. Write design_nstate.md (S2) before any S3 page.
