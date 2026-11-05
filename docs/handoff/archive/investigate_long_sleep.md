# investigate_long_sleep: long lid-close sleep leaves the console asleep (black screen, LED pulses)

Read-only. No source edit. No Azahar start (Azahar cannot reproduce sleep).
Worktree C:\Users\Andrew\Documents\GitHub\papermario-3ds-port-main, branch native.
Tags: PROVEN (line read in 3ds/src, or grep result) or INFERRED (reasoning or libctru/citro3d behaviour not read in this run).
Line numbers are current (native branch). docs/handoff/investigate_sleep_hang.md cites pm_gpu.c:1956; the frame begin is now pm_gpu.c:1970.

## 0. Facts used

- User (console): a short lid close (seconds) works. A close of about 2 minutes leaves the top screen black and the power LED pulses as in sleep, also after the lid opens. The console needs a hard reboot.
- Reading of the new fact: the app never finishes the wake. The LCD is forced black until libctru reacquires it in aptHandleSleep. A black screen with the LED still pulsing means the wake is not handled by the game.
- libctru and citro3d sources are not in this worktree (no devkitPro install found). Calls into libctru below are carried from docs/handoff/investigate_sleep_hang.md and are INFERRED until re-read.
- Sleep hooks: pm_log.c:757-773 (ONSLEEP, sleep_enter_hook), pm_log.c:775-787 (ONWAKEUP, wake_hook), pm_log.c:792-796 (aptHook switch).
- aptMainLoop is called only at pm_main.c:373 (game loop head) and pm_gpu.c:2079 (the start-error screen). Each game iteration calls it once, before any frame wait.
- No call to aptSetSleepAllowed or aptSetHomeAllowed exists in 3ds/src (grep, PROVEN). Sleep is allowed by default.
- Thread that runs aptMainLoop: the game thread (pm_log.c:695 comment, pm_main.c:373).
- No pm_net*.c file exists in 3ds/src (PROVEN). The only network code is the live log in pm_log.c.
- No osGetTime call exists in 3ds/src (grep, PROVEN). Time uses svcGetSystemTick.

## 1. What the game does while the handshake is pending

The wake can only be handled when the game thread returns to aptMainLoop (pm_main.c:373). Any game-thread wait that does not return keeps the wake pending. The waits below are on the game thread and have no timeout (PROVEN unbounded by read):

| # | Call | File:line | Bounded? | Notes |
|---|------|-----------|----------|-------|
| U1 | C3D_FrameBegin(C3D_FRAME_SYNCDRAW), waits for the previous GPU frame (and for VBlank in the older mode) | pm_gpu.c:1970 | No (citro3d C3D_FrameSync / C3Di_WaitAndClearQueue, INFERRED) | Used when pm_interp_on > 0 (30-step mode per the PT88 comment, pm_gpu.c:1970). Default state not checked. |
| U2 | pm_audio_join_at, LightEvent_Wait(&au_done) | pm_audio.c:198-205 (callers pm_main.c:502, pm_phase.c:669, pm_nusys.c:144, pm_nusys.c:150, pm_main.c:678) | No | Called every drawn step (pm_main.c:502), after the draw. |
| U3 | pm_au2 wait_jobs, LightEvent_Wait(&ev_idle) | pm_au2.c:82-90, reached by pm_au2_sync (pm_au2.c:121-127) and from pm_audio.c:234-235 | No | Render-thread join. |
| U4 | gspWaitForP3D in jt_result | pm_gpu.c:1274-1278 | No | Only with the switch jumptest. Excluded. |

Bounded waits (checked, not the cause):
- Sleep hook joins: pm_audio_join_timed (pm_audio.c:215-222, 300 ms), pm_au2_sync_timed (pm_au2.c:114-127, 200 ms), flash step (pm_log.c:766, pm_step_timed pm_log.c:735-745, 400 ms). Wake hook: pm_log.c:781-782 (300 ms, 200 ms).
- Idle pacing: pm_main.c:398-410 (sleeps, breaks when t >= sched_next), vblank_wait_timed pm_main.c:315-322 (40 ms).

