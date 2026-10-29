# Diagnosis: hang after lid sleep longer than a few seconds (O3DS)

Read-only. No source edits. Code read at HEAD (main), commit d35248f0d for the log gate.

## Hook and main loop
- Only APT hook: pm_log.c:700 `aptHook(&cpu_hook)`. No aptSetSleepAllowed, no aptIsSleepAllowed, no ndsp re-init on wake (pm_audio.c:38, 47 only at init and exit).
- cpu_hook pm_log.c:663-666. ONSLEEP runs on the game thread (aptMainLoop context).
- ONSLEEP calls, in order: pm_audio_drain (pm_audio.c:212), pm_flash_flush (pm_flash.c:22-52), cpu_apply, io_gate(1). None has a timeout.
- Main loop pm_main.c:357 `while (aptMainLoop() ...)`. Waits with no timeout: pm_main.c:397 gspWaitForVBlank, C3D_FrameBegin (pm_gpu.c:1688).

## Blocking calls that can stall across sleep (file:line)
1. pm_audio.c:201 `LightEvent_Wait(&au_done)` in pm_audio_join_at, no timeout. Called from the sleep hook via pm_audio_drain.
2. pm_au2.c:86 `LightEvent_Wait(&ev_idle)` in wait_jobs (called by pm_au2_sync, pm_audio.c:213). No timeout.
3. pm_flash.c:24 `LightLock_Lock(&flash_wr_lock)` plus SD fwrite/fclose (pm_flash.c:40-52). Runs in the sleep hook.
4. pm_log.c:420 fsync and pm_log.c:776 fsync in pm_log_crash_close, with no timeout. Called by the watchdog exit path (pm_wdog.c:33-45, pm_exit_process).
5. pm_log.c:465 send, pm_log.c:77 send: socket is O_NONBLOCK (pm_log.c:88, 235). Not a block. A dropped socket gives EAGAIN or close only.

## Candidate causes (ranked)
1. Sleep hook waits on audio or flush with no timeout (pm_log.c:664, pm_audio.c:201, pm_au2.c:86, pm_flash.c:24).
   Evidence: the game thread runs the hook. If the audio job or an SD write does not finish, the hook never returns and the app never acks sleep. Wake then never reaches the game thread.
   Test (console): add one log line before and after pm_audio_drain(3) in cpu_hook. Sleep 1 s, then 30 s. Pass if the "after" line is missing for the 30 s run.
2. Audio DSP not re-initialised after sleep (no ndspInit on ONWAKEUP, pm_log.c:666). Wave buffers stay QUEUED or PLAYING, so pm_dspa_voice_ended (pm_dspaud.c:222-229) never returns 1 and au_done is never signalled after wake.
   Evidence: pm_audio.c:201 has no timeout. Only the 30 s sleep reaches a long DSP off period.
   Test (console): log ndsp wave buffer status of channel 0 once per second after wake. Sleep 30 s. Pass if status stays not DONE and the next pm_audio_kick join blocks (au_w_max grows).
3. Watchdog exit path blocks, so the process never ends (pm_wdog.c:53 ignores aptIsActive during sleep, then pm_wdog.c:41 counts 10 s after wake; pm_exit_process calls pm_log_crash_close with fsync at pm_log.c:776 and pm_crash_line fsync at pm_log.c:789). If the SD card or FS is still asleep, fsync blocks.
   Evidence: pm_log_crash_close has no timeout. crash_flush (pm_log.c:757-760) has a timeout, pm_exit_process does not call it.
   Test (console): run with wdog=5. Sleep 30 s, wake. Pass if the log shows "watchdog: no progress" and then "watchdog: exiting the process" within 10 s after wake. Missing lines mean the watchdog thread is blocked.

Lower rank: gspWaitForVBlank and C3D_FrameBegin wait on the GSP event with no timeout (pm_main.c:397, pm_gpu.c:1688). Only if GPU state is lost on wake. No test planned.

## Log on versus off
- Logging on: the writer thread (pm_log.c:580, 597) and the sink (pm_log.c:520, 10 Hz) both touch SD and socket. The hook sets io_gate(1), which only signals the writer (pm_log.c:644-651). The hook does not wait for the drain.
- Logging off (d35248f0d): the writer waits on ev_wake with no timeout (pm_log.c:581) and no fsync or send. Only pm_audio.c:201 and pm_flash.c remain in the hook path. So if cause 1 or 2 is real, log off must still hang. Test: run one 30 s sleep with log=off. Hang with log off means the cause is in audio or flash, not the log.

## Next step
Console test for cause 1 first (one log line before and after drain in cpu_hook, sleep 1 s and 30 s). Run cause 2 and 3 tests only if cause 1 passes.
