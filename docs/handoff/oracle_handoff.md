# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 8bcf0252ee (T1FIX, NC0S merged) plus the pages of oracle NATIVE17 (second pass), 2026-11-20.
## Goal
- User: better frame time on the O3DS, work spread about evenly over core 0 and core 1, core 0 decides nothing, good ARM11 code, good PICA use.
- The plan: docs/handoff/plan_perf.md (findings with file:line, ranked rows, section 6a MCOPY, section 7 nd_c0 fields left, section 10 questions).
- Numbers (FACT console capture build 7569): core 1 44.4 ms, core 0 24.0 ms, ratio 1.86, ft 46.5, fps 42.2; c1a m 10975 words per step (0 after MCOPY2). c0gate 0 / 0 / 1 (nd_c0, 896 B).
## Tools on main
- `python 3ds/tools/c0gate.py <elf>`: calls / interp / state. Run on the ELF of the same tree (a clean worktree or git archive).
- `bench.py base <bench> --commit $(git merge-base HEAD main)`, `bench.py run a,b --row <id>`, `bench.py cmp <bench> --row <id>`.
- Gate benches (drec): spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec, hudfb_drec; entcapd_kgr01_drec (darkness stencil).
- drec end lines (pm_gpu.c:628-632): `c0src ... wbind` (binds from walker tiles), `altw ... c1 m ... c0 all ... m` (copy words). `drec frame <F> seq <S>` (:609).
## Open work (order)
1. Pages on main: ws_NC0V (Sonnet, runs now); then ws_NC0M and ws_NC0T1 (Haiku, parallel, WAIT NC0V); then ws_NC0F (Sonnet, WAIT NC0M, NC0T1).
2. Next pages: NC0T2 (texture setter state td, tst1, timg, tlut, tlut_n, about 230 references: own read pass). Then c0gate state 0.
3. ALT2 has no closed design: core 0 runs 50 percent of the entity copy words (slices, refusals, nd_ent.c:310-326). Next: a census of run reasons.
4. Q2 capture picks the next core 1 row and MDLHIT2. GPUTRL needs the user (PICA hang risk).
## Design facts (FACT code read)
- Stream OML units never write nd_c0.omode_l (nd_rec.c:480 records nd_oml_run, nd_walk.c:258 is empty). nd_img_role read a stale word until NC0R.
- core 1 keeps no nested state after a stream CALL (nd_rec.c:483-485 apply to a scratch copy); core 0 runs the list. Static runners (nd_sg_try, ent_gdl,
  nd_ent_case) read nd_c0 today. NC0B gives them an entry record (core 1 values at the call) and the register nd_gs (NdGs, nd_snap.h).
- alt_s is the applied state at every nd_alt_put (nd_rec.c:258, :276, :484, :510, :519, nd_gtab.c:143, :152): VTX words come from it.
- Entity copy slices run inside nd_ent_case (nd_ent.c:417): copy GEOM, TEXTURE, OML, OMH appliers write nd_gs when it is set.
- The model copy's TILESIZE units set the uv origin (nd_tex_set.c:127-130); the miss gv reads it (nd_geom.c:117) and the cache keeps it: MCOPY1 has a census first.
- nd_alt_open and nd_alt_close reset the core 1 stream shadow (nd_rec.c:534-535 sh_all): MCOPY2 must keep the resets.
- Parsers of stream records by runner pointer: nd_model.c:288 nd_r11_state, nd_rec.c stream_run, host tests nd_alt_et_test.c, nd_geo_test.c, nd_st_test.c.
## Facts that cost time
- Sources are CRLF. Git Bash `sed -i` turns them into LF: pages say `sed -b -i` or the Edit tool.
- Host -Werror units link some files: nd_rec.c with nd_ds.c (geounit, hunit, nat5a, nmdl0, ns3unit, stunit), nd_ds.c alone (nst1, nset1),
  nd_direct.c with 3ds/tools/nd_direct_test.c (it defines nd_ap). nd_snap.c, nd_walk.c, nd_geom.c, nd_comb.c, nd_tex_set.c are not host-linked.
- Host seeds sed nd_rec.c text: `|| c == 1u;` (nat5a.sh), `rec_nat(nd_epm_run, 0u,` and `p[0] = NDC_HDR(NDC_NAPI, 1, 1);` (stunit.sh). Keep those strings.
- The Bash default timeout is 120 s. nd_host_test.sh needs `timeout: 600000`. bench.py run does not build: `PM_LANE=<id> sh 3ds/build.sh` first.
- The stream lives as long as its display context (2 chains, nd_rec.c:32-35). A core 0 pointer into the stream must not outlive the walk (nd_snap_hold).
- The c1t timers run in sampled steps only. The alt timer is per unit: subtract altu x cal ticks (cal 303 ticks per pair) for real ms.
- Model cache hit path (nd_recs.c:265-293) runs no rec_apply, no nd_msnap_use: core 0 state on a hit comes from hit_apply only.
- State leaks out of a called list on core 0 (nd_c0) but not on core 1 (scratch copy nd_rec.c:466, CALL :491-492). A field that moves from nd_c0 to a snap needs a carry back after the scratch run (NC0V: tex_on, fog).
- ms_cur getters (nd_msnap_w, _gvc, _td0) return the last nd_msnap_use: read each call order at the call site (T1FIX fault).
- nd_set.c holds several setters per line: rows that touch nd_set.c:84-111 or adjacent NdC0 lines in nd.h conflict; give them WAIT lines.
- PERF0 altw and PERF1 snapi lines are cumulative from boot: compare a tip run with its base log, never with a steady-state rate.
- Snap memo: a new snap needs a new command between draws (sn_cmds, nd_snap.c:60). nd_snap_alt and nd_snap_now force the next snap new.
- Core 1 runs while core 0 walks. Det runs are serial: a race does not show in a bench.
- The main checkout moves while an oracle reads (merges): grep line numbers again before a page is released.
- Oracles cannot start coders (permission denied): the orchestrator starts them.
## What to do first
1. `git log --oneline -15`; `ls docs/handoff`. Answer coder questions (SendMessage, 10 lines).
2. NC0V: step 1 merges as expected-diff (marker texfog; base gv read a stale nd_c0.tex_on after unarmed list, particle and rect snaps). Step 2 runs after that merge, byte-exact.
3. NC0M, NC0T1: byte-equal by construction; a DIFF is a missed site or a fault. NC0F step 1: a `segdiff` line s1 breaks the premise (stream units need the table):
   then core 1 must resolve the stream addresses (nd_rec_nr) before step 2.
4. Write NC0T2 after a reader read of nd_tex_set.c setters (the walker texture model, the glyph bind, nd_rect.c, nd_ent.c, pm_gpu.c readers).
