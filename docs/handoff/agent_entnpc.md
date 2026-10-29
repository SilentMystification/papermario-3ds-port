# Handoff: ENTNPC epic, agent_entnpc

## State
- Rows run in order: EN2, NP1, T1B. Done: EN2 (committed), NP1 (dropped), T1B (committed).
- Working tree: HEAD (b7e847d8c) plus the user file 3ds/src/map_restore_table.c (modified, never committed or discarded).
- Status lines: 3ds/logs/perf/status_ENTNPC.txt.

## Commits
- 35b41c5eb EN2: shadow texture load as one G_PM_TEXI run (src/entity/Shadow.c, include/gbi_custom.h; TARGET_3DS flat shadow lists). mac01t f899 r1 (shadow draws, 30 draws): 795 -> 520 ops (26.5 -> 17.3 per draw). Closes 0, tris 146 same. mac01 2 of 2 and mac01t 2 of 2 pictures byte-equal to HEAD.
- b7e847d8c T1B: name the task tail rows 0xF0E0 + k (src/main_loop.c, src/cam_main.c; TARGET_3DS, pm_ph_on). Census only. mac01 2 of 2 and mac01t 2 of 2 pictures byte-equal.

## Dropped rows
- NP1 (sprite texture load as G_PM_TEXI in src/sprite.c spr_appendGfx_component_flat, both shading and non-shading branches). Census passed: mac01t f899 r3 (NPC sprite, 27 draws) 490 -> 301 ops (18.1 -> 11.1 per draw). Pictures: mac01 2 of 2 equal, mac01t 2 of 2 equal, pause_menu 1 of 1 equal. yard: top shot 0000_0_11 equal, 60 bottom shots (bot_0200 to bot_0259) differ by 18 to 140 px. A HEAD control run of yard (same binary as the baseline) differs from the baseline in 58 of 61 files, by 18 to 186 px. The byte-equal gate cannot separate the NP1 change from run noise on yard. Row undone per gate rule. Re-test needs a yard run that is reproducible at HEAD first.
- No other row dropped.

## EN2 gate note
- Plan target was 16 or less ops per shadow draw. Result is 17.3. The epic gate (direction down, pictures equal) passes. Remaining shadow ops: MTX, RM list DL with 5 ops, GEOMETRYMODE, SETCOMBINE x2 (callback and shape), PRIMCOLOR (if not G2 skipped), shape DL, CC (duplicate after the texture words), CLEARGEOM, VTX, TRI2, ENDDL, POPMTX. Candidate for the next row: drop the second CC of the circle shape (one op per circle draw, same combine state, no texture op between them changes combine). Alone it gives about 16.3 per draw.

## T1B tail owner table (mac01t, mdlsum f899, census window 600 to 899)
| mark | section | ops (window) | closes | note |
|---|---|---|---|---|
| 0xF0FE (before T1B) | task tail, all after last task | 332 | 19 | removed by T1B |
| 0xF0E2 (k=2) | gfx_back_ui: func_802C3EE4, render_screen_overlay_backUI, render_workers_backUI, render_hud_elements_backUI, render_effects_UI, render_game_mode_backUI | 319 | 19 | owner of the 19 closes |
| k=1, 3 to 14 | interact prompts, window root, world HUD, messages, front UI, curtains, cam_hud_elements, cam_item_entities, camera end | 13 total | 0 | rows below the top-12 print of mdlsum; not split further |
- The 19 closes are in gfx_back_ui. Split that section (render_hud_elements_backUI and draw_status_ui, src/inventory.c:1275) with more marks to name the owner. That is the next row for a diagnosis.

## Azahar starts (budget: 6 per row)
- EN2: 4 (mac01t, mac01, yard HEAD baseline, pause_menu HEAD baseline). Baselines ran with -Build head (lane-head, built from the pre-EN2 tree).
- NP1: 5 (mac01t, mac01, yard, pause_menu, yard HEAD control).
- T1B: 2 (mac01t, mac01).
- mac01t log gate 1900 to 2200 (mdlsum total ops): HEAD 2315 (fail, high), EN2 2040 (pass), T1B 1844 (fail, low). The gate range is from HEAD and is now outside after EN2. Update the range in azt_scenarios.json and test_catalog.md if the orchestrator agrees.

## Baseline shots
- build3ds/azt/shots_headbase/mac01, mac01t (from HEAD e8b37866c).
- build3ds/azt/shots_headbase/yard (61 files), pause_menu (1 file): from the pre-EN2 tree (c3d766548 content), lane-head build.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_BUILD_CAP=900 sh 3ds/build.sh; then cp 3ds/src/map_restore_table.c.bak 3ds/src/map_restore_table.c.
- Test: PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row <row> [-Try for mac01t] [-Build head].
- Lanes: build3ds/lane-en2 (EN2 build), build3ds/lane-head (HEAD build), build3ds/lane-np1 (NP1 build, dropped), build3ds/lane-t1b (T1B build).

## Open items
- Yard bottom-screen shots are not reproducible at HEAD (58 of 61 differ between two HEAD runs). The yard byte-equal gate needs a stable yard capture (fixed frames or a shot that does not move) before a sprite row can be judged on yard.
- The NP1 change is in the working history only as a dropped edit. It is not in the tree.
- Drop the duplicate circle CC (candidate above) if the orchestrator approves a further EN2 step.

## Next steps
1. Add marks inside gfx_back_ui (split at render_hud_elements_backUI and render_game_mode_backUI) to name the 19 closes.
2. Re-test NP1 after the yard capture is stable.
3. Consider the second circle CC removal under EN2.