Key defect (PROVEN by read): the timed joins in the sleep and wake hooks do not clear au_busy. When the join times out, the job is still owed. The next game-thread call to pm_audio_join_at (U2) then waits with no timeout. The log line "sleep: audio join timeout" (pm_log.c:762) and "audio not resumed" (pm_log.c:785) are the only trace.

## 2. Ranked causes

Rank 1 (INFERRED, highest fit to the new fact): the game thread waits in a call that does not return after the long sleep.
- 1a. Wake never reaches the app. The app is inside aptHandleSleep (libctru, the wait after the ONSLEEP hook; apt.c:843 per the prior note). The log ends at "sleep: gate closed" (pm_log.c:772). The watchdog stays silent (see section 3).
- 1b. Wake reaches the app, but the game thread then blocks in U1, U2 or U3 before the next aptMainLoop. The log shows "sleep: wake" (pm_log.c:779) and then stops at a frame line.
Both are shown by the same log. Section 6 separates them.

Rank 2 (PROVEN unbounded wait, INFERRED trigger): U2 with au_busy still 1 after a timed-out join (pm_log.c:762 or pm_log.c:785). The audio job (core 1, pm_audio.c:130-146) or its ndsp/DSP call does not finish after the long sleep. The game thread then waits forever in pm_audio.c:204. This fits a state loss in the DSP or ndsp service, which the OS may power down over a long sleep. pm_dspa_rearm (pm_dspaud.c:398) runs only when both joins succeed (pm_log.c:782), so ndsp channels stay stale when they fail.

Rank 3 (PROVEN unbounded wait, INFERRED trigger): U1 in the default 30-step mode. citro3d waits for the GPU or VBlank events from GSP. After a long sleep those events may not come. The GSP event state is not re-armed by our hooks (no GSP call in pm_log.c hooks; PROVEN by grep of pm_log.c 757-796). libctru's own GSP handling on sleep was not re-read.

Rank 4 (PROVEN unbounded, INFERRED trigger): U3 render-thread join. Same family as rank 2. The render thread waits on ev_idle and does not wake if its work never completes.

Rank 5 (PROVEN calls, INFERRED risk): APT service calls on the game thread inside the hooks. cpu_apply calls APT_SetAppCpuTimeLimit (pm_log.c:687-688). The sleep hook calls it before libctru replies (pm_log.c:770). If the APT service waits for the app reply, the call can stall. Short sleep runs the same calls, so this does not explain the long-only trigger. Rank 5 keeps it as a check.

Ruled out for the hang (PROVEN by read):
- Sockets. The listen socket and the client are O_NONBLOCK (pm_log.c:108, pm_log.c:275). send and recv on the game path are non-blocking (pm_log.c:77, 91, 506). The crash send uses MSG_DONTWAIT (pm_log.c:925). The writer accepts only from its own thread (pm_log.c:646-647) and the accept is non-blocking. A dead Wi-Fi returns an error, and the client closes (pm_log.c:509-511). No game-thread wait depends on the socket.
- Flash and SD. flash_lock is released before any SD I/O (pm_flash.c:24-35), and pm_flash_mark takes only flash_lock (pm_flash.c:76-84). The game thread does not wait on SD writes.
- Time. No osGetTime. The catch-up cap at pm_main.c:517-519 limits a time jump. The pacing loop (pm_main.c:399-410) exits when the time passes sched_next. pm_dspa_pace (pm_dspaud.c:456-466) resets next_due on a big jump. pm_ticks is 32-bit (pm_main.c:158) and wraps, but only feeds the stats (pm_audio.c:207-209). No loop waits for a time that already passed.
- The ndsp and audio thread do not block the game directly except through U2 (Rank 2).

Watchdog gap (PROVEN): pm_wdog.c:59 skips the count when !aptIsActive(). In case 1a the game is inside libctru's wait, so aptIsActive() may stay false. The watchdog then never writes its "last phase" line. In case 1b the phase is only useful if aptIsActive() is true after wake (not checked).

