# Row STORYMENU: Story progress debug folder, and story order for it and Warp area

## User facts
User (2026-11-09): "We should have a debug menu for setting the story progression point. It should be shaped similarly to the warp
menu. Also for both of these menus make it ordered by story and then have extra / debug options."

## Goal
1. A new Debug folder "Story progress" with the same 2-level shape as Warp area: level 1 = the chapters in story order (Prologue,
   Chapter 1..8, Epilogue), then an "Extras" group; level 2 = the story points of that chapter in story order; A sets that point.
2. Warp area level 1 in story order (areas in the order the story reaches them), then the extras/debug rows (presets, Minigames,
   Game Over, Test map, Ending) at the end. Level 2 map rows keep their order.

## Start state
Worktree build3ds/wt/STORYMENU, branch STORYMENU from main (hash in the first commit). PM_ROW=STORYMENU, PM_LANE=STORYMENU.

## Code facts
- Debug tab rows: pm_options.c tab_ids (about 384): RW_WARPAREA first. Folder logic: pm_options.c 428-480 (warp_warn, sub_area,
  pm_optui_sub_pop, level 1/level 2, the row memory), labels about 475-553. Area list: pm_warp.c wa_build (about 579) over gAreas, names
  area_name[] (pm_warp.c:570). Presets: pm_warp.c:38-45. Story names: pm_warp.c:50 on (name, STORY_* value; include/enums.h has 226
  STORY_ values). Story set path: the `story=` switch (w_story_set, pm_warp.c:106).
- Cheats tab has a "Story" row (RW_CH8 + 4, pm_options.c:530). The new folder replaces it: delete that row (no duplicate controls).

## Plan
1. Story order of areas (check against the STORY_ enum and the chapter first visits; adjust where the code says otherwise):
   Goomba Village, Shooting Star Summit, Toad Town, Outside Peach's Castle, Peach's Castle, Koopa Village, Koopa Bros. Fortress,
   Mt. Rugged, Dry Dry Outpost, Dry Dry Desert, Dry Dry Ruins, Forever Forest, Boo's Mansion, Gusty Gulch, Tubba Blubba's Castle,
   Shy Guy's Toy Box, Inside the Whale, Jade Jungle, Mt. Lavalava, Flower Fields, Shiver City, Crystal Palace, Bowser's Castle,
   Toad Town Tunnels; then Extras: presets, Ending, Minigames, Game Over, Test map. One static order table (area index list), wa_build
   uses it. The row memory keeps working by area index, not row.
2. Story progress folder: one static table {chapter, STORY_* value, short label} built from include/enums.h (all story values, grouped
   by chapter by their enum range; label from the enum name, readable: "CH1 Kooper joined party"). Level 1 = 10 chapter rows + Extras
   (current value display, +1 / -1 step rows). Level 2 = the points; the current point is marked; A sets the story through the same code
   path as story= and logs `story: set <value> <name>`. No warning page unless setting story while in a battle is unsafe (then refuse in
   battle and say so).
3. Same shape and controls as Warp area (pm_optui_sub_pop levels, B back, row memory in RAM).
4. Delete the Cheats "Story" row (RW_CH8 + 4) and its code.

## Done criteria
1. nd_host PASS. memcheck OK (tables are const: .rodata, not .bss).
2. One Azahar run (warp=random, menu by padscript): shots of Warp area level 1 (story order, Extras last), Story progress level 1, and
   one chapter page with the current point marked; the log line `story: set` after A. Shots must show the menu text.
Affected scenarios (no others): the new menu scenario.

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash. No toggles: the old order and the Cheats Story row are deleted. Commit after each passed step.
Never commit 3ds/src/map_restore_table.c.

## At the end
Delete this row file in the commit that completes the row. SendMessage main "STORYMENU ready <hash>".
