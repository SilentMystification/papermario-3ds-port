# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main abf33eaaf5 (T1FIX, NC0S, NC0V, NC0M, NC0T1, NC0F merged; NC0T2 runs; ALT2C gated 0464762731) plus pages of oracle NATIVE17, 2026-11-20.
## Goal
- User: better frame time on the O3DS, work spread about evenly over core 0 and core 1, core 0 decides nothing, good ARM11 code, good PICA use.
- The 60 fps overview: docs/handoff/plan_60fps.md (budget, hot areas, order of attack). The plan: docs/handoff/plan_perf.md (findings with file:line, ranked rows, section 6a MCOPY, section 7 nd_c0 fields left, section 10 questions).
- Numbers (FACT console capture build 7569): core 1 44.4 ms, core 0 24.0 ms, ratio 1.86, ft 46.5, fps 42.2; c1a m 10975 words per step (0 after MCOPY2). c0gate 0 / 0 / 1 (nd_c0 now timg, tlut, tlut_n, tst1, td; NC0T2 step 2 deletes it: 0 / 0 / 0).
## Tools on main
- `python 3ds/tools/c0gate.py <elf>`: calls / interp / state. Run on the ELF of the same tree (a clean worktree or git archive).
- `bench.py base <bench> --commit $(git merge-base HEAD main)`, `bench.py run a,b --row <id>`, `bench.py cmp <bench> --row <id>`.
- Gate benches (drec): spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec, hudfb_drec; entcapd_kgr01_drec (darkness stencil).
- drec end lines (pm_gpu.c:628-632): `c0src ... wbind` (binds from walker tiles), `altw ... c1 m ... c0 all ... m` (copy words). `drec frame <F> seq <S>` (:609).
## Open work (order)
1. MSNAP2 and PROF3 merged (MSNAP2: snn 19572 to 3957, spot_mac01 f201). Capture 1 (plan_60fps.md section 5 words, prof mode, no phase=on) reads c1g.
   The first capture 1 ran with phase=on: s0, all timers 0.00 (live_cap1_170.txt). The push of the retry failed (cap1b is the same run).
2b. GAMEDRAW (oracle NATIVE18): plan_gamedraw.md (ranked rows, section 3; status bar core 0 section 1.6). Pages: ws_GD9A (status bar block census, Haiku), ws_GD7 (core 1 memo, after GD9B), ws_PROF4B (profile lane overhead). DO3 needs DO3F with fps60. wk holds waits (GDW).
2c. CRITPATH (oracle NATIVE19): plan_critpath.md. ft = core 0 work 25.2 (pmu cyc) + GPU waits about 15; core 1 off the chain (fw 9.88). CPT0, KICK3 merged. PIPE design: plan_pipe.md (rows PIPE0 to PIPE4), pages ws_PIPE0 (fence census), ws_PIPE1 (L lists at word PIPE_H). PIPE0, PIPE1 merged. IDLE (oracle NATIVE20): no merge regressed idle (FACT equal ndc windows: 353b31c792 ft 41.21, a05f62c545 ft 38.98). The 3 ms was the status bar (inventory.c:737-746): core 0 gbi +4.15, gx +2.14 (live_play2_170.txt f10494 vs f11094). GD7 cuts core 1 only. Page ws_CAPCMP (compare tool).
2d. WIDEWIN (oracle NATIVE20): plan_widewin.md (clip audit, 3 modes). Rows in order: ws_WCLIP0 (census, drec scissor regs 0x065-0x067: every drec base changes once), ws_WCLIP1 (nd_sci.c rule), ws_WCLIP2 (Fullscreen rects 1.25 x). WALK: plan_walk.md (ROM read drain of player rasters, pm_os.c:492-495), pages ws_STEPX, ws_ROMDR (merge after the STEPX capture).
2d. POND (oracle NATIVE21): plan_pond.md. mac_00 water solid blue: a DO1 memo hit inside the build_custom_gfx unit log (nd_rec.c:356, no lg_put) drops the copy record units. Pages ws_POND0 (benches, bisect), ws_POND1 (fix). drec frames hide DO1 faults (nd_rec.c:359): use pic benches.
2f. INTERP (oracle NATIVE23): plan_interp.md. K = L words + patches, GPU K = L (gpuk 7.73). L and K swaps about 8 ms apart, no vblank wait (dropped L, tear). Sky scroll not lerped. Pages ws_IK0, then ws_PRES0.
2e. MAPTRANS (oracle NATIVE22): plan_maptrans.md. Load step 240-266 ms (console xfer). Mechanisms: blanket drain per ROM read (each read posts an invalidation:
  a round trip per read), romfs .seam/.dlc waits in the load, inline first sight. Pages ws_MTX0 (census, start now), ws_MTX1 (reader stamps), ws_MTX2 (file jobs): MTX1, MTX2 need the user's yes on section 7.
