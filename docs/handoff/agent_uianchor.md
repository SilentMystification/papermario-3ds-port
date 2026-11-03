# Handoff UIANCHOR (agent_uianchor)

Source: plan docs/handoff/plan_ui_anchor.md (branch uc2c, commit f44c47e06). Worktree: C:\Users\Andrew\Documents\GitHub\papermario-3ds-port-uianchor, branch uianchor (from main 7d19c5808).
Azahar budget file: the worktree 3ds/logs/test_budget.csv (starts counted per row UIANCHOR). Justification lines: main folder 3ds/logs/perf/status_UIANCHOR.txt.

## State

| Row | State | Notes |
|---|---|---|
| UA1 world projection | code edited, uncommitted, gate NOT passed | cam_main.c get_screen_coords: screenX = 1.25 x - 40 when pm_wide_cam. screen_overlays.c set_screen_overlay_center_worldpos: same x rule. btl_states_menus.c: one-time pm_log_d "debug target select: pointer x y" and pm_gpu_shot_burst(8) at the target pointer (debug only). azt_scenarios.json: battle_target_wide (unverified). Catalog row not added yet. |
| UA2 speech box top | not started | |
| UA3 bottom boxes | not started | |
| UA4 battle wheel | not started | |
| UA7 pause spin (new, after UA4, before UA5) | not started | Code: src/pause/pause_main.c (window flip angles :60-66, pause_update_page_* :378-440, guScaleF in the draw path :568-583). Check the 320x240 centered assumption against wide (cam zoom 789649bef). Fix with pm_wide.h helpers. Scenario: pause open in wide, shots through the spin. Wide-off byte-equal. |
| UA8 chapter intro (new, after UA7) | not started | Code: src/effects/chapter_change.c (chapterPos, endOfPos :151-202, draw :160-172). Find the Mario and foe positions in the intro (native 320x240 absolute vs wide view). Scenario: warp to the chapter intro (find the debug string). Wide-off byte-equal. |
| UA5, UA6 | not started | Later epic (action command). Do not start. |

## UA1 blocker (4 Azahar starts used of 8)

- Start 1: Azahar exited at 12 s, debug3ds.txt held another row's string (shared sdmc folder, another run wrote it).
- Start 2: wheel open at frame 199 (shot 0199), one padscript A, no target select, no pointer log. Scenario ran to cap.
- Start 3: marker missing at 21 s, Azahar exited. Log had the warp line in build3ds/lane-UIANCHOR/last_log.txt (copied at the end).
- Start 4: padscript wait150,A,wait90,A,wait90 (A at about frames 162 and 207). Marker missing again at 21 s. Shot 0199 is identical to start 2: the A presses did not change the wheel.
- Open question: why A does not reach the battle wheel. Input is read in src/battle/16C8E0.c:262-284 (curButtonsPressed from gGameStatusPtr->pressedButtons[0] & battleStatus->inputBitmask). The wheel reacts in src/battle/btl_states_menus.c:765-770 (BTL_MENU_STATE_CHOOSING, BUTTON_A). Check inputBitmask during the battle intro.
- Two of four starts lost Azahar or the marker to other runs on the shared sdmc folder. Wait for a free Azahar before each start.

## Commands

- Build: `cd <worktree> && python gen: /c/Python314/python 3ds/tools/gen_map_restore.py`, copy 3ds/src/map_restore_table.c from the main folder (never commit it), then `PM_LANE=UIANCHOR sh 3ds/build.sh` (about 1 min incremental).
- Test: `PM_ROW=UIANCHOR powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 battle_target_wide -Row UIANCHOR -Build UIANCHOR -Try`
- Shots: build3ds/azt/shots/<scenario>/*.ppm. Convert to PNG to read: `/c/Python314/python -I <scratchpad>/ppm2png.py in.ppm out.png` (script in the agent scratchpad).
- Asset inputs not in git (copied from the main folder): assets/us, ver/us/build, ver/us/baserom.z64.

## Next steps

1. UA1: find the battle input (see the open question), one start with the fix, shot read by eye (pointer over the Goomba). Then wide-off byte-equal on battle_start (HEAD binary needed as base: build it in a lane from main).
2. If UA1 cannot pass in 2 more starts, report INCOMPLETE and keep the code uncommitted.
3. UA2 to UA4, UA7, UA8 as in the plan list above. Each row: one gate, one wide-off check, one commit on uianchor.

## Rules kept

No push. No stubs. No toggles. The debug log lines and the shot burst in btl_states_menus.c stay (debug tools may stay). No attribution in commits.

## UA9 (item shop prices, new, after UA8)

- Report: in the item shop the coin price numbers sit at native 320x240 positions while the shop panel is placed relative to the wide view.
- Code to read first: src/world/script_api/shops.c (shop windows: name (100, 66) :972, desc y 184 :974, :976, from the plan; price draw calls there). Per-map shop scripts: src/world/area_mac/mac_00/shop.c, mac_01, mac_04, hos_03, jan_03, nok_01, obk_03, dro_01 (look for draw_number, price positions, SCREEN_WIDTH constants).
- Fix: pm_wide.h helpers (pm_wide_left / pm_wide_right / pm_wide_top / pm_wide_bottom) on the price coordinates so they move with their item list. Wide-off must stay byte-equal.
- Scenario: warp into a shop map (debug string to find: the Toad Town item shop in mac_00 or the map that opens a shop, preset toad plus a padscript walk to the shopkeeper). Shot of the item list with wide on. Until: a log line when the shop list opens (add one debug line if none exists).
- Status: not started. No test run for UA9 yet. Budget 8 starts per row, UA9 has 0 used.

## Talk route attempt (UA1 option A, UA2 talk_wide), starts 5 and 6

- Start 5 (talk_wide discovery, preset=toad wide simlog): NPC n0 at x -66, y 20; Mario at x -100, y 30 (log simstep f110). The NPC z is not logged.
- Start 6 (padscript wait20,right*30,A,wait150): Mario did not move (pos -100 30 -370 at f300, f600, f900, f1200). No talk anchor line, no bubble. Cause not known: stick input, a wall at z -370, or the NPC is not at z -370.
- UA1 budget: 6 of 8 used. The msg_api.c:96 log and pm_gpu_shot_burst(8) are in the working tree, uncommitted.
- Next: add the NPC z to the simstep line (one line in pm_warp.c), then one start with a route to n0 (dright or the go_ token with the z).

## NPC position (start 7 and 8, preset=toad wide simlog)

- Toad n0 at x -66, y 20, z -532. Mario at x -100, y 30, z -370 (f110). Distance about 162 units in z.
- Plan (coordinator): pos=<x>:<y>:<z> from the POSWARP branch, placing Mario about 40 units in front of n0, then A. Poswarp commit not on branch poswarp at 262fd3c49 time (head 99266ab2d, BOTLAYOUT commits).
- UA1 budget: 8 of 8 used (starts 1 to 8). Waiting for a reset.
