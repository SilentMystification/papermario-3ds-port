# Row HUD3: wide field HUD bar ends show an odd yellow bar, not the HUD frame

## User facts (bug, 2026-11-08, console, wide mode, field play)
User: "HUD isnt properly widened in gameplay either, keep the elements inside of the hud in their same relative position but the hud is not properly stretched to the left and right side. there is an odd yellow bar there instead." Earlier answers (HUD2): the status bar background spans the full 400 px; the items keep their 4:3 art size. Reproducing scenario: yard_wide_hud (HUD2 gate; frame span x 0..399) or any field map in wide mode with the status bar shown.

## Goal
The status bar (HP/FP/coins, boxes WINDOW_STYLE_5 and _6, src/inventory.c:745-750) looks like the 4:3 bar stretched to the screen edges: the frame/border art and the fill continue to x 0 and x 399, with the proper end caps at the screen edges. The items inside keep their position relative to the bar as now. No yellow bar.
Trace by code first: HUD2 (merge 1773bba99) made box 5 start at x 0 and box 6 end at x 320 in game space, and "the wide edge rule extends the edge vertices to the screen edges". Name which draw makes the yellow bar: an end-cap or corner slice of the 9-slice draw_box stretched by the edge rule (a 1-texel column smeared over 40 px), the box 5/6 join, or another quad. Name the vertex x and uv of the slices at both ends in wide mode. The fix likely puts the end caps at the screen edge (x -40 and x 360 in game space, i.e. 0 and 399 on screen) and stretches only the middle slice, instead of extending edge vertices.

## Start state
Worktree build3ds/wt/hud3, branch hud3 from nds1 (current). Copy assets/us and ver/us/build into it.

## Touch only
src/inventory.c (status bar boxes under pm_wide), the draw_box / window path or the wide edge rule (pm_wide*.c, nd_rect.c) that the trace names.

## Done criteria
1. Code trace in the handback: the slice that makes the yellow bar, its x and uv.
2. Build; nd_host PASS.
3. Picture gate: yard_wide_hud shot shows the bar frame continuous to x 0 and x 399 with end caps at the screen edges, no yellow bar, items where they were.
4. Non-wide: one shot of the same scene with wide off is byte-equal to the base. The 4 byte gates (spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015) byte-equal unless the trace shows they show the status bar in wide (then name the pixels).
Affected scenarios (no others): yard_wide_hud, its non-wide twin, the 4 byte gates.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=hud3 PM_ROW=hud3 on every build.sh and bench call. No git stash. No toggles, no stubs. Do not edit src/msg.c. 2 tries per named cause.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=hud3 PM_LANE=hud3 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=hud3 PM_LANE=hud3 python 3ds/tools/bench.py run <bench> --row hud3 --lane hud3 --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "hud3 ready <hash>".
