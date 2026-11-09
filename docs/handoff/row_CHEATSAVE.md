# Row CHEATSAVE: cheat selections saved per save file, outside the save file

## User facts
User (2026-11-09): "The cheats menu should have a save option so cheats persist. Somewhere there should be a press X to save whenever
the cheat selection is dirty. It should be a cyan blue, and appear on the bottom of the screen. Then the selected cheats should persist
through a power cycle. Selected cheats should be tied to a specific save file. Though, this should not be saved in the save file."

## Start state
Worktree build3ds/wt/CHEATSAVE, branch CHEATSAVE from main AFTER MENUTEX merges (MENUTEX edits pm_options.c). PM_ROW=CHEATSAVE,
PM_LANE=CHEATSAVE.

## Scope
"Selected cheats" = the persistent cheat switches of the Cheats tab (Moon jump, Speed x2, Infinite HP/FP/star power, No battles,
Perfect action commands, and any other on/off cheat rows). One-shot actions (give items, set stats, equipment, partner levels, spawn
item, save/load position, instant win) change the game state, which the game save already holds: they are not part of this file.
List the exact set in the commit.

## Plan
1. Storage: one file per save slot in the app folder (sdmc:/3ds/PaperMario/cheats_slot<N>.ini, N = gGameStatus saveSlot 0..3),
   key=value lines like options.ini (reuse its reader/writer). Never inside save.flash. Write with the SAVESAFE pattern (temp + rename).
2. Dirty state: any change of a persistent cheat row sets dirty (compare with the loaded values; changing back clears it).
3. Footer: while the Cheats tab is open and dirty, the bottom screen bottom row shows "Press X to save cheats" in cyan (the menu
   text colour path; pick the cyan the menu palette has, or add one RGB constant). X saves, logs `cheats: saved slot N (<k> on)`,
   clears dirty. Leaving the menu with dirty values keeps them for this session only (no auto save).
4. Load: when a save file is loaded (file select load, loadslot= switch, return to the file) apply that slot's cheats file; a new file
   or a slot without a file starts with all persistent cheats off. Log `cheats: loaded slot N (<k> on)`.
5. Boot switches that set cheats (e.g. acmd) win over the file for that run and do not mark dirty.

## Done criteria
1. nd_host PASS. memcheck OK (no new static buffers over a few hundred bytes).
2. Azahar: load slot 0, open Cheats, turn Moon jump on: shot of the cyan footer; X: log `cheats: saved slot 0`; restart the emulator
   (power cycle), load slot 0: log `cheats: loaded slot 0 (1 on)` and Moon jump shows On; load slot 1: Moon jump Off.
3. save.flash unchanged by the cheats save (hash before/after).
Affected scenarios (no others): the new cheatsave scenario(s).

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no toggles, no stubs. Never commit 3ds/src/map_restore_table.c. Padscripts in the azslots sdmc.

## At the end
Delete this row file in the commit that completes the row. SendMessage main "CHEATSAVE ready <hash>".