Watchdog write path (PROVEN): pm_crash_line calls crash_flush (pm_log.c:868-900) and fsync (pm_log.c:917, 923) before the monitor writes the phase line. These fsync calls have no timeout. If the SD card is still powering up, the watchdog thread blocks before it prints "last phase". The one bounded exit (pm_exit_process, pm_wdog.c:28-37) applies only to the close step.

Diagnostic gap (PROVEN by read): the live log socket (port 17492) dies with Wi-Fi during sleep. The listen socket is not re-created on wake, so the PC cannot attach to a new live log after the wake. The wake line goes to log.txt and crash.txt (SD) only.

## 3. Logging-off impact

- Writer thread: waits on ev_wake with no timeout when file logging is off (pm_log.c:633). Accept and read run only when file logging is on (pm_log.c:646-647). A stuck wake is not affected by the log level.
- Sleep and wake hooks: run whether logging is on or off. pm_log lines are queued regardless. Only the file and socket output depends on the level.
- Audio stats (pm_au_tk, pm_audio.c:131-140 and 207-209): skipped with logging off. The join itself (U2, pm_audio.c:204) is not skipped. Rank 2 happens with logging off.
- Watchdog (pm_wdog.c:43-75): runs regardless of logging. Its line reaches SD only if log_fd is open.

## 4. Fix proposal (no edit made)

F1. Log the sleep state from the hooks, with its own marker that survives a stuck wake:
- In sleep_enter_hook set a flag pm_asleep = 1 and the tick. In wake_hook set it to 0 and log "sleep: wake at tick N, asleep M ms". Call the SD write (fsync) through pm_step_timed so the line is on SD before the game waits.
F2. Watchdog sleep monitor: when pm_asleep = 1, the watchdog logs "asleep N s, no wake" every 10 s to crash.txt. It must not exit the process. A legitimate long sleep is not a hang. Do not use aptIsActive() for this case.
F3. Reorder pm_crash_line: write and send the line before crash_flush, so the phase line is emitted even when fsync blocks. Keep the fsync after the write.
F4. Bound U2 (pm_audio.c:204) with LightEvent_WaitTimeout (the pattern at pm_audio.c:220). On timeout, log "audio: job stuck N ms", clear au_busy, and skip the join for that step. Test F4 only after F1 to F3 show which rank applies.
F5. Set PM_WD_PHASE to a new "asleep, in APT wait" value in sleep_enter_hook, so the watchdog names the state. Only after F1 to F3.
F6. If case 1a shows the wake is never delivered, the app cannot fix it alone. The next step is a log of the APT and NS state at wake. Do not propose a GPU re-init until a test names the wait (see investigate_sleep_hang.md section 5).

## 5. Test to separate the ranks (user runs on console, no Azahar)

1. Build with F1 and F2 (logging on, log level as the push command in CLAUDE.md rule 6).
2. Close the lid for 30 s. Open. Wait 30 s. Read crash.txt and log.txt from the SD card (or log.old after the hard reboot). This is the pass case.
3. Repeat with a 120 s close. Hard reboot after the black screen.
4. Read the last lines in log.old and crash.txt:
   - No "sleep: enter": the lid close did not reach the game. Look at aptMainLoop and the game-thread phase before the close.
   - "sleep: enter" and "sleep: gate closed", no "sleep: wake": case 1a. The wake never reached the app.
   - "sleep: wake" then "sleep: audio not resumed": rank 2 (audio). Confirm with the phase line.
   - "sleep: wake" then "sleep: audio resumed" then frame lines that stop: rank 3 (U1) when the phase line says "gpu begin (frame begin)" (pm_wdog.c phase names), rank 2 when it says "audio join".
   - "watchdog ... last phase N (name)" in crash.txt: the phase names the wait. "idle wait for vblank" is not a blocking wait. "gpu begin" is U1. "audio join" is U2.
5. Pass for the test: "sleep: wake" with a tick, and frame lines after it. Fail: any line in section 4 that stops early, or no wake line.
6. Interpret 30 s against 120 s. If 30 s passes and 120 s fails, the cause depends on the time asleep (state lost during sleep, rank 1b or 2 or 3). If both fail, the cause is not time-dependent, so check the short-sleep path again.
