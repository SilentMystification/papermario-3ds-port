# Row ESCAPE2: START+SELECT exits cleanly, with a debug menu choice of where to go

## User facts (2026-11-10, O3DS XL, main 86004672a)
- "start+select quit doesnt work as expected, it kicks us out to the homescreen and restarts the system from a fault."
  The XL log shows the escape line `escape: START+SELECT 1 s, phase 2 steps 29470 stuck 0 gx n 1 cur 1 last 1 ... f7974`, so the
  not-stuck path ran (pm_exit_process from the watchdog thread), and the system then faulted and restarted.
- "from the debug menu we should be able to configure the start+select behavior between homebrew menu, quitting and homescreen if possible."

## Facts (orchestrator)
- FTP pull from the XL after the fault: connection refused (no new dump).
- ESCAPE design (row text in git history, commit 080d34e7f): the watchdog thread (pm_wdog.c, core 1) calls pm_exit_process, which
  does the save flush, the log close and svcExitProcess while the game, audio and render threads still run.

## Oracle task
1. Name by code why the exit from the watchdog thread faults the system (for example svcExitProcess while another thread holds
   GSP/GPU or the audio DSP, gfx or ndsp not shut down, an exit from a thread other than main, or the 3dsx loader return path).
2. Design a clean exit that works from a hung or a running game: for example the watchdog sets a flag that the main loop reads to run
   the normal exit path, and only a truly stuck game (no main loop progress) uses the forced path.
3. Design the debug menu setting "START+SELECT" with 3 values, as the user asks, each with its libctru call:
   - Homebrew menu: the normal 3dsx exit back to the loader (hbmenu).
   - Quit: close the app (the same as Homebrew menu for a 3dsx, or name the difference).
   - HOME Menu: the system HOME Menu (for example aptJumpToHomeMenu or an APT reboot to HOME), and what works with a stuck GPU.
   Say what is possible for a 3dsx under hbmenu. The setting is saved with the other port settings. Place it in the Debug tab.
4. Give the Haiku rows with files and gates (Azahar for the running exit path; the console check by the user).
