# Row SAVEFIX: remove the warp save block (every save is written); options.ini survives a cut write

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## User facts (2026-11-11)
"saving is not working consistently." Flow: "game -> save -> select -> return to title -> load save file we just saved -> that save
doesn't load." Decision: "I don't want to gate saving on whether we've warped; remove that."
## Cause (oracle code read, main 141c5e371)
- The save block writes the slot into the RAM image synchronously (src/fio.c fio_save_game :279-324 -> fio_write_flash ->
  pm_flash_put, pm_flash.c:234); pm_title_return flushes it (pm_warp.c:1412); the file select reads the RAM image
  (fio_fetch_saved_file_info :165); the soft reset does not reload or clear it (flash_init runs once, pm_os.c:597-598).
- The only path that drops the save: src/fio.c:282-285 returns when pm_save_blocked(); pm_warp.c:832 sets the block for any warp with
  no save slot (boot warp :811, file select or title warp :1011, boss launch outside the world :1238); nothing clears it.
- options.ini: pm_options.c:112-113 unlinks options.ini before the rename, and pm_options_init (:119-121) never reads options.tmp.
## Every reader and writer of the save block (all removed)
pm_warp.c:384-386 comment and `static const char* w_why`; :387-388 `save_block`, `save_why`; :392-396 pm_save_block; :397
pm_save_blocked; :398 pm_save_note; :811, :1011, :1238 `w_why = ...`; :832 the pm_save_block call; :1130-1134 pm_save_prompt reads
save_block (keep the function, change its body; its caller src/entity/SaveBlock.c:86 stays).
src/fio.c:31-33 the two declarations; :281-286 the early return.
## Start
`sh 3ds/tools/wt_new.sh SAVEFIX` (build3ds/wt/SAVEFIX, branch SAVEFIX from main). PM_ROW=SAVEFIX PM_LANE=SAVEFIX on every call.
No running row edits pm_warp.c, src/fio.c or pm_options.c.
## Exact edits
1. pm_warp.c: delete the two comment lines above `static const char* w_why` (:384-385), the line :386, :387 `static int save_block;`,
   :388 `static const char* save_why = "";`, the functions pm_save_block, pm_save_blocked, pm_save_note (:392-398), the three
   `w_why = "...";` lines (:811, :1011, :1238) and the line :832 (`if (w_loadslot < 0) pm_save_block(w_why); ...`). Line :833 and
   below stay. pm_save_prompt body becomes `pm_log("save prompt: saving enabled");` (comment: "SAVEFIX: saving is never blocked").
2. src/fio.c: delete `/* WARPFIX2 ... */`, `int pm_save_blocked(void);` and `void pm_save_note(s32 slot);` (:31-33; keep the
   `#ifdef TARGET_3DS` and pm_cheat_file_erase). In fio_save_game delete the block `#ifdef TARGET_3DS` / `if (pm_save_blocked()) {` /
   `pm_save_note(saveSlot);` / `return;` / `}` / `#endif` (:281-286).
3. pm_options.c pm_options_write: replace `unlink("options.ini");` and the rename line with
   `if (rename("options.tmp", "options.ini") != 0) { unlink("options.ini"); if (rename("options.tmp", "options.ini") != 0) pm_log_e("options: rename to options.ini failed"); } /* SAVEFIX: the old file stays until the new one is complete */`
4. pm_options_init: `FILE* f = fopen("options.ini", "r");` becomes
   `FILE* f = fopen("options.ini", "r"); if (!f && (f = fopen("options.tmp", "r")) != NULL) pm_log_w("options: options.ini missing, read options.tmp (a cut write)"); /* SAVEFIX */`
5. `git grep -n "pm_save_block\|pm_save_blocked\|pm_save_note\|save_block\|w_why"`: no hit in 3ds/src, src, include. Paste it in the commit.
## Debug switches (no new guard added; the user decides)
- freshsave (pm_os.c:601): a boot with it starts erased and the first save overwrites save.flash. Only from debug3ds.txt or a launch
  word; a debug3ds.txt left on the SD card with it erases the real saves.
- warp= with no loadslot, and the Select-menu warp from the title: a new-file game state can now be saved into a slot and replace a
  real file there (the block prevented that).
## Gate
- memcheck OK; `python 3ds/tools/bench.py run nd_host --row SAVEFIX --why "SAVEFIX"`: PASS.
- Byte compare: spot_mac01_det against the stored base b8bc06613: run, then `python 3ds/tools/bench.py cmp spot_mac01_det --row SAVEFIX
  --commit b8bc06613`: 6/6 (one Azahar start).
- Console (user): save at a save block, Select menu Return to Title, load the file: the new save; reboot: still there.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
