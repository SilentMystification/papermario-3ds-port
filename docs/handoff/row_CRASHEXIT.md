# Row CRASHEXIT: a CPU fault exits through the clean shutdown, not svcExitProcess

(Model: Haiku. 60 lines or fewer. The agent reads only this file, CLAUDE.md and what this file names.)

## User facts (bugs only)
User (2026-11-11, console): "The crash handler still causes the console to hard crash and reboot. Can we recover and just go to the
home screen or the homebrew menu?"

## Cause (oracle code read)
- Every crash ends in svcExitProcess with APT, GSP (gfx), citro3d, ndsp and the other threads still live: crash_handler
  (3ds/src/pm_main.c:140-197) -> crash page -> pm_exit_process (pm_wdog.c:41-48: flash flush, log close, svcExitProcess). ESCAPE2 found
  that this path never runs aptExit/gfxExit/ndspExit; the system then hangs or reboots. pm_fatal_stop (pm_main.c:202-209) uses
  svcBreak(USERBREAK_PANIC), which the user handler never sees (the kernel ends or freezes the process).
- Faults the user handler gets (libctru threadOnException, pm_thread.c:39/67 for every port thread): prefetch abort, data abort, undefined
  instruction, VFP (ERRF types 0-3); a stack overflow is a data abort (the handler has its own stack); out of memory is a NULL return, then a
  data abort. Not catchable in user mode: svcBreak, a kernel panic, a stuck GPU (pm_wdog.c:134 APT_HardwareResetAsync stays: with a stuck
  GPU, C3D_Fini/gfxExit block; a reboot to HOME is the only exit).
- The handler cannot return (thread.h: "an ARM function that does not return"), but a libctru thread can end itself with threadExit.
  main (pm_main.c:936-957) runs the game in thread "game" and calls pm_shutdown after threadJoin: ending the faulting thread lets main
  run the clean exit (return from main -> hbmenu, or HOME after aptClearChainloader, as ESCAPE2 pm_esc_take pm_wdog.c:105-113).
- pm_crash_line keeps the log drain lock (pm_wdog.c:124-125 comment): pm_shutdown's pm_log_stop would block after a crash.

## Start
`sh 3ds/tools/wt_new.sh CRASHEXIT`; PM_ROW=CRASHEXIT PM_LANE=CRASHEXIT on every call. No other running row edits pm_main.c or pm_wdog.c.

## Touch only
3ds/src/pm_main.c: crash_handler (:194-196), pm_fatal_stop (:207), pm_shutdown (:920-934), one global near crash_taken (:27).

## Exact edits
1. pm_main.c after `static int crash_taken;` (:27): `volatile int pm_crash_exit; /* CRASHEXIT: a crash ended its thread; main exits clean */`
   and `extern volatile int pm_exit_request;`.
2. crash_handler: replace `pm_exit_process();` (:196) with:
   `if (threadGetCurrent() == NULL) pm_exit_process(); /* the main thread: no thread to end */`
   `pm_crash_exit = 1; pm_exit_request = 1; /* CRASHEXIT: the game loop ends, main runs pm_shutdown and returns */`
   `if (pm_opt.esc_mode != 0) aptClearChainloader(); /* esc mode quit or home: the close goes to the HOME Menu */`
   `pm_wd_crash_tick = (unsigned)(svcGetSystemTick() / (SYSCLOCK_ARM11 / 1000ull)) | 1u; /* 3 s for the clean exit, then the watchdog */`
   `threadExit(1);`
3. pm_fatal_stop: replace `svcBreak(USERBREAK_PANIC);` with `pm_crash_exit = 1; pm_exit_request = 1; if (threadGetCurrent()) threadExit(1); pm_exit_process();`
   (move the global of edit 1 above pm_fatal_stop if the compiler needs it; it is above already at :27).
4. pm_shutdown: replace `pm_flash_flush("exit");` with `if (pm_crash_exit) pm_flash_crash_flush(); else pm_flash_flush("exit");` and
   `pm_log_stop();` with `if (pm_crash_exit) pm_log_crash_close(); else pm_log_stop(); /* CRASHEXIT: the crash flush holds the drain lock */`.
5. The watchdog (pm_wdog.c:162-170) stays: if the clean exit hangs 3 s (a lock held by the dead thread, a stuck GPU), it calls
   pm_exit_process as today. No edit in pm_wdog.c.

## Gate (CLAUDE.md 2; Azahar never calls the guest handler, pm_main.c:199-200, so no Azahar run proves the path)
- `PM_ROW=CRASHEXIT PM_LANE=CRASHEXIT sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row CRASHEXIT --why "crash exit"`: PASS.
- One Azahar run of spot_mac01_det (one start) and `bench.py cmp spot_mac01_det --row CRASHEXIT`: 6/6 (no draw change).
- Console (user): a push with `crashtest=game` (pm_thread.c:40), A on the crash page: the console returns to hbmenu (esc mode hbmenu)
  or HOME (quit); crash.txt has the [CRASH] lines. The orchestrator asks for this push.

## Stop
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, then hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash, the gate results, the memcheck line.
