# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 8cfb9f345c, 2026-11-18. Counters FACT (listcen 642c83353f): build 306, data 191, gfxtype 230, reader 417, model 11, core1 442. Run listcen for fresh numbers.
## The plan: plan_finish.md (waves, width and stop file per wave), design_nstate.md (S3-S6 design), build_pipeline.md (BUILDO tools). Read them first.
- D1 A, D2 A. UD1 option 1: core 1 keeps the N64 field model; listcen class core1 is outside the end proof. UD2 option 1: core 0 keeps a native transform stack.
- User 2026-11-16: a table miss uses a fixed fallback and one log entry (PSOFB). Option B 2026-11-17: gbi.h etc. stay until S7.
- User 2026-11-17: at most 5 parallel Haiku coders. Goal: every listcen total 0, elfgate func 0 data 0 gfxnames 0. No console check until the end.
- User 2026-11-18 option A: NDR0G2 and NDR1C have an expected-diff gate (the correct bind from the snap, not the stale walker bind). Other rows stay byte-equal.
## Merged on main: NST1, NST3, NST2A-K, FALLBK, NDR0A-D, NDR0E1-2, NDR0F1-2, NDR0G1, NDR1B, NDR3, NDR5, NCONV1A, REN1-8, MREN, PSOFB, DEL4D34, BUILDO.
## Pages on main (oracle FIN), in order (W4, W5)
- NDR0G2 (starts now): mesh epochs bind and map tiles from the snap (nd_bsrc, ND_BTD/BWR/BSZ/BSH, nd_ep_bs, epoch bump in draw_tri), bindsnap and `drec frame` log lines.
- NDR1C after NDR0G2: rectangle snap always, nd_snap_live (a draw does not clear it; a command does), rc_render, nd_snap_comb, rectsnap lines.
- NDR2A after NDR1C: fill pso snap; nd_dir_rec, nd_dir_drop, nd_dir_prim1 serve NDR2B and NDR2C. Then NDR2B || NDR2C || NDR4 (disjoint files). Then NDR2D (deletes nd_pso_decide).
## Open work (in order)
1. Answer coder questions of the chain. Expected-diff rule: a DIFF passes only when the frame of its first DIFF draw has a bindsnap (NDR0G2) or rectsnap (NDR1C) line.
2. NCONV1D (Sonnet, standing yes): static list draws take no snap. Facts: nd_rec_dl (nd_gtab.c:110-136) records one NAPI per list (nd_ntab_rec v2, nd_gtab_run v1)
   and applies all its units to nd_ds (nd_ds_list, nd_ds.c:417-443). Core 0 gt_run (nd_walk.c:639-678): state-only lists walk units; else ent_gdl (nd_ent.c:437) or
   dlc_try (nd_shape.c:872, dlc_seg uses the open epoch) draw natively, else gt_body walks the units (nd_ntab_run :630) and draw_tri runs unarmed (walker state).
   Design need: a snap per draw epoch inside a list record that the DLC and entity native paths also read, with no change of their batch cuts. Then NCONV2, NREC1A-5, DEL rows.
3. NDR4 sprsnap: a DIFF there is a stale walker; only the user can allow an expected-diff gate for NDR4.
4. Stream size: NdSnapChk about 640 bytes (GUESS), 16 x 32 KB per context (FACT nd_rec.c:30-31). Rectangles and sprites add snaps; the compact NdSnap (design section 7) is a later row.
5. DEL1D: dead declarations nd_walk_state at nd_api.h:16 and nd_direct.c:254 (no caller outside nd_walk.c; host stubs define it).
## Facts that cost time
- Git Bash `sed -i` writes LF and turns a CRLF file into a whole-file diff (FACT dry run 2026-11-18). Pages say `sed -b -i` or the Edit tool.
- drec_cmp (bench.py:1658) names only the first DIFF draw. drec.txt has `frame` lines only with a step above 1 (pm_gpu.c:605); NDR0G2 logs `drec frame <f> seq <s>`.
- No host unit links nd_geom.c, nd_comb.c, nd_tex_set.c, nd_snap.c, nd_rect.c. nd_direct_test.c includes nd_direct.c (stubs there and in nd_snap_link_stub.c).
- Game files (src/) do not include nd_ds.h or nd_snap.h: give them getters (nd_dir_prim1) or plain-word accessors (nd_snap_spr).
- A crash at map load: read last_log.txt for `shape load fail`, then the shape entry (volume pm3ds-shape, build_pipeline.md) before any code theory.
- Core 1 (pm_logic.c) runs at the same time as core 0. Det runs are serial: a race does not show in a bench. Shared statics of a core 1 call need a lock or a copy.
- Every setter unit passes nd_rec_nr (nd_rec.c); core 0 runs it in nd_walk_state. Core 0 never resets nd_ws: nd_ds must not reset either.
- The F3 globals (conv_k4, dual_on, comb_set ...) have struct-field and parameter twins in 20+ files: never rename them by sed.
- listcen: reader = NDO_ and G_ op tokens in 3ds/src/*.c, 3ds/include/*.h. model = definitions of MODEL_NAMES (listcen.py:196-198). gfxtype = Gfx in src, include, 3ds/src.
- Host seed scripts sed source files: before a rename row, grep 3ds/tests/host.d/*.sh for sed lines on the row's files.
- box_fold_sync (nd_comb.c:377) folds tst1 on core 0 at a new epoch only. SETCOMBINE resets box_folded.
- snapchk benches: nimgm_strip_snapchk (until f400), wscmb_pause_snapchk (until f200), bench.d/NDR0A.json. drec windows: nimgm 400:2:20, wscmb_pause 140:2, wscmb_fsel 300:2, spot 200:2.
- bench.py run does not build: `sh 3ds/build.sh` before a capture run. A new debug switch of a gate scene must be on main before the gate row.
- Hooks: SendMessage text must not hold the words "log line" (no_guess_hook.js). No sleep loops (no_poll_hook.js).
- Other oracles edit this file: read `git log -- <file>` and the file before a write.
- Core 0 stale state: a model cache hit (nd_recs.c:198) skips lights, tile/load units, panner sizes, filter bits; a direct draw writes pso state and binds without tile ops.
## What to do first
1. Read plan_finish.md, design_nstate.md and the ws_* pages on main. Answer coder questions (SendMessage, 10 lines).
2. Design NCONV1D (open work 2), then write NCONV2, NREC and DEL pages. Check `git log -5 -- docs/handoff` first.
