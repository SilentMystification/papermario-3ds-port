# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 8cfb9f345c, 2026-11-18. Counters FACT (listcen 642c83353f): build 306, data 191, gfxtype 230, reader 417, model 11, core1 442. Run listcen for fresh numbers.
## The plan: plan_finish.md (waves, width and stop file per wave), design_nstate.md (S3-S6 design), build_pipeline.md (BUILDO tools). Read them first.
- D1 A, D2 A. UD1 option 1: core 1 keeps the N64 field model; listcen class core1 is outside the end proof. UD2 option 1: core 0 keeps a native transform stack.
- User 2026-11-16: a table miss uses a fixed fallback and one log entry (PSOFB). Option B 2026-11-17: gbi.h etc. stay until S7.
- User 2026-11-17: at most 5 parallel Haiku coders. Goal: every listcen total 0, elfgate func 0 data 0 gfxnames 0. No console check until the end.
- User 2026-11-18 option A: NDR0G2 and NDR1C have an expected-diff gate (the correct bind from the snap, not the stale walker bind). Other rows stay byte-equal.
## Chain NCONV coach: a NCONV1D1 coder afc7b34e8d19be862 (done).
## Merged on main: NST1, NST3, NST2A-K, FALLBK, NDR0A-F, NDR0G1-2, NDR1B-C, NDR2A-C, NDR3, NDR4, NDR5, NCONV1A, NCONV1D1, NDR2D, REN1-8, MREN, PSOFB, DEL4D34, BUILDO.
## Chain NDR: done (coach a28bf24a1ad35b1c8). No NDR page is left. Snapchk data at the end (FACT NDR2D): nimgm f400 recs 14138 cmp 6286 wst 870; wscmb f200 recs 18047 cmp 13482 wst 12060.
## Open work (in order)
1. Expected-diff rule (NDR0G2, NDR1C only): a DIFF passes only when the frame of its first DIFF draw has a bindsnap (NDR0G2) or rectsnap (NDR1C) line.
2. NCONV1D: pages ws_NCONV1D1 (core 1 list snaps in the list record, rec[4] table, core 0 ordinal and compare, scenes fxops_snapchk and entcape_kmr03_snapchk) and ws_NCONV1D2 (consume in P1 dlc_seg and P2 draw_tri). Haiku, Sonnet standing yes if Haiku fails twice.
   Order: after NDR0G2, NDR1C, NDR2A, NDR4 (all edit nd_snap.c), parallel with NDR2B, NDR2C, NDR2D. NCONV1D2 starts only when NCONV1D1 shows lsnap bad 0, tmis 0, over 0, p1 above 0, wst 0. A wst above 0: ask the user for an expected-diff gate (default no). Then NCONV2, NREC1A-5, DEL rows.
   NCONV1D1 merged ae0f255985. FACT hand-back: fxops_snapchk lsnap f500 lists 1118 snaps 1583 cmp 1583 p1 1583 bad 0 wst 1577 tmis 0 over 0 (snapchk wst 4101). entcape_kmr03_snapchk lsnap f200 lists 644 cmp 644 p1 644 bad 0 wst 0.
   ws_NCONV1D2 rewritten (no wst WAIT): new lsnap counter bst = list triangles whose epoch binds tiles other than the range snap. Step 1 measures bst before (seeded fault, GUESS above 0), step 2 consumes, gate bst 0 + fxops_drec byte-equal. A DIFF stops: the user decides an expected-diff gate (bindsnap proof). Both list scenes have p1 = cmp: P2 (draw_tri) has no list compare in them (FACT), nd_part_draw does not count nd_ls_ord (tmis guards it).
   Design: a list snap range starts at the first triangle after a state unit; core 0 finds it by triangle ordinal (nd_snap_lsr). Open risk: P1 compares may name box_folded (dlc_seg runs no box_fold_sync).
3. NDR4 merged byte-equal with 0 sprsnap lines (FACT 0cf4333a5e).
4. Stream size: NdSnapChk about 640 bytes (GUESS), 16 x 32 KB per context (FACT nd_rec.c:30-31). Rectangles and sprites add snaps; the compact NdSnap (design section 7) is a later row. Risk (GUESS): the snap_take memo (nd_snap.c:51) counts no NDC_END, so a frame whose only draw repeats the last snap has no live snap at nd_direct_bind and walks.
5. DEL1D: dead declarations nd_walk_state at nd_api.h:16 and nd_direct.c:254 (no caller outside nd_walk.c; host stubs define it).
6. NREC (design oracle NREC). ws_NREC1A written: model snap after nd_ds_model in stream data after the record (header arg bit 0), core 0 compare in rec_miss before model_draw, line `msnap ... bad wst genv nos` (snapchk only, drec equal). It replaces the plan_finish text "payload as FS ops": the payload units carry ND_FS ids already (FACT nd_rec.c:246-248, nd_ds.c:466-468).
   NREC1B (consume in model_draw) waits for the NREC1A data. More walker reads of model_draw stay OPEN for it: pm_gbi_bind_tile1 (core 0 plan decision, nd_tex_set.c:23-27), cconst, gs.r_mix, r_memblend, pm_tex_native_unit0 (pm_gpu.c:3906-3950).
   No page yet, wait for the NCONV1D2 merge: NCONV2 (FACT core 0 has no NdNTab op runner: nd_ntab_run runs the NDN_UNIT words, nd_walk.c:631; a new design, GUESS not Haiku-sized), NREC2 (entity lists reuse the nd_lsr code that NCONV1D2 changes), NREC3 (nd_shape.c is a NCONV1D2 file), NREC4 (needs NREC2, NCONV2), NREC5 (the scomp combine bind goes only when every later draw consumes a snap).
## Facts that cost time
- The Bash default timeout is 120 s. nd_host_test.sh runs longer and goes to the background: pages say `timeout: 600000` (FACT NDR2B, NDR2C). `bench.py run` does not build: a page says `PM_LANE=<id> sh 3ds/build.sh` first (FACT NDR0G2 STALE BUILD exit 5).
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
- Model key A hashes all 28 record words (pm_gpu.c:3836): a new record word breaks the model cache. Put model data in stream data (nd_rec_data_st) or the NDC_MODEL header arg.
- Core 0 stale state: a model cache hit (nd_recs.c:198) skips lights, tile/load units, panner sizes, filter bits; a direct draw writes pso state and binds without tile ops.
- wst (NDR0G2) is the NDR0G1 b-name bad on the current compare population: its cap is a base run of the same scene, never a fixed number. Later pages say: report wst, not judged, when a row adds compares.
## Roles (dispatcher.md)
A COACH oracle runs the coders of one chain under a quota (oracle.md rule 7). A design oracle writes pages and starts no agent. Both report to the dispatcher.
## What to do first
1. Read plan_finish.md, design_nstate.md and the ws_* pages on main. Answer coder questions (SendMessage, 10 lines).
2. NCONV1D pages are written (open work 2). Write NCONV2, NREC2-5 after the NCONV1D2 merge, NREC1B after the NREC1A data (open work 6), then DEL pages. Check `git log -5 -- docs/handoff` first.
