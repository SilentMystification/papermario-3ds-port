# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main d323ade40d, 2026-11-18 (NCONV2A merged 73359b439a; NREC4, NREC2B running or paged). Counters FACT (listcen 642c83353f): build 306, data 191, gfxtype 230, reader 417, model 11, core1 442. Run listcen for fresh numbers.
## The plan: plan_finish.md (waves, width and stop file per wave), design_nstate.md (S3-S6 design), build_pipeline.md (BUILDO tools). Read them first.
- D1 A, D2 A. UD1 option 1: core 1 keeps the N64 field model; listcen class core1 is outside the end proof. UD2 option 1: core 0 keeps a native transform stack.
- User 2026-11-16: a table miss uses a fixed fallback and one log entry (PSOFB). Option B 2026-11-17: gbi.h etc. stay until S7.
- User 2026-11-17: at most 5 parallel Haiku coders. Goal: every listcen total 0, elfgate func 0 data 0 gfxnames 0. No console check until the end.
- User 2026-11-18 option A: NDR0G2 and NDR1C have an expected-diff gate (the correct bind from the snap, not the stale walker bind). Other rows stay byte-equal.
## Chain NCONV coach: coders afc7b34e8d19be862 (NCONV1D1), aeaed982632fdec55 (NCONV1D2), a5b6acc6b4c73a61b (NCONV2A), all done. NCONV2A GATED ce1e571699: fxops f500 tsnap cmp 8821 wtss 0 tris 10 vtss 0, snapchk bad 469 = base 4d10b0ebb5.
## FAULT on main (NREC2): fxops_snapchk f500 snapchk bad 469 name "sl" at NDC_ENT (last 6), on entity list ranges. NREC2 gated on entcape only. sl = nd_snap.c:176. sl_const writers nd_comb.c:313, nd_scomp.c:210, :305. Base log: build3ds/basecache/4d10b0ebb5.../fxops_snapchk/run.log.
## Merged on main: NST1, NST3, NST2A-K, FALLBK, NDR0A-F, NDR0G1-2, NDR1B-C, NDR2A-C, NDR3, NDR4, NDR5, NCONV1A, NCONV1D1, NCONV1D2, NREC1A, NDR2D, REN1-8, MREN, PSOFB, DEL4D34, BUILDO.
## Chain NDR: done (coach a28bf24a1ad35b1c8). No NDR page is left. Snapchk data at the end (FACT NDR2D): nimgm f400 recs 14138 cmp 6286 wst 870; wscmb f200 recs 18047 cmp 13482 wst 12060.
## Open work (in order)
1. Expected-diff rule (NDR0G2, NDR1C only): a DIFF passes only when the frame of its first DIFF draw has a bindsnap (NDR0G2) or rectsnap (NDR1C) line.
2. NCONV1D1 and NCONV1D2 merged (8e3e583bff). FACT: fxops lsnap lists 1118 cmp 1583 p1 1583 bst 0 tmis 0 over 0, drec equal. entcape_kmr03 lsnap lists 644 cmp 644 p1 644.
   Design: a list snap range starts at the first triangle after a state unit; core 0 finds it by triangle ordinal (nd_snap_lsr). LS_MAX 16 ranges: a list over it keeps range 15 (stale) until NREC2 gives it no table.
