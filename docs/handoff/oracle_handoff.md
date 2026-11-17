# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 05f18ca04d, 2026-11-17. PHASEE2 wrote ws_FLAMEDL, ws_PSOFB (waits PSOSEEN merge), design_nstate.md (S2).
Counters FACT (listcen on b4de08e55b): build 311, gfxpos 3, data 191, gfxtype 251, recdl 5, reader 792, model 15. Since: OPMOVE reader -62, FXSLOT2 recdl -5.
Merged: OPMOVE (reader -62), FXSLOT2 (recdl -5). PSOSEEN branch 2f1e9c7d06: gen_pso_tab fails in its build 2 (no oracle answer seen).
## The chain to the end: docs/handoff/plan_walker_delete.md (stages S0-S8, decisions D1 D2, risks). Read it first.
- D1, D2: orchestrator chose A (core 1 draw state, table lookup per draw; walk census deleted with the walker). The user may override.
- User 2026-11-16: a table miss uses a fixed fallback and writes one log entry. nd_comb_core leaves the device.
- User option B 2026-11-17: gbi.h, gbi_custom.h, gt.h stay until the walker goes (stage S7).
## Pages on main (Haiku)
- ws_OPMOVE (no wait): git mv 3ds/src/nd_op.c to 3ds/tools (host only), 10 script paths, CMakeLists.txt. reader -62.
- ws_PSOSEEN (no wait): nd_comb.c:241 miss arm logs `psoseen <ND_PSO_DESC fields>` once per key; psoseen.py writes nd_pso_seen.c.
- ws_FXSLOT2 (recdl -5), ws_TST13 (build -1, gfxpos -1), ws_IMGOVL (recdl -1).
## Open work (in order)
1. Answer coder questions: FLAMEDL, PSOSEEN (gen_pso_tab failure), TST13, IMGOVL.
2. PSOFB runs after the PSOSEEN merge. Its psochk lines lose the lkbad field.
3. User answers UD1-UD3 (design_nstate.md section 12) and the Sonnet yes for NST1, NDR0, NCONV1, NREC1. Then write ws_NST0
   (Haiku, ws_NST0.patch: the F3 globals into NdWs), then ws_NST1 (Sonnet).
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
1. Read design_nstate.md. Answer coder questions.
2. Write the S3 pages from design_nstate.md section 10 after the user decisions.
