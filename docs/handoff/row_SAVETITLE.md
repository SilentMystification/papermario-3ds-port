# Row SAVETITLE: a save made seconds before Return to Title is lost for good (user, 2026-11-09)

## User facts
O3DS XL: save at a save block, then within seconds use the Select (debug) menu Return to Title Screen. The new save is not on the
title screen, and it is still missing after a full restart: lost for good. "Multiple times." Root cause unknown.

## Code (orchestrator)
pm_warp.c:1408 pm_title_return: pm_lg_quiesce(); pm_flash_flush("return to title"); pm_audio_drain(3); soft reset flags.
pm_flash.c:115 pm_flash_flush -> flash_write_dirty(why) (returns at once if !flash_lock_ready). Saves go through the flash thread
(pm_flash.c:159, core 1) and the SAVESAFE rules. Suspects to check by code: (a) the save is still in the game's save buffer or the
flash thread queue when the soft reset starts, and the reset (GAME_MODE_STARTUP, fio/flash init) re-reads or clears the flash image
before the write lands; (b) flash_write_dirty sees no dirty page because the game marks it later; (c) the write lands but the reset
path writes the old image back (an init that writes defaults).

## Start state
Queue item (one agent at a time). `sh 3ds/tools/wt_new.sh SAVETITLE`, PM_ROW/PM_LANE=SAVETITLE.

## Steps
1. Code trace of the save path from the save block script to the SD file write, and of Return to Title (flush, reset, flash init).
   Log lines: save request (slot, bytes, dirty pages), flush start/end with page count and the file write result, and the flash
   init after the reset (read or write). One Azahar run: scenario save at a save block (padscript), Return to Title within 2 s,
   load the file select (log the slot data checksum before and after).
2. Fix the named cause; gate: the checksum after the title equals the saved one; one run.
3. Console (user): save, Return to Title within seconds, the save is there; then restart, still there.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. After 2 failed tries: the oracle.