3. NDR4 merged byte-equal with 0 sprsnap lines (FACT 0cf4333a5e).
4. Stream size: NdSnapChk about 640 bytes (GUESS), 16 x 32 KB per context (FACT nd_rec.c:30-31). Rectangles and sprites add snaps; the compact NdSnap (design section 7) is a later row. Risk (GUESS): the snap_take memo (nd_snap.c:51) counts no NDC_END, so a frame whose only draw repeats the last snap has no live snap at nd_direct_bind and walks.
5. DEL1D: dead declarations nd_walk_state at nd_api.h:16 and nd_direct.c:254 (no caller outside nd_walk.c; host stubs define it).
6. NREC (design oracle NREC). NREC1A merged (27f33ea1da): FACT fxops msnap cmp 260 nos 1 bad 0 wst 43 genv 0. The record payload already carries ND_FS ids (nd_rec.c:246-248).
   Pages on main (order): ws_NREC1B (model_draw reads the msnap; snap always, room reserved so nos 0; wst split w0td w0t w1td w1t) and ws_NREC2 (entity records get a list table in NDC_ENT p[1]; nd_lsc_end_at; over lists get no table) run in parallel. Then ws_NCONV2A (tss in NdSnapChk, tsnap line wtss vtss; WAIT NREC2). Then ws_NREC4 (parts tables in payload u[1], vprim 1 snaps, nd_part_draw consumes; new scene fxp_snapchk; WAIT NCONV2A). All Haiku.
   GATED (merge open): NREC1B 75665f9544 (msnap f500 cmp 261 nos 0 bad 0 wst 43 w0td 4 w0t 1 w1td 43 w1t 42), NREC2 0452b891fc (entcape lsnap f200 lists 5152 bst 0 tmis 0 over 0). FACT msnap cmp is flat after f100: use and cmp run only in rec_miss (nd_recs.c:256-257).
   OPEN walker reads of model_draw after NREC1B: pm_gbi_bind_tile1 (nd_tex_set.c:23-27), cconst, gs.r_mix, r_mixcol, r_memblend, pm_tex_native_unit0 (pm_gpu.c:3906-3950), the key B (pm_gpu.c:3856) and hit_apply (nd_recs.c:200) read nd_ws before the record.
   NREC3 dropped: FACT map shapes draw through model records (nd_model_draw reads only NdMCtx, no nd_ws in nd_model.c); dlc_try runs only for list calls into gMapShapeData (nd_walk.c:269, :574, :668) and under ndm or nomodelnat (nd_recs.c:302). Its entry geom and tss reads join NCONV2B.
   NCONV2 (oracle NCONV2RA, pages on main d323ade40d). Design: core 0 keeps RSP state (geom, tss, segbase, lights, matrices: the vertex load reads them, nd_geom.c:463, :491); the combine units (OML PRIM ENV FOG BLEND RARE COMB) leave core 0 lists. NCONV2B: sc_adopt writes the consumed snap fields into the walker (no-op where snapchk has no bad of those names). NCONV2C: core 1 ep flag per range (ND_LS_EP, ND_LS_DUP in the range start word), lscut line epc epm epx dup rect. NCONV2D (PRE epm 0 epx 0 rect 0): ls_run skips those units, ls_gpu replays their GPU calls at the arm, ds_take copies the core 1 end state (a NdDs after the snaps of each table) at nd_lsr_end, plod in pad2.
   NREC5 (page on main): compiled defs in lists and entity gaps run as units; nd_scomp_exec, _exec_own go (C11 -2 GUESS); the old cut and sr_ok = 0 stay. No NCONV2D wait. NREC2B (oracle NREC) owns the fxops sl fault. Next after NCONV2D (pages then, from the NCONV2D code): DEL3A stream units (nd_rec_nr) as ls_run with a core 1 ep unit; the texture model units (TIMG..LOADTLUT, TILESIZE writes tile_x0/y0/fx/fy nd_tex_set.c:165-168, read by pm_gbi_gv_at nd_geom.c:119) need a snap source first; then W10-W14 of plan_finish.md.
## Facts that cost time
- The Bash default timeout is 120 s. nd_host_test.sh runs longer and goes to the background: pages say `timeout: 600000` (FACT NDR2B, NDR2C). `bench.py run` does not build: a page says `PM_LANE=<id> sh 3ds/build.sh` first (FACT NDR0G2 STALE BUILD exit 5).
- A SendMessage to a coder that handed back resumes it. Never also start a fresh coder on the same worktree (FACT NCONV2A: 2 writers).
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
2. Answer the NREC1B, NREC2, NCONV2A, NREC4 coders. Write NCONV2B from the NCONV2A tsnap data (open work 6), then DEL pages. Check `git log -5 -- docs/handoff` first.
