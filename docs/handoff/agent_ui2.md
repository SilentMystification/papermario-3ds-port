# agent_ui2 handoff

## State
- Branch native, worktree papermario-3ds-port-main. No push. map_restore_table.c not committed (sha 92645afb, restored after each build).
- Build lane for UI2: ui2 (`PM_LANE=ui2 sh 3ds/build.sh`, source = working tree with the battlemenu hook).
- UI2 Azahar starts: 6 of 8 (test_budget.csv, UI2 rows with start).

## Commits
- 22ad77382 UI2 step 1: battlemenu debug switch (pm_warp.c, scene frame battlef+10: BS_FLAGS1_MENU_OPEN and save_tattle_flags per enemy), battle_menu_bar and battle_danger scenarios, catalog rows.

## Evidence so far
- battle_menu_bar run 1 (no tattle step): no enemy HP bar. Cause: 181810.c is_actor_health_bar_visible needs the enemy tattled (or Peekaboo). Fixed in the hook.
- battle_menu_bar run 2 (shot 0004, frame 182, build ui2): HP bar draws at the Goomba. Yellow fill with a dark outline. One dark red row (a00000, PPM row 330, x 50 to 54) at the end of the fill. The red is not in assets/us/ui/battle/hp_bar.png (32 by 16, opaque rows 0 to 8 only, palette has no red). The red matches the code prim at 16C8E0.c:946-947 (168,0,0 height 1 and 255,0,0 height 4). So the red is the lost-HP line, not image row 17.
- battle_danger (shot 0008, build ui2): DANGER banner draws at Mario, HP 5/10. The shot file is truncated (bottom 1/4 black, PIL reports truncated PPM). Re-run may fix that.
- fc602ce72 (16 row rule, hud_element.c:432-437) is in HEAD. Not yet proven by a baseline pair. HES_HPBar rows 9 to 15 are transparent, so the 16 row rule can change only the drawn transparent row. A baseline (lane without the rule) is needed to see any pixel change.

## Open items (step 2, none done)
1. Enemy bar 1 px too long: open. The red line is the lost-HP prim (16C8E0.c:946-947). Next: measure the bar fill and the prim x and y in the shot against the prim formula (temp = 25 - healthFraction) before any edit. Expected: the line width or x start is wrong by 1 px.
2. Danger notifier D cut (hud_element.c:1895, scissor x 12): open. Read battle_danger shots 0000 to 0008 for the D left edge; the banner in 0008 is not cut at the visible side.
3. Star points bar depth 20 (16C8E0.c:217-231): not started. Test with stereo on and off on battle_start.
4. Bombette charge bar (action_cmd.c:133-222): not started. Needs a Bombette bomb command.
5. HUD star meter (status_star_shimmer.c:159-163): not started. Needs wide on and star power full.
6. HP block HUD update (inventory.c:607-615, ignoreChanges; :640 ramp): not started. Scenario on tst_01 with hp=5 and a 60 frame log.
7. Blue switch poof (entity/model/BlueSwitch.c:24-38): not started. Needs a drawlog.

## Commands
- Build: `py 3ds/tools/gen_map_restore.py`, then `PM_LANE=ui2 sh 3ds/build.sh`, then restore src/map_restore_table.c from the user copy (sha 92645afb).
- Test: `PM_ROW=UI2 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row UI2 -Build ui2 -Try`.
- Justification: one line in 3ds/logs/perf/status_UI2.txt before each start.
- Picture: PIL (`py`) converts the .ppm; the PPM is 240 by 400 rotated (screen x is PPM y).
