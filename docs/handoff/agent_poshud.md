# Agent handoff: POSHUD (Mario position row) and the map change block

Branch `poshud` (worktree `papermario-3ds-port-poshud`, from main 99266ab2d).

## State
- Done: Mario row 5 on the bottom screen (poshud option, Debug tab row 8, or the poshud switch), drawn every frame. Camera row removed.
- Done: map change block on HUD rows 1 to 3 for the 2 s hold (MAPHUD). It replaces the fps, memory and VRAM rows during the hold. The log window does not move.
- Tests: toad_pond, mapchange_poshud, botlayout (see the POSHUD2 section of docs/handoff/test_catalog.md).

## Layout
- Position row off: as BOTLAYOUT. Rows 1 to 6 HUD, 7 separator, 8 header, 9 to 26 log, 27 separator, 28 to 30 footer.
- Position row on: 1 fps, 2 memory, 3 VRAM, 4 blank, 5 `Mario x <x> y <y> z <z> yaw <a>` (whole numbers, rounded), 6 stats, 7 and 8 blank, 9 separator, 10 header, 11 to 26 log (16 rows), 27 separator, 28 to 30 footer.
- Map block (2 s): row 1 old id; row 2 old area and map, `->`, new id; row 3 new area and map (right column at col 22).

## Code
- pm_route.c: `pm_pos_text(char*)` (Mario only, integer printf); `pm_map_hud_step(step, hold, rows)` returns 1 and fills the block.
- pm_main.c: the position text is built each frame in the HUD block (not in the hudint block). The memory rows keep hud_ms.
- pm_gpu.c: `pos_on` sets LOG_TOP (9 or 11) and LOG_ROWS (18 or 16). `frame_pos` redraws the layout on change and resets the log cursor to the window top.
- CAP1 log line now prints `pos f<frame> Mario x ...` (scenario mac01t_cap1 regex updated).

## Open items
- Option off when the HUD is off: no position row (the HUD block only runs with dbg_hud on).
- Layout change on the console clears no text: the rows that become the log window after the position row is turned off keep old separator or header text until the log scrolls.
- Row 29 of the botlayout compare was not read. It differs in the build tag.
- Shared Azahar: two starts ended after 6 s while another azahar.exe ran. The lock in run_azahar.ps1 did not stop that run.

## Commands
- Build: `PM_LANE=POSHUD sh 3ds/build.sh` (worktree needs assets/us and ver/us/build from the main folder, git-ignored).
- Test: `PM_ROW=POSHUD powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <toad_pond|mapchange_poshud|botlayout> -Row POSHUD -Build POSHUD -Try`
