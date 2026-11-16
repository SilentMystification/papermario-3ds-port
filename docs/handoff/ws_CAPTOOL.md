# ws_CAPTOOL (Haiku): the PC side of a console log capture: log profile, push echo, diagnosis, retry, capture check in Azahar.
Objective (FACT, oracle code read, main 9ecf66f859). Push 5824 `--profile log` gave `sent-no-log rc 6`. Cause: ws_CAPLOG.md lines 2-5.
- The log profile has no `log=on` (bench_defs.json:7). A saved options.ini `log_on=0` then keeps the log gate off (pm_options.c:229, pm_log.c:352).
- The push echo omits livelog (push_3ds.sh:394, :400); the real argv has it (:395, :402).
- pm_logrecv.py prints only `could not connect` (pm_logrecv.py:130). It has no retry.
- No tool proves a capture before a push. The Azahar slot copies the user's options.ini (run_azahar.ps1:231).
Counters: none. Model: Haiku. Worktree build3ds/wt/CAPTOOL, branch CAPTOOL. PM_ROW=CAPTOOL PM_LANE=CAPTOOL on every bench call.
Files (edit no other file): 3ds/tools/bench_defs.json (line 7 only), 3ds/push_3ds.sh, 3ds/tools/pm_logrecv.py, 3ds/tools/livelog.py,
3ds/tools/test_pm_logrecv.py, 3ds/tools/azt.ps1 (line 520 only), 3ds/run_azahar.ps1 (after line 290 only). New: 3ds/tools/capture_check.py,
3ds/tools/test_capture_check.py, 3ds/tools/azt.d/CAPCHK.json, 3ds/tools/bench.d/CAPCHK.json, 3ds/tests/host.d/captool.sh.
Parallel with ws_CAPLOG. The game line `#PMLIVE log_on 0 live 3 disk 3 prof 0` is the shared format (pm_livesrv.h of CAPLOG).
## Edits (exact text in ws_CAPTOOL_code.md, parts A-K)
1. bench_defs.json:7. Before: `    "log": "prof=off phase=off loglevel=debug conlevel=info",`
   After: `    "log": "log=on prof=off phase=off loglevel=debug conlevel=info",`
2. push_3ds.sh: part A (one argv for the echo and the send).
3. pm_logrecv.py: parts B, C, D. livelog.py: parts E, F. test_pm_logrecv.py: part G.
4. capture_check.py: part H. test_capture_check.py: part I. azt.ps1 and run_azahar.ps1: part J. CAPCHK.json files and captool.sh: part K.
## Loop (1 build, 2 Azahar starts)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/captool_host.txt 2>&1`.
   Pass: `ND HOST TESTS PASS`, `captool ok`. Commit `CAPTOOL 1`.
2. One build of the lane (`sh 3ds/build.sh > build3ds/out/captool_build.txt 2>&1`). Pass: rc 0. The Azahar runs need it.
3. Gate run (the new log profile, worst options.ini in force):
   `python 3ds/tools/capture_check.py --row CAPTOOL --proof "fxtest: spawn 87,saved log_on=0 live=-1 disk=-1" > build3ds/out/captool_gate.txt`
   Pass: exit 0, the line starts `CAPTURE CHECK OK` and has `log=on`. (`saved log_on=0 ...` proves PM_OPT_WORST, pm_options.c:249.)
4. Seeded fault (the old profile words, no log=on):
   `python 3ds/tools/capture_check.py --row CAPTOOL --proof "fxtest: spawn 87" --raw "livelog prof=off phase=off loglevel=debug conlevel=info" > build3ds/out/captool_seed.txt`
   Pass: exit 1, the line starts `CAPTURE CHECK FAIL` and has `log gate OFF`. GUESS (code read): the run ends rc 10 (no until line):
   with logging off pm_log_frame_tick does not wake the writer (pm_log.c:600) and the sink waits (pm_log.c:888). `git status` must show no change after it.
## Fix scope, stop and hand-back
A host test failure: fix only the files of this page. Do not change the #PMLIVE text, the diagnosis text or the gate line text.
A `CAPTURE CHECK TOOL FAULT` line, or an azt.ps1 SCENARIO line in step 3: tool fault, stop the gate (CLAUDE.md rule 4), send the output path to the oracle.
Stop: host PASS, seed FAIL as above, gate OK. Final commit `CAPTOOL 2: gate` with the two result lines. It deletes this page and
ws_CAPTOOL_code.md. Hand-back: build3ds/out/handback_CAPTOOL.txt (line 1 commit hash, seed line, gate line, counters: none).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle (SendMessage). Then stop. Designer: the oracle that sent part L (ws_CAPTOOL_code.md). Reply to the `from` id of its message.
Orchestrator, after Merge: CLAUDE.md rule 3 gets `Before a console log push: python 3ds/tools/capture_check.py --row <id> --proof "<lines>"
<push game words>. Push only after CAPTURE CHECK OK.`
