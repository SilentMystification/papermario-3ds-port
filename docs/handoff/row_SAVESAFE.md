# Row SAVESAFE: no lost or torn save from our own code paths

## User facts
User (2026-11-09) approved the fix list for save failures in code: flush on the crash and watchdog exits, write a temp file and rename,
page writes under the same lock, check the APT hook flush order, on-screen message and retry on a failed write, no lock-free write
when the copy malloc fails.

## Start state
Worktree build3ds/wt/SAVESAFE, branch SAVESAFE from main (hash in the first commit). PM_ROW=SAVESAFE, PM_LANE=SAVESAFE.

## Code facts
- 3ds/src/pm_os.c:513-600: the 128 KB chip array flash_mem; osFlashWriteArray (memcpy, flash_sync) and osFlashSectorErase (memset,
  flash_sync) run on the logic thread without flash_lock; flash_init loads save.flash.
- 3ds/src/pm_flash.c: flash_thread waits 70 ms after the last mark (60-64), then flash_write_dirty copies the dirty span under
  flash_lock (25-30), writes save.flash in place (fopen r+b, fseek, fwrite, fclose: 39-42); malloc failure writes from flash_mem without
  the lock (38); errors only logged (43-44, first 5); pm_flash_flush is the join; APT hooks flush in pm_log.c:1181 (exit) and flush_step
  1088 (sleep/HOME); pm_main.c:819 exit; pm_warp.c:1369 return to title.
- The crash handler prints [CRASH] lines (find it: grep "\[CRASH\]"); pm_wd_hold makes the watchdog wait for a running write.

## Plan
1. Lock: osFlashWriteArray and osFlashSectorErase take flash_lock around the memcpy/memset and the dirty mark (export a pm_flash_put /
   pm_flash_erase from pm_flash.c that does both; pm_os.c calls them). The flush copy then never sees a half page.
2. Copy: allocate the copy before taking flash_lock; if malloc fails, write from flash_mem while holding flash_lock (the game waits
   at most one write); never write without the lock. A 128 KB static copy buffer is NOT allowed (memcheck margin about 306 KB): heap only.
3. Atomic file: write the whole 128 KB image to save.flash.tmp, fclose (checked), then rename over save.flash (FS rename; if rename over
   an existing file fails on the 3DS SD, remove then rename, and on boot load save.flash.tmp when save.flash is missing or short).
   Log `save.flash written (atomic) <ms>`.
4. Crash and watchdog exits: call a crash-safe flush (no malloc: write from flash_mem under a trylock; skip if the lock is held by a
   half-done page write and log it) before the exit; the watchdog exit the same.
5. APT order: trace the hook order (pm_log.c cpu_hook) against the logic quiesce (pm_logic.c pm_lg_quiesce at APT suspend/sleep/exit);
   the flush must run after the logic thread stopped at a step boundary. Fix the order if not; log `save: hook flush after quiesce`.
6. Failure feedback: a failed write retries every 2 s on the flush thread until it succeeds (log each failure at pm_log_e, not only the
   first 5) and shows a short on-screen message (the existing HUD message path) "Save not written to SD (retrying)".
7. Commit with the hw note (cold path, no static growth beyond a few bytes).

## Done criteria
1. nd_host PASS. memcheck OK (margin about 306 KB, no new static buffers).
2. The flashselftest switch (pm_os.c flash_selftest + 3ds/tools/flash_selftest.py) passes on Azahar, and the file is written through the
   temp + rename path (log line).
3. A crash test: debug switch crashtest after a save (an existing crash test switch if any; else add `savecrash` that saves then
   faults 10 ms later): save.flash afterwards holds the new save (flash_selftest.py hash).
4. Save scenarios (grep save in azt_scenarios.json, e.g. savefile_check) pass.
Affected scenarios (no others): the flash/save ones.

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no toggles, no stubs. Never commit 3ds/src/map_restore_table.c.

## At the end
Delete this row file in the commit that completes the row. SendMessage main "SAVESAFE ready <hash>".
