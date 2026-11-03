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
