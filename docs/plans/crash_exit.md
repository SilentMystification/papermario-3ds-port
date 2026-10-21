# LB4: crash exit and hang watchdog

## Problem
A crash on the console wrote the [CRASH] report to log.txt. Then the handler called svcBreak(USERBREAK_PANIC). The app stayed frozen. The user had to switch the console off.

## What the code does now
Crash (data abort, prefetch abort, undefined instruction) in the game or audio thread:
1. The handler runs on the faulting thread. It sets pm_wd_crash_tick.
2. It writes the report with pm_crash_line. That function takes no lock that the faulting thread can hold. It uses trylock on the log drain lock with a 500 ms wait, then write and fsync on log.txt and crash.txt.
3. It writes the last game loop phase and the step count.
4. pm_exit_process (src/pm_wdog.c) waits up to 1 s for a running save (pm_wd_hold), closes log.txt and calls svcExitProcess.
svcExitProcess stops all threads of the process (audio, flash, render, log writer, watchdog). The handler calls no APT, GSP or ndsp function, so it cannot block on them.

Hang watchdog (src/pm_wdog.c): one thread, core 1, priority 0x3F, sleeps 1 s. Switch `wdog=<s>`, default 10, 0 = off.
- Progress counter pm_wd_prog: the game loop adds 1 per iteration and 1 per drawn step.
- Phase marker pm_wd_phase: one store per stage (1 loop top, 2 log poll and pad, 3 idle vblank wait, 4 gpu begin, 5 game step and display list build, 6 audio step, 7 gpu end, 8 audio join, 9 warp tick, 10 hud and test switches).
- No trigger: before the first loop iteration (boot, ROM load), when aptIsActive() is false (HOME menu, sleep), and while a save runs (pm_wd_hold, set in flash_write_dirty).
- On a hang it writes `watchdog: no progress for N s, game thread last phase ...` with pm_crash_line (the log queue is flushed first, so the last log lines are in log.txt), then exits like a crash.
- The same thread ends the process if a crash handler runs for more than 3 s (handler hung or faulted again). This also works with wdog=0.
- svcGetThreadContext does not work in user mode, so the phase marker replaces the pc.

Test switches: `crashtest=1` null write (data abort), `crashtest=2` busy loop (hang), `crashtest=3` or bare `crashtest` undefined instruction. `crashframe=<n>` sets the frame (default 300).

## What is possible on hardware
- The process ends and the system takes the app away. The user sees the HOME menu or the launcher entry, not a frozen screen. This is the standard result of svcExitProcess for a title or 3dsx process.
- Not possible from the handler: a normal return from main. The handler runs on the faulting thread. The main thread waits in threadJoin. libctru exit (aptExit, gfxExit, ndsp) needs a healthy game thread and can block. HW7 shows that exit with live threads crashes. So the handler does not use that path.
- Whether the console reloads the Homebrew Launcher or shows the HOME menu depends on the loader. With svcExitProcess the process ends and APT does not run its exit steps. The HBL return is NOT proven. If the console shows the HOME menu, the user starts the launcher again.

## Risks
- A crash inside the handler (for example a bad stack): the watchdog thread ends the process after 3 s. If the watchdog thread did not start (log line "NOT STARTED"), the console can still freeze.
- A hang in the kernel or a driver call (not a game loop hang) can stop the watchdog only if the SD or the scheduler is blocked. A hung SD write can block pm_crash_line.
- A save in progress: the exit waits 1 s. A longer save is cut. save.flash can be partly written (the same risk as a power-off).
- log.txt: every crash line is fsynced. The file is closed before the exit. A cut during the fsync can leave the last line short.
- The audio DSP can keep a short sound loop after the exit until the system resets it.
- Debugger pause: the tick runs, so a pause longer than the limit triggers the watchdog. Use wdog=0 with a debugger.
- Map loads (2 to 4 s) and saves are below the 10 s default.

## Tests
- Azahar, one start: `crashtest=2 crashframe=120 wdog=3` (build 1014-laneB). log.txt has `watchdog: no progress for 3 s, game thread last phase 10 (hud and test switches), steps 120` and `watchdog: exiting the process`. The boot line `rom resident ... cap ... margin` also printed.
- Crash path (`crashtest=1`): code review and host compile only. Azahar does not call the guest exception handler on a real fault.

## Console check needed
1. `crashtest=1` on the O3DS: check that the app leaves the screen, log.txt and crash.txt have the [CRASH] report and "exiting the process", and what the console shows after (HOME menu or Homebrew Launcher).
2. `crashtest=2 wdog=5` on the console: the same check for the hang path.
3. A normal map load and a save with the default wdog=10: no false trigger.