2. DRAWOBJ (user yes 20 Nov): plan_drawobj.md, pages ws_DO0 (Haiku, gst.txt state hash tool) and ws_DO1 (Sonnet, model record memo).
   The user reads plan_drawobj.md before a DO coder starts. DO2 to DO4 pages after DO0 merges (DO3 needs gst). DO5 needs the user.
3. plan_perf.md section 11 rows C1, C3 (= DO1), C4 (= DO4), K1, K2 (= DO3): pick by the capture 1 numbers. Content cuts: none unless the user decides.
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
- Compare console windows only with an equal ndc line, nd dr tri and at pos (ws_CAPCMP). The status bar shows after 240 idle steps: the first window differs.
- Sources are CRLF. Git Bash `sed -i` turns them into LF: pages say `sed -b -i` or the Edit tool.
- Host -Werror units link some files: nd_rec.c with nd_ds.c (geounit, hunit, nat5a, nmdl0, ns3unit, stunit), nd_ds.c alone (nst1, nset1),
  nd_direct.c with 3ds/tools/nd_direct_test.c (it defines nd_ap). nd_snap.c, nd_walk.c, nd_geom.c, nd_comb.c, nd_tex_set.c are not host-linked.
- Host seeds sed nd_rec.c text: `|| c == 1u;` (nat5a.sh), `rec_nat(nd_epm_run, 0u,` and `p[0] = NDC_HDR(NDC_NAPI, 1, 1);` (stunit.sh). Keep those strings.
- The Bash default timeout is 120 s. nd_host_test.sh needs `timeout: 600000`. bench.py run does not build: `PM_LANE=<id> sh 3ds/build.sh` first.
- The stream lives as long as its display context (2 chains, nd_rec.c:32-35). A core 0 pointer into the stream must not outlive the walk (nd_snap_hold).
- The c1t timers run in sampled steps only. The alt timer is per unit: subtract altu x cal ticks (cal 303 ticks per pair) for real ms.
- C3D_FrameBegin stops the GX queue (citro3d renderqueue.c:97); gxCmdQueueAdd does not run a stopped queue (libctru gxqueue.c:62-75): a C3D_FrameSplit alone starts no GPU work.
- Model cache hit path (nd_recs.c:265-293) runs no rec_apply, no nd_msnap_use: core 0 state on a hit comes from hit_apply only.
- State leaks out of a called list on core 0 (nd_c0) but not on core 1 (scratch copy nd_rec.c:466, CALL :491-492). A field that moves from nd_c0 to a snap needs a carry back after the scratch run (NC0V: tex_on, fog).
- Gate rule seen this pass: a census line in a drec frame with a byte-equal cmp of that frame names words the draw does not read: accept it.
- An expected-diff marker must exist in the gate tree. When step 2 deletes the census, merge step 1 first (NC0V: texfog, then gstex).
- Step-1 omissions show as step-2 diffs (NC0V nd_sv_tex nd_gs write). Check the page's source list against the step 1 diff before step 2.
- ms_cur getters (nd_msnap_w, _gvc, _td0) return the last nd_msnap_use: read each call order at the call site (T1FIX fault).
- nd_set.c holds several setters per line: rows that touch nd_set.c:84-111 or adjacent NdC0 lines in nd.h conflict; give them WAIT lines.
- PERF0 altw and PERF1 snapi lines are cumulative from boot: compare a tip run with its base log, never with a steady-state rate.
- Snap memo: a new snap needs a new command between draws (sn_cmds, nd_snap.c:60). nd_snap_alt and nd_snap_now force the next snap new.
- Core 1 runs while core 0 walks. Det runs are serial: a race does not show in a bench.
- The main checkout moves while an oracle reads (merges): grep line numbers again before a page is released.
- Oracles cannot start coders (permission denied): the orchestrator starts them.
## What to do first
1. `git log --oneline -15`; `ls docs/handoff`. Answer coder questions (SendMessage, 10 lines).
2. NC0T2 step 1: a `bindsrc f s3` or `txsrc f` line means the rect or direct block differs from the walker model: read that draw's path first.
3. ALT2C: read the entw lines; design ALT2 for the biggest word reason (a refusal code removes whole records; a slice reason removes segments).
