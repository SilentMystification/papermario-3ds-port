# Row HUD2: field status bar background edge to edge in wide mode

## User facts (2026-11-08)
In wide mode (400 px top screen) the field status bar (HP/FP/coins/star points) is not wide enough. Target: the bar background spans the full width (screen x 0..399); the items keep their art size with no stretch, at their 4:3 places. Not in scope: battle HUD, popups, debug HUD. "Spread the items across the bar" is a later step only if the user asks after the shot.

## Start state
Worktree build3ds/wt/hud2, branch hud2 from nds1 (head after this row). Copy assets/us and ver/us/build into it. Agents AUDIT (pm_tex.c, nd_rect.c, nd_geom.c, pm_gpu.c) and CORE2 (pm_audio.c, pm_log.c, pm_phase.c, pm_main.c) run in other worktrees: edit only src/inventory.c (and draw_box.c / pm_gpu.c only if the premise check fails; tell the orchestrator first for pm_gpu.c).

## Oracle trace (nds1 1fdb3e73e)
- The bar is two draw_box windows (src/inventory.c): WINDOW_STYLE_5 (174x35) at x = drawPosX, WINDOW_STYLE_6 (122x25) at drawPosX + 174 (inventory.c ~769-770). 4:3 span x 12..308. draw_box (src/draw_box.c:338) is a 9-slice: corners and edges keep their art, only the body grows.
- Items are hud elements from drawPosX: HP 22/37/48, FP 110/125/136, star points 195/200, coins 244/247 (inventory.c ~793-913), digit and slash loops (~499-573), star power orbs and shimmer.
- 1.25 today: sb_off(n) = n*5>>2 in wide (inventory.c:9); sb_set_script and the init block call hud_element_set_scale(1.25f); drawPosX = pm_wide ? -25 : 12 (inventory.c ~343).
- Wide UI projection: ui_ortho 1:1 with +40 px (pm_gpu.c:211-212): game x -40..360 = screen 0..400; scissor opens to 0..400 (pm_gpu.c:947-953). draw_box and hud_element clip against pm_wide_left(0)/pm_wide_right(320) (draw_box.c:422-426, hud_element.c:449-458).

## Premise and its check (read only, first)
Premise: a draw_box quad at game x -40..360 under ui_ortho draws 1:1 and is not scaled by the WIDE1-2 edge rule (wedge_x / wide_edge, pm_gpu.c ~211, ~2587-2650, ~2854-2867: scales an ortho draw whose vertices sit on or past the game edge, |x| >= 0.999 w). Read draw_box's emit path (draw_box.c after ~430) and wide_edge's test. If the rule catches the box: in wide, draw the box ends exactly on the game edge (x 0 and 320) and let the rule extend only the edge vertices to the screen edge (F07-7, pm_gpu.c ~3789); confirm in one shot that the corner art is not scaled.

## Change (inventory.c, no toggle; nowide stays a debug switch in pm_gpu.c ~1432)
1. Delete sb_off, sb_set_script, the hud_element_set_scale(1.25f) block and the 1.25 comments: items at 4:3 offsets and art size.
2. drawPosX = 12 in both modes.
3. Box 5: x = pm_wide_left(0) (-40 in wide), width 174 + (12 - pm_wide_left(0)) (226 wide, 174 4:3), height 35.
4. Box 6: x = drawPosX + 174, width 122 + (pm_wide_right(320) - 308) (174 wide, 122 4:3). In 4:3 the expressions give exactly 12, 174, 122.

## Gate
- yard_wide_hud (uishow=bar), shots ~272-300: the beige frame spans screen x 0..399 (pixel scan of the frame colour on the bar rows); item art at 4:3 pixel size (HP heart width equals the yard_hud shot), bar height 35/25 (not 44/31).
- yard_hud with nowide: byte-equal to nds1.
- battle_wide: compare; the status bar also shows in battle. If it changes, report it (bar edge to edge there too), do not gate against it.
- The 5 det byte gates unchanged.
Shots are read by eye for the subject (CLAUDE.md rule 5 PASS).

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. PM_ROW=HUD2 PM_LANE=hud2 on every build.sh and bench call. Budget 8 starts. No git stash. Never commit runs.jsonl or 3ds/src/map_restore_table.c. One commit; it deletes this row file. 2 tries per named cause.
