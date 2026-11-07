# Row SLEEP: sleep and HOME hangs (core 1 SD I/O across the transition)

## User report (2026-11-06, O3DS)
Sleep still hangs sometimes. User idea: the 80 percent core 1 CPU limit; drop to 30 while asleep and on HOME. Code fact: that drop exists (pm_log.c cpu_hook, sleep_enter_hook); the oracle traced the real causes below.

## Oracle plan SLEEP
Threads (from the code):
- Core 0: the game thread (pm_main.c:733, prio 0x30) calls aptMainLoop (pm_main.c:376), so the APT hooks run on it, between frames. The step worker (pm_log.c:1015, prio 0x30, core -2 = core 0) does the bounded SD steps.
- Core 1: au_thread (pm_audio.c:113), the au2 render thread (pm_au2.c:177), the log writer and sink (pm_log.c:949 and :1142, prio 0x3F), the flash thread (pm_flash.c:86, 0x3F), the watchdog (pm_wdog.c:116, 0x3F).
- The game thread waits in aptMainLoop while asleep. Every core 1 thread keeps running during sleep.
Waits and I/O that can hang, ranked:
1. The watchdog writes to the SD card during sleep and HOME (pm_wdog.c:70-90).
   - While pm_wd_asleep is set it calls wd_note every 10 s: fopen, fwrite, fclose on crash.txt.
   - While the app is inactive (HOME) it writes "app not active" every 10 s.
   - wd_note never checks pm_io_quiet. Any sleep longer than 10 s puts an FS call into a sleeping system module. That is the PT68 freeze, and it is "sometimes" because it depends on how long the sleep lasts.
   - If the lid closes while the HOME menu is open, ONSLEEP may never reach the app. pm_wd_asleep then stays 0, and the inactive notes keep writing.
2. io_gate(1) is asynchronous (pm_log.c:979). It only sets io_drain_req and signals. The writer drains the queue and runs fb_sync_all (fsync) later, at prio 0x3F on core 1, while the hook has already returned and the system is entering sleep. The sink can be inside fb_write_locked until pm_io_quiet is set at the end of that drain.
3. The flash step can outlive its deadline. pm_step_timed(PM_STEP_FLASH, 400 ms) at pm_log.c:1046 returns 0 on timeout, but the worker goes on writing the SD card into the sleep.
4. HOME is not bounded (pm_log.c:1090). ONSUSPEND calls pm_audio_drain(3) and pm_flash_flush synchronously on the game thread, with no timeout. A busy SD card or a stuck audio job hangs the HOME transition, and a sleep taken from HOME stacks on top of it.
5. Low: APT_SetAppCpuTimeLimit called from the hook is legal (the game thread, not the APT event thread). The audio and render joins are bounded (300 and 200 ms) and run before the limit drops.
Does the order matter, and does the user's 80 to 30 change cause the hang?
- The hook waits on no core 1 thread after cpu_apply(30), so the 30% limit cannot deadlock a join as built.
- But 30 is applied before io_gate(1), so the asynchronous drain (cause 2) runs on a core 1 cut to 30%, at the lowest priority. That makes the SD write overlap the sleep for longer.
- The rule: all core 1 I/O must be finished, and confirmed finished, before cpu_apply(30) and before the hook returns. The limit change itself is fine; its place in the sequence is wrong.
Fix:
1. pm_wdog.c wd_note: when pm_io_quiet or pm_wd_asleep is set, or the app is inactive, keep the note in a static RAM ring; write it at wake or restore, after io_gate(0).
2. io_gate(1) becomes synchronous: signal, then LightEvent_WaitTimeout on a new ev_quiet (signalled by the writer after fb_sync_all and pm_io_quiet = 1), bounded at 500 ms. On timeout, log it and set pm_io_quiet anyway.
3. New order in sleep_enter_hook: audio join, render sync, flash step, wait (bounded) until the step slot is no longer busy, io_gate(1) synchronous, then cpu_apply(30).
4. ONSUSPEND uses the same bounded sequence (factor out a quiet_enter(reason) used by both hooks); ONRESTORE mirrors wake_hook. Set a pm_wd_quiet flag on suspend, so the watchdog also stays off the SD card in HOME.
Console check (read crash.txt and log.txt after a hang):
- crash.txt holds "watchdog: asleep N s" or "watchdog: app not active N s" and no later "sleep: wake at tick": cause 1 (the freeze came after a watchdog SD write).
- log.txt ends between "sleep: render sync" and "sleep: gate closed", or partway through a batch: cause 2 or 3. "sleep: flash flush timeout" means cause 3.
- No "sleep: enter" at all, with "app not active" lines: the sleep was taken from HOME (causes 4 and 1).
- After the fix: "sleep: gate closed" carries the drain time, with no watchdog lines between enter and wake.

## Gate
Code first (CLAUDE.md rule 5). Azahar: one scenario that triggers the sleep and HOME hooks if the harness can (else host-side reasoning plus log lines): the log shows the new order (audio join, render sync, flash step, step slot idle, io drain with its time, cpu 30) and no watchdog SD write between "sleep: enter" and wake. Build clean. Then the orchestrator pushes; the user sleeps the console 10 times (short and longer than 10 s, also from HOME) and reads crash.txt / log.txt per the console check above.

## Rules
Own worktree: git worktree add build3ds/wt/sleep -b sleep nds1; copy assets/us and ver/us/build; build there (PM_LANE=sleep). Files: 3ds/src/pm_wdog.c, 3ds/src/pm_log.c (hooks, io_gate, writer quiet event), 3ds/src/pm_main.c only if the hooks need it. Commit on branch sleep, no attribution lines, never commit 3ds/src/map_restore_table.c, keep Azahar open. Two tries per named cause. Delete this file in the last fix commit.
