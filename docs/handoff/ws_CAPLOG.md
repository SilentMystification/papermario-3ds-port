# ws_CAPLOG (Haiku): the live log socket accepts and answers sendlog with logging off, and tells the client the log gate.
Objective (FACT, oracle code read, main 9ecf66f859). Push 5824 with `--profile log` got `connected`, then no line in 60 s.
- The log gate is `qon() = log_file_on || prof_mode` (pm_log.c:73). With qon() 0 the writer waits with no timeout (pm_log.c:1040)
  and skips the pass (pm_log.c:1043). Only that pass calls accept_client (pm_log.c:1066). pm_log_poll accepts only before the writer runs (pm_log.c:241).
- options.ini `log_on=0` sets log_file_on 0 (pm_options.c:229). The menu Profiling row saves log_on 0 (pm_options.c:269). The words
  `prof=off phase=off` set no Profiling (pm_options.c:278). The log profile has no `log=on` word (bench_defs.json:7). The prof profile works: prof=30 sets prof_mode 1.
- No accept: the 3DS completes the TCP connect into the backlog of 2 (pm_log.c:429) and sends nothing. GUESS: a full backlog drops new SYNs (PC timeouts).
Counters: none. Model: Haiku. Worktree build3ds/wt/CAPLOG, branch CAPLOG. PM_ROW=CAPLOG PM_LANE=CAPLOG on every bench call.
Files: 3ds/src/pm_log.c, 3ds/include/pm_livesrv.h (new), 3ds/tools/pm_livesrv_test.c (new), 3ds/tests/host.d/caplog.sh (new). Edit no other file.
Parallel with ws_CAPTOOL (disjoint files). The line `#PMLIVE log_on 0 live 3 disk 3 prof 0` is the shared format: do not change it.
## Edits (exact text in ws_CAPLOG_code.md)
1. New 3ds/include/pm_livesrv.h (code page part A). pm_log.c:18: after `#include "pm_logfmt.h"` add `#include "pm_livesrv.h"`.
2. accept_client, pm_log.c:163-164. Before: `    int fd = accept(listen_fd, (struct sockaddr*)&addr, &len);` / `    if (fd < 0) return;`
   After: the same 2 lines, then the drain loop of code page part B (take every waiting connection, keep the newest).
3. accept_client, pm_log.c:171. Before: `    live_sent = ring_pos - ring_len; /* LOGX1: the pump sends the ring from its oldest byte */`
   After: the same line, then the 3 hello lines of code page part C.
4. writer_main, pm_log.c:1039 and :1043: replace as in code page part D (a listen socket keeps the 250 ms pass, accept and sendlog with logging off).
5. New 3ds/tools/pm_livesrv_test.c (part E) and 3ds/tests/host.d/caplog.sh (part F, LF endings).
## Loop (1 build, at most 2 Azahar starts; base runs not charged)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/caplog_host.txt 2>&1`. Pass: `ND HOST TESTS PASS` and `caplog ok`. Commit `CAPLOG 1`.
2. One build (`sh 3ds/build.sh > build3ds/out/caplog_build.txt 2>&1`). Pass: rc 0 and memcheck margin above 0.
3. Gate: for B in spot_mac01_drec sbk14_drec: `python 3ds/tools/bench.py base $B`,
   `python 3ds/tools/bench.py run $B --row CAPLOG --why "CAPLOG gate"`,
   `python 3ds/tools/bench.py cmp $B --row CAPLOG --commit $(git merge-base HEAD main)`. Pass: byte-equal (one differing bit fails).
## Fix scope, stop and hand-back
A host test failure: fix only caplog.sh, pm_livesrv_test.c or edits 1-4. A gate that differs: write the cmp output to build3ds/out/,
send the path to the oracle (SendMessage). Do not change the hello text or the edit shape to pass a test.
Stop: host PASS, build rc 0, margin above 0, both benches byte-equal. Final commit `CAPLOG 2: gate` with the values. It deletes this page
and ws_CAPLOG_code.md.
Hand-back: build3ds/out/handback_CAPLOG.txt (line 1 commit hash, gate values, memcheck margin, counters: none).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle (SendMessage). Then stop.
FACT: no bench connects to 17492. The user checks on the console: a push with `--profile log` and options.ini log_on=0 gives
`#PMLIVE log_on 0 ...` on the PC at once, and `pm_logrecv.py --ip <ip>` gets log.txt.
