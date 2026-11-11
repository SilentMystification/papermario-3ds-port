# Row SAVEFIX: a save is not written after a debug warp for the rest of the session; options.ini can be lost on a cut write

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## User fact (2026-11-11)
"saving is not working consistently." (No more detail yet: see the questions at the end.)
## Causes (oracle code read, 141c5e371)
- P1 (proven path): pm_warp.c:392-397 `pm_save_block` sets `save_block = 1`; nothing sets it back to 0. It is set at :832 by any warp
  with no save slot (w_loadslot < 0): a boot warp (:811), a Select-menu warp from the file select or title (:1011), a boss launch
  outside the world (:1238). src/fio.c:282-285 fio_save_game then returns at once (only a log warning, pm_warp.c:398). So after one
  such warp, every later save of the session is dropped, even after Return to Title and loading a real file (pm_title_return
  :1408-1418 does not clear it).
- P2: options.ini (pm_options.c pm_options_write :112-113) unlinks options.ini, then renames options.tmp. A cut between the two (power,
  a crash) leaves no options.ini; pm_options_init (:119-121) reads options.ini or options.txt, not options.tmp: defaults next boot.
- P3 (by design, kept): the START+SELECT escape of a hung game calls APT_HardwareResetAsync (pm_wdog.c wd_escape) with no
  pm_flash_crash_flush: a save made within 70 ms before the hang is lost. The normal exits flush (pm_main.c:925, pm_log.c:1224,
  pm_title_return :1412). Noted, not changed here.
- Checked, no fault: save.flash write (pm_flash.c:41-56 tmp, fclose check, rename, retry every 2 s, footer warning), load
  (pm_os.c:594-631, a complete tmp is newer), the flash thread waits while asleep (pm_flash.c:146), the path is the working folder
  sdmc:/3ds/PaperMario for both hbmenu and HOME launches (pm_main.c:945-946).
## Start
`sh 3ds/tools/wt_new.sh SAVEFIX` (build3ds/wt/SAVEFIX, branch SAVEFIX from main). PM_ROW=SAVEFIX PM_LANE=SAVEFIX on every call.
No running row edits pm_warp.c, src/fio.c or pm_options.c (ESCAPE2 is merged).
## Touch only
3ds/src/pm_warp.c (after pm_save_note :398, one new function), src/fio.c (fio_load_game, the non-PAL success branch), 3ds/src/pm_options.c
(pm_options_write :112-113 and pm_options_init :119-121).
## Exact edits
1. pm_warp.c after pm_save_note: `void pm_save_unblock(void) { if (save_block) pm_log("save: block cleared (a save file was loaded)"); save_block = 0; } /* SAVEFIX */`
2. src/fio.c: next to the existing `int pm_save_blocked(void);` declaration (:32) add `void pm_save_unblock(void);` (same #ifdef as :32).
   In fio_load_game, non-PAL branch, before `return true;` after `fio_deserialize_state();` add
   `#ifdef TARGET_3DS` / `    pm_save_unblock(); /* SAVEFIX: a real save file is loaded: its saves are written */` / `#endif`.
   Do the same in the PAL branch before its `return true;`.
3. pm_options.c pm_options_write: replace `unlink("options.ini");` and the rename line with
   `if (rename("options.tmp", "options.ini") != 0) { unlink("options.ini"); if (rename("options.tmp", "options.ini") != 0) pm_log_e("options: rename to options.ini failed"); } /* SAVEFIX: the old file stays until the new one is complete */`
4. pm_options_init: `FILE* f = fopen("options.ini", "r");` becomes
   `FILE* f = fopen("options.ini", "r"); if (!f && (f = fopen("options.tmp", "r")) != NULL) pm_log_w("options: options.ini missing, read options.tmp (a cut write)"); /* SAVEFIX */`
## Gate
- memcheck OK; `python 3ds/tools/bench.py run nd_host --row SAVEFIX --why "SAVEFIX"`: PASS.
- Byte compare: spot_mac01_det (base stored at b8bc06613): base, run, cmp 6/6.
- Log gate: a scenario with a boot warp (warp=random) then title return and loadslot is not in the catalog: the console check (user)
  decides: Select-menu warp, Return to Title, load the file, save at a save block, reboot, load: the save is there.
## Questions for the user (the orchestrator asks)
Which console and launch (hbmenu or HOME Menu)? Was a Select-menu warp, a boss launch or a debug3ds.txt switch (warp=, freshsave)
used in that session? Which save was lost (a save block, or the options)? Is sdmc:/3ds/PaperMario/debug3ds.txt on the SD card?
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
