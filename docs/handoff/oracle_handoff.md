# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 9e091b034e, 2026-11-17. PHASEE2 (Opus) wrote ws_FLAMEDL (merged), ws_PSOFB (coder aa0ba759af9c83fdf runs), design_nstate.md
(S2, revised 8aabea0190: no NST0), ws_NST1 (Sonnet), ws_NST3, ws_NST2A. Merged since b4de08e55b: OPMOVE, FXSLOT2, PSOSEEN, FLAMEDL, TST13, TATTLEAR.
Counters FACT (listcen on b4de08e55b): build 311, gfxpos 3, recdl 5, reader 792, model 15. Moves since: OPMOVE reader -62, FXSLOT2 recdl -5,
FLAMEDL build -2 gfxpos -2, TST13 build -1 gfxpos -1 (page values). Run listcen for fresh numbers.
## The chain: plan_walker_delete.md (stages; S3-S6 rows updated) and design_nstate.md (the S3-S6 design). Read both first.
- D1 A, D2 A. UD1 option 1 (user): core 1 keeps the N64 field model; listcen reports a class core1 outside the end proof.
  UD2 option 1: core 0 keeps a native transform stack. UD3 option 1: prof push at the NST1 gate. Sonnet yes: NST1, NDR0, NCONV1, NREC1.
- User 2026-11-16: a table miss uses a fixed fallback and one log entry (PSOFB). Option B 2026-11-17: gbi.h etc. stay until S7.
## Pages on main and their waits
- ws_PSOFB (Haiku, running): model -2. Its psochk lines lose lkbad. nd_psokey.c holds the device pso helpers.
- ws_NST1 (Sonnet, no wait): nd_ds.h/nd_ds.c (core 1 copy of the state writes, nd_ds_apply), hook in nd_rec_nr, host nst1.sh.
- ws_NST3 (Haiku, after NST1): dschk switch (nd_dschk.c, nd_ds_cmp, ep bit in nd_rec.c, nd_st_epoch), 4 dschk scenes, dschk_base.txt.
- ws_NST2A (Haiku, after NST3): nd_ds_list for static lists at nd_rec_dl and NDO_CALL units.
## Open work (in order)
1. Answer coder questions (PSOFB, NST1, NST3, NST2A). The NST3 epbad rule is design section 6; GEOM after rects is a GUESS.
2. Write NST2B (model records: R0 render-mode units and R13 aux units at nd_rec_model, scomp defs), NST2C (entity nd_rec_ent_draw,
   sprite nd_rec_spr, particles nd_parts_add pre/post units and parts_tab lists), NST2D (nd_rec_list, map shapes dlc, ranges
   nd_rec_call: walk the units of [a, b) of the core 1 stream). Each: call nd_ds_apply / nd_ds_list at the core 1 producer; gate
   drec equal + dschk bad lower; NST2D ends with bad 0 and epbad 0 in the 4 scenes. Form: ws_NST2A.
3. Then NDR0 (Sonnet): design section 7. Then NCONV1 (section 8), NREC1 (section 9).
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
- Coders often hand back before the oracle reply arrives. Tell the orchestrator to resume them with the reply.
## What to do first
1. Read design_nstate.md and plan_walker_delete.md. Answer coder questions.
2. Write ws_NST2B, then NST2C, NST2D (open work 2).
