# Agent handoff: BOTLAYOUT (bottom screen layout)

Branch `botlayout` (worktree `papermario-3ds-port-botlayout`, from main 7d19c5808).

## Layout (bottom screen, 1-based rows; cursor and ANSI count the same)
- 1: fps line `<fps>/<30|60> FPS late<N>% <ms> ms`. While a map change holds (2 s), this row shows `Now in <id> (area A, map M)`, then the fps line again.
- 2: memory row (pm_mem_hud row 0).
- 3: VRAM row (pm_mem_hud row 1).
- 4: stats string (pm_gbi_stats, only with the stats option; blank otherwise).
- 5, 6: blank (reserved).
- 7: separator (39 dashes).
- 8: header `Log (console: <conlevel>)` left, `(SD: <loglevel>)` right (names from pm_lvl_name).
- 9 to 26: log window (consoleSetWindow 1,9,40,18). Only the log scrolls here.
- 27: separator.
- 28: map row `Map: <id> (area A, map M)` (pm_map_now).
- 29: system row `apt: <N>%` (`apt: <N>%?` when the last APT call failed), `N3DS` or `O3DS`, `#<build>`. Cut to 39 columns.
- 30: blank.

## Removed or moved data
- Removed: VRAM bank row `A <u>u/<f>f B ... ev <n>/s` (pm_mem_hud row 2). The bank helper stays (boot log). The eviction rate is no longer on the HUD. The `vram acct` debug line still prints the bank split at map loads.
- Frame time: moved from its own row into the fps line.
- Model and build: moved from the fps line to footer row 29.
- Map line: moved from HUD row 1 to footer row 28. The 2 s transition text stays on HUD row 1 (top) during its hold.
- Stats row 4 (stats option only) prints the per-second counters from pm_gbi_end_frame: `t` GPU commands, `i` input triangles, `c` cut, `d` dropped, `o` output, `k` kept, `x` sample x, `sp` splits, `sl` GPU-off count, `gr` grabs. Behind the stats option (`pm_opt.dbg_stats`, switch `stats`, or `fast`).

## Logging
- Map change info line: `map change: <from> -> <to> (area A map M)` at info level (pm_route.c pm_map_hud_step). The step runs with the HUD off too.

## Commits
- 8937a1dc4: first layout (HUD 1-6 with the map line on row 1; footer rows 28-30 before the rework).
- Rework (map to footer, apt row, removed VRAM bank row): see `git log botlayout`.

## Tests (scenarios in 3ds/tools/azt_scenarios.json, catalog in docs/handoff/test_catalog.md, BOTLAYOUT section)
- botlayout (scroll): padscript=wait420, shots 200 to 205, until `padscript: done`.
- botlayout_menu: padscript opens (SELECT) and closes the menu, shots 84 to 129, until `padscript: done`.
- Run 1 (old rows): layout drawn, log region empty after menu close.
- Run 2 (HUD rework, before the footer move): 5 of 5 log checks, picture read; the layout returned after menu close. The run ran to the 90 s cap (the until line `shot 205` never matched).

## Open items and rules
- Scroll picture and menu-open picture: pending the runs above.
- Another Azahar run shares `%APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt` and the shots folder. Wait for `azahar.exe` to exit before a start (run 3 started with another run up; its log and shots were overwritten).
- PPM shots do not open in the Read tool. Convert with a small PNG writer first.

## Commands
- Build: `PM_LANE=BOTLAYOUT sh 3ds/build.sh`. The worktree needs ver/us/build and assets/us copied from the main folder (git-ignored).
- Test: `PM_ROW=BOTLAYOUT powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <botlayout|botlayout_menu> -Row BOTLAYOUT -Build botlayout -Try`
