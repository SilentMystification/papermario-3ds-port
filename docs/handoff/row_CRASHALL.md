# Row CRASHALL: every port thread exits cleanly on a crash (user, 2026-11-09)

## User facts
"In the case of something that crashes the game, can we wrap the whole game in a try catch block so we can gracefully quit when
something terrible happens?" The XL crash of 2026-11-09 (row LINRACE) was on the prefetch thread: Luma caught it (crash dump,
console frozen until reset) because that thread has no handler.

## Today (orchestrator, code read)
C has no try/catch; the 3DS equivalent is a per-thread CPU exception handler. pm_main.c:69 crash_handler (writes [CRASH] lines,
crash.txt, then pm_exit_process: back to the HOME Menu) is installed by pm_crash_install on the game thread (pm_main.c:785), the
logic thread (pm_logic.c:55), the audio thread (pm_audio.c:98) and the audio render thread (pm_au2.c:156). NOT installed on:
pm_prefetch.c:64 pf_main, pm_romload.c:181 rs_loader, pm_flash.c:159/206, pm_log.c:1010/1090/1255 (writer, step, sink),
pm_phase.c:348/415 (c2_main, c1_main), pm_wdog.c:153 wd_main. The handler is one-at-a-time (shared crash_stack); a handler stack
per thread or a lock is needed.

## Start state
`sh 3ds/tools/wt_new.sh CRASHALL`, branch CRASHALL. PM_ROW=CRASHALL, PM_LANE=CRASHALL. One coder agent.

## Steps
1. One helper pm_thread_create(fn, arg, stack, prio, core, name) that creates the thread and installs the handler at the thread's
   start (each thread gets its own small handler stack, 4 KB, from one static pool; a thread name for the [CRASH] line). Every
   threadCreate in 3ds/src uses it (host grep in nd_host_test.sh fails on a direct threadCreate outside the helper).
2. The handler on a second thread: if a crash is already being handled, that thread waits (no second dump); the first ends the
   process. Keep the save flush rule (pm_flash) as today.
3. Test: debug word crashtest=<thread name> makes that thread do a null write once; one Azahar run per two threads (prefetch,
   rs_loader): the log has the [CRASH] line with the thread name and the process ends (Azahar may not call the guest handler: if so,
   say so and gate on the host grep + code read only).
4. Gates: nd_host PASS (with the grep), build OK, memcheck OK, spot_mac01_det byte-equal. Commit; delete this row file.
   Budget: 3 Azahar starts.

## Touch only
3ds/src/pm_main.c (handler), the threadCreate call sites listed above, a small pm_thread.c/.h, nd_host_test.sh.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
