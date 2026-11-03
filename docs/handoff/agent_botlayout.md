# Agent handoff: BOTLAYOUT (bottom screen layout)

Branch `botlayout` (worktree `papermario-3ds-port-botlayout`, from main 7d19c5808).

## State
- Bottom screen rows (1-based, cursor and ANSI count the same way):
  - 1: map line (MAPHUD, pm_gpu_hud_map).
  - 2: fps line `<fps>/<30|60> FPS late<N>% <ms> ms`.
  - 3: memory row (pm_mem_hud row 0).
  - 4: VRAM row (pm_mem_hud row 1).
  - 5: stats string (only with the stats option; blank otherwise).
  - 6: blank (reserved).
  - 7: separator (39 dashes).
  - 8: header `Log (console: <conlevel>)` left, `(SD: <loglevel>)` right. Names from pm_lvl_name (pm_options.c).
  - 9 to 26: log window (consoleSetWindow 1,9,40,18). The log scrolls only here.
  - 27: separator.
  - 28: footer `L<cpu limit> <N3DS|O3DS> #<build>` (L<n>? when the APT call failed).
  - 29 and 30: blank.
- Removed from the HUD: the VRAM bank row `A <u>u/<f>f B ... ev <n>/s` (pm_mem_hud row 2). Its bank helper stays (used by the boot log).
- Frame time moved from the second HUD row into the fps line. The 3DS model and build moved from the fps line to the footer.
- Map change info line: `map change: <from> -> <to> (area A map M)` at info level (pm_route.c, pm_map_hud_step). The step now runs with the HUD off, so the line is written with the HUD off too.
- Menu: the menu close branch resets the HUD, footer and frame state so they draw again.

## Commits
- See `git log botlayout`. Code and the scenario row are in one commit. The map_restore_table.c file is never committed.

## Test
- Scenario `botlayout` (3ds/tools/azt_scenarios.json), catalog row in docs/handoff/test_catalog.md (section BOTLAYOUT).
- Run 1 (old HUD rows): layout rows drawn, log region empty, until never matched (no shots switch).
- Run 2 (HUD rework): 5 of 5 log checks. Picture read: HUD rows 1 to 4, blank row 6, separator 7, header 8, footer separator 27, footer row 28, blank rows 29 and 30. Layout returns after `menu: close`.
- Open: log scrolling inside rows 9 to 26 is not shown. Run 2 had 2 log lines in the region.

## Open items
- Show a scroll: a longer scene or a log burst, then one bottom shot. Needs a justification line first (3ds/logs/perf/status_BOTLAYOUT.txt).
- The scenario ends at its 90 s cap, not at the until line. The until regex (shot 205) did not match the top shot (frame 203 was logged). Check the shot frame window before the next run.
- Menu-open picture not taken.

## Commands
- Build: `PM_LANE=BOTLAYOUT sh 3ds/build.sh` (repo worktree needs ver/us/build and assets/us copied from the main folder; both are git-ignored).
- Test: `PM_ROW=BOTLAYOUT powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 botlayout -Row BOTLAYOUT -Build botlayout -Try`
- PPM shots do not open in the Read tool. Convert with a small PNG writer (zlib, struct) first.
