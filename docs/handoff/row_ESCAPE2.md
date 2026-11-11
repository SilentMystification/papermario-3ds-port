# Row ESCAPE2: START+SELECT exits through the normal exit path, Debug row "START+SELECT" picks the target

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## User facts (2026-11-10, O3DS XL, main 86004672a)
- "start+select quit doesnt work as expected, it kicks us out to the homescreen and restarts the system from a fault." XL log:
  `escape: START+SELECT 1 s, phase 2 steps 29470 stuck 0 gx n 1 cur 1 last 1 ... f7974` (GX queue idle, game loop running).
- "from the debug menu we should be able to configure the start+select behavior between homebrew menu, quitting and homescreen if possible."
## Cause (oracle code read)
- pm_wdog.c:110 `if (!stuck) pm_exit_process();` runs on the watchdog thread; pm_exit_process (pm_wdog.c:41-47) calls svcExitProcess
  while the game, logic, walker and audio threads run. It skips every step of pm_shutdown (pm_main.c:919-933) and of libctru exit:
  1. ndspExit (pm_audio.c:36) never runs: the DSP component stays loaded, its DMA points into the linear heap the kernel frees.
  2. C3D_Fini and gfxExit (pm_main.c:930-931) never run: GSP right, framebuffers and GX work of the dead process stay with GSP.
  3. aptExit never runs: libctru aptExit is the only sender of APT PrepareToCloseApplication (0x00220040) / CloseApplication
     (aptExitProcess, 0x00270044) or DoApplicationJump (0x00320084) (libctru.a apt.o disassembly). NS sees the active app die unclosed.
- The clean path exists: pm_exit_request (pm_options_ui.c:26) ends the loop (pm_main.c:534), main runs pm_shutdown and returns.
- libctru aptInit sets chainload-to-self when __system_runflags has RUNFLAG_APTCHAINLOAD (4) (Homebrew Launcher): a plain return goes
  to hbmenu; aptClearChainloader() first makes aptExit close the app to the HOME Menu; aptJumpToHomeMenu() is the HOME button (game
  thread only: it calls GSPGPU_SaveVramSysArea and the suspend hooks; aptMainLoop calls it the same way).
- The debug menu Close game (pm_options_ui.c) is a separate exit mechanism (user, 2026-11-10): row ESCAPE3. This row does not touch it.
## Start
`sh 3ds/tools/wt_new.sh ESCAPE2` (build3ds/wt/ESCAPE2, branch ESCAPE2 from main). PM_ROW=ESCAPE2 PM_LANE=ESCAPE2 on every build and bench call.
Never at the same time as ESCAPE3 (both edit 3ds/tools/azt_scenarios.json); ESCAPE2 runs first.
## Touch only
3ds/include/pm_port.h, 3ds/src/pm_options.c, 3ds/src/pm_wdog.c, 3ds/src/pm_main.c, 3ds/tools/azt_scenarios.json, 3ds/tools/bench_defs.json.
## Done (build3ds/wt/ESCAPE2)
Edits 1-5 of the first version are committed: dc26fa7d5 (code), 48d389a01 (scenario wd_escape_run and its bench). nd_host PASS, memcheck OK.
## Gate fail (run 212905, 1 of 3 checks) and its cause (oracle code read)
- wd_escape (pm_wdog.c:121-124) still writes the escape line with pm_crash_line before the clean path. pm_crash_line calls crash_flush
  once (pm_log.c:1352-1354). crash_flush (pm_log.c:1303-1325) sets crashed = 1, keeps drain_lock and fsink_lock taken and sets writer_on 0.
- After that no queued line reaches log.txt (vlog, pm_log.c:429-479, only queues; the writer skips the drain when crashed, :981, :995).
  So "escape: taken" and "exit: audio job done" were made but not written.
- The game loop took the request (wd_escape returned: no "did not take" line), the loop ended after phase 10, main ran pm_shutdown,
  and pm_log_stop (pm_main.c:929) blocks for ever at pm_log.c:1296 `LightLock_Lock(&drain_lock)` (held by the watchdog's crash_flush).
  pm_wd_prog stopped, so the watchdog stall path (pm_wdog.c:191-193) ended the process with svcExitProcess after 10 s: the old fault path.
## Next edit (pm_wdog.c wd_escape only; one try)
In wd_escape, replace everything from `buf[n++] = '\n';` (after the pm_gpu_hang_dump line) to the `}` that closes `if (!stuck) {` with:
```
    buf[n] = 0;
    if (stuck) { buf[n++] = '\n'; buf[n] = 0; pm_crash_line(buf, n); } /* GPU stuck: the crash path, then the reset below */
    else { /* ESCAPE2: no pm_crash_line before the clean exit: its crash_flush (pm_log.c:1303) stops the log writer and keeps drain_lock,
            * so pm_log_stop in pm_shutdown would block for ever. The line goes to the log queue and to crash.txt (wd_note: no crash state). */
        pm_log_evt("%s", buf);
        buf[n++] = '\n'; buf[n] = 0; wd_note(buf, n);
        pm_esc_req = 1 + pm_opt.esc_mode;
        for (int i = 0; i < 30 && pm_esc_req; i++) svcSleepThread(100000000LL);
        if (!pm_esc_req) return;
        static const char m[] = "escape: the game loop did not take the request in 3 s, hardware reset\n";
        pm_crash_line(m, (int)sizeof m - 1);
    }
```
The lines from APT_HardwareResetAsync to the end of wd_escape stay. Commit "ESCAPE2: escape line through the log queue", then the gate.
## Gate (CLAUDE.md 2: no picture; no draw code changes, so no byte compare)
- `PM_ROW=ESCAPE2 PM_LANE=ESCAPE2 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`, memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row ESCAPE2 --why "ESCAPE2 build check"`: PASS.
- `python 3ds/tools/bench.py run wd_escape_run --row ESCAPE2 --why "ESCAPE2 running exit path"`: LOG CHECKS PASS (3 of 3).
- Quit, HOME Menu and the hung-game reset: the user's console check.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
