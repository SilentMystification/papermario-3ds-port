# ws_AZHANG: bench and capture runs that never end (tool fault)
Counter: none (tool fault). Part A: orchestrator, now. Part B: row AZHANG, Haiku (closed edits), after Part A.
## Facts (read-only, 20:22 to 20:31)
F1. FACT: build3ds/azslots/queue has no .json entry and no azahar.exe runs. No run got past azt.ps1:549. Last pool.log line: 19:12:34.
F2. FACT: each first run waits on one child powershell.exe with CPU 0 s. S9GATE python 48928 -> 49028 (19:16:32). NAT1 python 37828 -> 23108 (19:56:32). NATM capture_check 47620 -> bench.py 12524 (GUESS by elimination) -> 63648 (19:55:19).
F3. FACT: each powershell starts 1 s after the status line (bench.py:1019: 19:16:31, 19:56:31). NATM has no status_NATM.txt, so it stops in queue_line (bench.py:1227, before 1229).
F4. FACT: queue_line (bench.py:1049-1050) uses subprocess.run timeout=120. On Windows, CPython then calls kill() and communicate() with no timeout. A child that does not exit keeps the pipe open, so the call never returns.
F5. FACT: killed processes do not exit now. `timeout 9 powershell Get-CimInstance` (msys 537815, 20:05:50) still runs at 20:27. WmiPrvSE.exe 8592: CPU 34:57:37, 122 s more about 5 min later.
F6. FACT: since 19:12:47 these do not move. Post-merge host test: src_sync.sh:25, paste 66480 and sed 49628 wait on a pipe (post_merge_host.txt ends "STAGE checks"). Main build.sh msys 532006: main_build_orch.txt ends "STAGE copy-out" (build.sh:394).
F7. FACT: 0 bytes is buffering. bench.py prints with no flush to a file (bench.py:1039, 2936). In post_merge_30eaaef405.txt the bench.py lines come after azt.ps1 "LIST DONE".
F8. FACT: no post-merge lock blocks a coder. The runner holds only build3ds/out/postmerge.lock (postmerge_bg.sh:20). hostslots is empty. buildslots/slot1.d is main build 532006 (1 of 6 slots).
F9. FACT: later runs wait on the base lock of the first run (bench.py:1858-1895). S9GATE python 63656, 64476 on f2ab50d38d (pid 48928). NAT1 python 24508 on 82e8b59c86 (pid 37828). They stop with rc 6 after 40 min (bench.py:1812): about 20:38, 20:45, 20:44.
F10. FACT (postmerge_runner.txt): 6 jobs 18:10-19:05 took 3 min 31 s to 7 min 22 s. Job 30eaaef405 is in step 4 since 19:12:44.
Cause: OPEN root, GUESS an OS stall (F5, F6). Tool part, FACT: bench.py:1049 cannot end a hung child. azt.ps1:546 -> azslot.ps1:339 -> 293 calls Get-CimInstance with no timeout before the queue write.
## Part A: orchestrator order
1. Start no run, build, host test or CIM query.
2. Stop python 63656, 64476, 24508. Safe: they hold no lock file, slot or queue entry.
3. Stop powershell 49028, 23108, 63648. Then stop python 48928, 37828, 12524, 47620. Safe: queue empty, no Azahar. A lock of a dead pid goes at the next take (bench.py:1874-1878).
4. Stop your own: powershell 64116, 44320, msys 537744, 537815, main build msys 532006, 532371. A dead build slot is reaped (build_slot.sh:26-50).
5. Stop post-merge msys 531305, 531339, 532280, 532332, 532333, 532341, 532342. Safe: the next runner removes a dead lock (postmerge_bg.sh:23-26). The queue keeps 7e9bdf6bbe (child of 30eaaef405). Start: `nohup sh 3ds/tools/postmerge_bg.sh --run >> build3ds/out/postmerge_runner.txt 2>&1 &`.
6. A process that stays after a stop (F5): user decision. Default: end WmiPrvSE.exe 8592 (admin). Second default: reboot.
7. Test once: `timeout 30 powershell -NoProfile -Command "(Get-CimInstance Win32_Process -Filter 'ProcessId=4').ProcessId"` prints 4.
8. Run one at a time, each with PYTHONUNBUFFERED=1 and its own output file: NATM capture, NAT1 base 82e8b59c86, S9GATE base f2ab50d38d. Both lane-base_* folders hold a 3dsx (no cold build).
## Part B: row AZHANG (edit only 3ds/tools/bench.py, 3ds/tools/azslot.ps1, 3ds/run_azahar.ps1, 3ds/tools/azt.ps1)
E1. bench.py:2936-2937. Before: `if __name__ == "__main__":` / `sys.exit(main(sys.argv[1:]))`. After: add `sys.stdout.reconfigure(line_buffering=True)` as the first line of the if block.
E2. bench.py:1049-1050. Use `p = subprocess.Popen(cmd, cwd=REPO, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)` and `out = p.communicate(timeout=60)[0]`. On TimeoutExpired: `p.kill()`, print "QUEUE: no summary in 60 s (azt.ps1 -Queue killed)", return. No second communicate.
E3. Azahar lookups use Get-Process, not CIM. azslot.ps1:40 before: `@(Get-CimInstance Win32_Process -Filter "Name='azahar.exe'" ... | Where-Object { $_.ExecutablePath -match ... })`. After: `@(Get-Process -Name azahar -ErrorAction SilentlyContinue | Where-Object { $_.Path -match ... })`. Same at azslot.ps1:131 and run_azahar.ps1:223, 257.
E4. Readers of E3: ExecutablePath -> Path, ProcessId -> Id at azslot.ps1:58, 63, 64, 66 and run_azahar.ps1:223, 226, 257, 258, 259. azslot.ps1:132 returns the process itself.
E5. run_azahar.ps1:119: Get-Process. Text: "pid $($_.Id), started $($_.StartTime), path $($_.Path)".
E6. Parent lookups keep CIM with `-OperationTimeoutSec 15`: azslot.ps1:293, azt.ps1:389. Get-ParentPid returns 0 on fault (Get-StaleReason skips parent 0, azslot.ps1:351).
Gate: `sh 3ds/tools/nd_host_test.sh` PASS. `powershell -NoProfile -File 3ds/tools/azt.ps1 -SelfTest` prints SELFTEST PASS. One `python 3ds/tools/bench.py run spot_mac01_drec --row AZHANG --why "AZHANG tool gate" > build3ds/out/azhang_run.txt 2>&1`: rc 0, and the file has the QUEUE line before the run ends. Budget 2 Azahar starts.
Stop: gate PASS, one commit. Fail: send the output path to the oracle. Hand-back: commit hash, gate lines, "counter none".
