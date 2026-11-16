# ws_FLIPLOG (Haiku): launch switch flipfps prints one live log row per drawn frame (flip lag capture)
Designer to ask: the oracle that wrote this page (the orchestrator knows its id). Worktree build3ds/wt/FLIPLOG, branch FLIPLOG.
PM_ROW=FLIPLOG PM_LANE=FLIPLOG on every build and bench call. Edit only the files of ws_FLIPLOG_code.md. No counter (C1..C12) moves.
Do not edit pm_fpsmode.h, pm_main.c, pm_options.c, pm_port.h (FSKIP2), 3ds/build.sh (STALEOBJ), tools/gset.py, gset.d, gfxin (GSETR).
## Objective
- Row form: `flipfps f<frame> cpu <us> wait <us> gpu <us> off <n> soff <n> spl <n> reuse <n>`. One row per drawn frame, printed at the
  next frame begin (the GPU time of a frame is known only there: pm_gpu.c:2929, the prof gpu field reads it the same way, pm_phase.c:78).
- Switch off (default): no row, no tick read. Cost: 3 cached word compares per drawn frame (pm_ff_on, the PMDL form of pm_port.h:68).
- Sink: the live log only (new pm_log_live_line, flag bit 3). Never the console. log.txt only with profdisk=1, as a prof line (pm_log.c:1001).
- ARM11: the row code is off the hot path. No division (tick to us is a multiply and a shift). No double.
## Files, in this order (exact text in ws_FLIPLOG_code.md)
1. New 3ds/include/pm_flipfps.h, 3ds/src/pm_flipfps.c (3ds/CMakeLists.txt:346 globs src/*.c: no build edit).
2. New 3ds/tools/pm_flipfps_test.c, 3ds/tests/host.d/fliplog.sh. 3. 3ds/src/pm_gpu.c (4 edits). 4. 3ds/src/pm_log.c (4 edits).
5. New 3ds/tools/azt.d/FLIPLOG.json, 3ds/tools/bench.d/FLIPLOG.json (scenario and bench fliplog_fsel, one commit).
## Edit shape (one example, 3ds/src/pm_gpu.c:3223)
Before: `    ndunit_scan(); rd_sync(); C3D_FrameEnd(GX_CMDLIST_FLUSH);`
After:  the same line, then
`    if (__builtin_expect(pm_ff_on(), 0)) pm_ff_end(pm_frame_count(), pm_ticks(), off_used, off_sused, grab_split + kick_done, OFF_N); /* FLIPLOG: the counts of this frame */`
Every call of pm_ff_begin, pm_ff_wait, pm_ff_end stands behind the switch test on its own line. fliplog.sh checks this.
## Loop (1 build, 3 Azahar runs in 2 gates)
1. Make all edits. Run `sh 3ds/tools/nd_host_test.sh > build3ds/out/fliplog_host.txt 2>&1`.
   Pass: lines `fliplog ok 6/6`, `fliplog gate ok` and the last line `ND HOST TESTS PASS`.
2. `PM_LANE=FLIPLOG PM_ROW=FLIPLOG sh 3ds/build.sh > build3ds/out/fliplog_b1.txt 2>&1`. Pass: rc 0, 0 errors, memcheck `OK`.
3. Commit: `FLIPLOG 1: switch flipfps: one live log row per drawn frame (pm_flipfps.c, 3 hooks in pm_gpu.c, live-only line in pm_log.c);`
   `host fliplog 6/6 with a seeded gate fault; hw: off = 3 cached compares per frame, no division, no double`.
4. Byte gate (switch off): `python 3ds/tools/bench.py base spot_mac01_drec --commit $(git merge-base HEAD main)`, then
   `python 3ds/tools/bench.py run spot_mac01_drec --row FLIPLOG --why "FLIPLOG byte gate, switch off"`, then
   `python 3ds/tools/bench.py cmp spot_mac01_drec --row FLIPLOG --commit $(git merge-base HEAD main)`. Pass: byte-equal (1 bit fails).
5. Capture check: `python 3ds/tools/bench.py run fliplog_fsel --row FLIPLOG --why "FLIPLOG capture words in Azahar"`.
   Pass: rc 0 and the until line (a flipfps row) matched. run_azahar.ps1:193 adds profdisk=1, so the row reaches log.txt.
## Fix scope
- A failing host test: fix only your edit. Never change an expected value, a seed or a regex of the test (the values are computed).
- A compile error: fix only the edited line. A tool fault in step 4 or 5: stop and send the output path to the oracle.
## Stop and hand-back
Stop: steps 1-5 pass. Final commit `FLIPLOG 2: gate` with the cmp line. It deletes ws_FLIPLOG.md and ws_FLIPLOG_code.md.
Hand-back: build3ds/out/handback_FLIPLOG.txt. Line 1 commit hash. Line 2 `host fliplog ok 6/6, fliplog gate ok, ND HOST TESTS PASS`.
Line 3 build rc, errors, memcheck line. Line 4 the cmp line. Line 5 the fliplog_fsel until line. Line 6 `counters: none`.
Line 7 the files changed. Last line: `FLIPLOG done`. Two tries per named cause, then send the output path to the oracle.
