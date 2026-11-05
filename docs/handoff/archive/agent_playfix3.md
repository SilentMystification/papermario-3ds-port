# agent_playfix3 handoff

## State
- Branch native. HEAD after this file: see git log. No push. map_restore_table.c not committed (user copy sha 92645afb, restored after each build).
- Build lane for tests: hang1a3 (source = HEAD a4d3a2aed). Lane ui1 has the uishow=choice hook.
- Azahar starts (test_budget.csv / this run): TEX1 6 of 8. UI1 4 of 8. UI2 4 of 8.

## Commits
- 131c2d3f3 docs: TEX1 nok14_lake result. Window shows lake water and terrain. Clamp change (branch tex1-clamp e8896aa97) does not change the terrain pixels. NOT merged to native. Branch kept. OPEN ITEM.
- f99354de7 UI1: toad_house_menu probe scenario (3 runs). Room entry not found.
- 6b3b0eec5 UI1: uishow=choice debug hook (pm_warp.c, frame 200, MSG_Choice_000A) and ui1_choice scenario. Menu NOT drawn. UI1 is with the investigation agent. Do not touch.

## Open rows (UI2 only)
- Enemy bar: battle_start run (lane hang1a3, shot 182) shows no enemy HP bar. The bar draws only while the battle menu is open (src/battle/16C8E0.c:905-906). NEXT: debug hook uishow=battlemenu that sets BS_FLAGS1_MENU_OPEN (or opens the battle menu) at a fixed frame, then one shot. Then read the bottom row of the bar (17th row rule, hud_element.c:432-437).
- Danger D (hud_element.c:1895, scissor x 12): NOT STARTED. Needs a low-HP battle and an actor at the left edge. Hook: a log of the scissor x0 at the danger draw.
- Star points bar (depth 20, battle HUD, 16C8E0.c:217-231): NOT STARTED. Test with the stereo switch on and off on battle_start.
- Bombette charge bar (action_cmd.c:133-222): NOT STARTED. Needs a Bombette bomb command. Log per frame of meterFillWidth.
- Star meter HUD (status_star_shimmer.c:159): NOT STARTED. Needs wide on and star power full.
- HP block HUD update (inventory.c:607-615, ignoreChanges): NOT STARTED. Scenario on tst_01 with hp=5 and a 60-frame log.
- Blue switch: NOT STARTED. No item info in the handoff notes.

## Commands
- Build: `py 3ds/tools/gen_map_restore.py`, then `PM_LANE=<lane> sh 3ds/build.sh`, then restore src/map_restore_table.c from the user copy (sha 92645afb).
- Test: `PM_ROW=UI2 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row UI2 -Build hang1a3 [-Try]`.
- Justification: one line in 3ds/logs/perf/status_UI2.txt before each start.
- Montage check: convert shot .ppm to PNG with `py` (PIL installed, not `py -I`), read the PNG.
