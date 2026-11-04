# investigate_sleep_hang: console hangs on a black screen after a short lid close

Read-only. No source edit. No Azahar start (Azahar cannot reproduce sleep).
Branch native, HEAD 8fca53deb. Tags: PROVEN (line read, or libctru source read) or INFERRED (reasoning, not seen on hardware).
This file replaces the sleep part of docs/handoff/diag_sleep_hang.md. That note says the sleep hook waits without a timeout. That is no longer true on this branch (see section 2).

## 1. Cause (INFERRED)

After wake, the game thread waits with no timeout for a GPU or GSP event that does not arrive. The most likely wait is in the frame begin: C3D_FrameBegin waits for the previous GPU frame, and that frame never completes after sleep.
The wait that blocks forever is one of these (all PROVEN as unbounded waits, the failing one is INFERRED):
- A. Frame begin, pm_gpu.c:1956 `C3D_FrameBegin(... C3D_FRAME_SYNCDRAW ...)` when pm_interp_on > 0. Calls C3D_FrameSync, renderqueue.c:169.
- B. C3D_FrameSync, renderqueue.c:75-86. Loops on `gspWaitForAnyEvent()` until both VBlank0 and VBlank1 frame counters change. Only the GSP VBlank interrupts advance them.
- C. C3Di_WaitAndClearQueue(-1), renderqueue.c:170-171 (called from C3D_FrameBegin). Waits for the GX queue to finish (P3D interrupt). Timeout -1 means forever.
- D. libctru aptHandleSleep, apt.c:843 `LightEvent_Wait(&aptSleepEvent)`. Signalled only at apt.c:599, and only when apt.c:597 `if (!aptIsActive()) break;` does not drop the wake first.
- E. Pacing wait, pm_main.c:404 `gspWaitForVBlank()`, no timeout (libctru gspgpu.c:291-297 has no timeout form).

Why it fits the report: citro3d hooks only ONSUSPEND and ONRESTORE (base.c:42-52). It does not hook ONSLEEP or ONWAKEUP. So after a sleep the GX queue state and the VBlank callbacks are not reset or re-armed. If the GSP stops sending the VBlank or P3D interrupt through the sleep, B or C waits forever. The black screen fits: no new frame is drawn or swapped, and the LCD force-black flag is cleared at apt.c:847 on wake.

## 2. What the code does on sleep (PROVEN by read)

- Sleep hook runs on the game thread, from aptMainLoop (apt.c:853, pm_main.c:363). Calls: apt.c:841 ONSLEEP, apt.c:843 wait, apt.c:848 ONWAKEUP.
- Our ONSLEEP hook is bounded: pm_log.c:757-773. Audio 300 ms, render 200 ms, flash 400 ms (pm_step_timed, pm_log.c:713-745).
- Our ONWAKEUP hook is bounded: pm_log.c:775-787 (audio 300 ms, render 200 ms). pm_dspa_rearm runs only when both are idle.
- pm_io_quiet gate: pm_log.c:699-711. Flash thread waits at pm_flash.c:65 while the gate is set. Not on the game thread.
- Watchdog: pm_wdog.c:59 skips the count while `!aptIsActive()` (after sleep it is active again). After 10 s with no progress (pm_wdog.c:41, `limit_s` 10), it calls pm_crash_line (pm_log.c:907) and pm_exit_process (pm_wdog.c:28-37).
- pm_crash_line calls fsync (pm_log.c:907-925) and crash_flush calls fsync (pm_log.c:868-900). Both are unbounded. If the SD card is still asleep, the watchdog thread can block too. Secondary cause (INFERRED).

## 3. Sample code and other homebrew (checked)

- libctru sample and library: sleep is handled by aptHandleSleep (apt.c:835-849). The app is expected to return from the wait. libctru does not hook GSP on sleep (gspgpu.c has no aptHook). libctru does reacquire the LCD (apt.c:847).
- citro3d: hooks ONSUSPEND and ONRESTORE only (base.c:36-60). It re-enables VBlank callbacks on restore (renderqueue.c:102-105).
- Not checked: other homebrew sources (no network access beyond the libctru and citro3d files above). The fix below uses only what libctru and citro3d expose.

## 4. Jr Troopa splash hang (investigate_nok11_splash.md)

Same class of wait. The splash hang blocks in gspWaitForP3D (pm_gpu.c:1275, no timeout) during a large GPU load. A sleep hang blocks in the same frame path (A to C). Possible shared cause: the GPU or the GSP stops completing work, and the game waits with no timeout. INFERRED. Not proven to be one bug.

## 5. Fix proposal

1. Make the frame waits timed. Replace the unbounded waits on the game thread (pm_gpu.c:1956, pm_main.c:404) with a deadline. On timeout, log "gpu: no frame event for N ms" and skip the frame. This is a small change, but the game then runs with no GPU. It only shows the failing wait.
2. The real repair is a GPU re-init on ONWAKEUP (pm_log.c:795). This is larger and is not proposed here until the test below names the wait.

## 6. Console test (user runs it, no Azahar)

1. Run the push command from CLAUDE.md rule 6 with a build that logs the watchdog (default `wdog` 10 s).
2. Close the lid for 5 s, open it. Wait 15 s. Then read the live log.
3. Read the phase in the watchdog line `watchdog: no progress for 10 s, game thread last phase N (name)`:
   - "gpu begin (frame begin)" points to A, B or C (pm_gpu.c:1956, renderqueue.c:169-171).
   - "idle wait for vblank" points to E (pm_main.c:404).
   - "loop top" points to D (apt.c:843).
   - no watchdog line and no "sleep: wake" line means the wake never reached the game thread.
4. Pass for this check: the log has "sleep: wake", then "sleep: audio resumed" or "audio not resumed", then frame lines. Fail: the last line is "sleep: wake" or a watchdog line with the phase above.
5. Repeat with a 30 s close. A 5 s close that passes and a 30 s close that fails narrows the cause to the GSP or GPU state lost during a long sleep.
