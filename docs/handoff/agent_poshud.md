# Agent handoff: POSHUD (Mario position row), stats row and the map change block

Branch `poshud` (worktree `papermario-3ds-port-poshud`). Rework 2 merged from main 77a63e603.

## State
- Done: fixed HUD rows, always visible. The map block never replaces the fps, memory or VRAM rows.
- Done: Mario row (poshud option, Debug tab row 8, or the poshud switch), drawn every frame. Camera row removed.
- Done: stats row (stats option) drawn after the Mario row.
- Done: map block on HUD rows 4 to 6: during the 2 s change it shows old and new map. Otherwise the current map.
- Tests: mapchange_poshud (poshud on, stats off), mapchange_allrows (both on), mapchange_hud (both off). See the POSHUD2 section of docs/handoff/test_catalog.md.

## Row table (1-based, bottom screen)
| Row | Item | When |
|---|---|---|
| 1 | fps, ms, late% | always |
| 2 | memory | always |
| 3 | VRAM | always |
| 4 | map block line 1: old id (change) or current id (idle) | always |
| 5 | map block line 2: `old area -> new id` (change) or current area and map (idle) | always |
| 6 | map block line 3: new area and map (change) or blank (idle) | always |
| 7 | Mario `Mario x <x> y <y> z <z> yaw <a>` (whole numbers) | poshud on |
| 7 or 8 | stats row (`t`, `i`, `c`, `d`, `o`, `k`, `x`, `sp`, `sl`, `gr`) | stats on |
| 7 + poshud + stats | separator (39 dashes) | always |
| separator + 1 | log header (console and SD level) | always |
| separator + 2 to 26 | log window (scrolls) | always |
| 27 | separator | always |
| 28 | map (`Map: <id> (area A, map M)`) | always |
| 29 | apt, model, build | always |
| 30 | blank | always |

Separator row: 7 with neither option, 8 with poshud, 9 with both (the log window loses one row per option: 18, 17, 16 rows).

## Code
- pm_route.c: `pm_pos_text(char*)` (Mario only, integer printf); `pm_map_hud_step(step, hold, rows)` always fills the 3 map rows, returns 1 during the hold.
- pm_gpu.c: `pm_gpu_hud(line0, stats, mem, pos, map)`. `stats` and `pos` are NULL when off. `pos_on`, `stats_on` set LOG_TOP (9 + extra) and LOG_ROWS (18 - extra). `frame_lay` (LAY_KEY) redraws the layout on change and resets the log cursor to the window top.
- pm_main.c: the Mario text is built each frame (not in the hudint block). The memory rows keep hud_ms.
- CAP1 log line prints `pos f<frame> Mario x ...` (scenario mac01t_cap1 regex updated).

## Open items
- Turning an option off leaves old text in the rows that become the log window until the log scrolls.
- No row is dropped: the log window gives its rows (18 to 16).
- With the HUD off (dbg_hud), no HUD rows draw, including the Mario and stats rows.
- toad_pond run exit 2 (marker check read a log.txt the run did not write, `-CheckLog` 3 of 3). Two starts ended early while another azahar.exe ran.
- Azahar budget for POSHUD used: 8 of 8.
- Not pushed.

## Commands
- Build: `PM_LANE=POSHUD sh 3ds/build.sh` (worktree needs assets/us and ver/us/build from the main folder, git-ignored).
- Test: `PM_ROW=POSHUD powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <mapchange_poshud|mapchange_allrows|mapchange_hud|toad_pond|botlayout> -Row POSHUD -Build POSHUD -Try`
