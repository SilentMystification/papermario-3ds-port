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

## Status at this report (UA1, UA4, UA7)

- UA4 (battle_wide): gate by eye. Wheel log `debug wheel open: base 33 183`. Wheel beside Mario in wide, same place relative to Mario as battle_start wide off (shot 0007), within a few shot pixels. Byte-equal to HEAD not run.
- UA7 (pause_spin_wide, pause_spin): gate by eye. Pause window at frame 116 wide on has the same edges and text size as wide off (shot 0004 of both). Code: src/draw_box.c frustum +-100 when pm_wide (cancels the port 1.25 for perspective). Byte-equal wide off by construction (pm_wide 0 keeps +-80). Risk: every draw_box quads window in wide mode changes shape (now equal to wide off). Not checked on other windows.
- UA1 (talk_wide, pos=-66:20:-518:0 with A): log `debug talk anchor` fired with speaker -1 (NPC_PLAYER), screen 152 143. Speaker is Mario, not the Toad. Shot 0180 is a fade and a wall, not the bubble. Gate not passed. Toad n0 is not talked to by the A press, or the A press reached another message. Next: log the speaker NPC id and the Toad's own screen position; hold the A press longer.
- UA2, UA3, UA8, UA9: not started.
- Marker miss at 20 s happens on about half the starts (shared sdmc log.txt). The shots still exist when the run reached the until.

## UA1 Mario test (start 12, mario_proj_wide, wide on)

- marlog: get_screen_coords of Mario = (160, 176) at every frame from 80 to 100. Mario pos -200 0 -370 (readback y 14.8 at place, falls to 0).
- The camera follows Mario, so Mario projects to the screen centre x 160. At x 160 the fix (1.25 x - 40) gives 160 too. The error is 0 before and after the fix. Mario cannot test the fix at the centre.
- Next: a subject off the centre. Options: (a) project a fixed world point 100 units to the side of Mario and read a drawn object at that point; (b) a camera with a fixed look-at (none found yet).
- Derivation (not measured): wide 3D draw px = 200 + 200 ndc (pm_gpu.c:196), 2D px = 40 + game x. Post-fix game x = 1.25 x_raw - 40 gives 2D px = 200 + 200 ndc, the same as the 3D draw.

## Byte-equal gate, branch vs main (7b8befae2), wide off

- yard (UA lane vs UAMAIN lane, top shot 0000_0_11): byte-equal.
- talk_off (wide off speech box, 200 top shots): 200 of 200 byte-equal.
- battle_start: 0000 to 0003 byte-equal; 0004 differs in x 58 to 110, y 98 to 171 (2510 px). A repeat of the same UA binary also differs from itself at 0004, so the 0004 difference is run noise, not the branch.

## UA3 (coin counter), start 1 and 2

- uishow=coins (pm_warp.c hook at frame 120, calls show_coin_counter). Log `debug uishow: coins opened f120` present.
- inventory.c: WIN_CURRENCY_COUNTER at pm_wide_left(32), pm_wide_bottom(164).
- coin_wide shot 0000 (frame 122): no counter visible; the HUD is not drawn in this frame either. Gate not passed. Next: a later frame (the counter draws after the HUD fade-in), or frames where the HUD is on (the yard shot shows HUD at frame 200+).
- Battle popup (popup_messages.c posY 192) and the other UA3 sites: not done.

## UA3 coin counter, starts 3 to 6 (blocked)

- coin_wide hook at frame 120 (start 3, shots 125 to 149): shot frame 149 has no HUD at all. Hook at frame 200 (start 6, shots 200 to 216): shot 216 also has no HUD and no counter.
- The HUD draws in talk_wide shots at frame 200 and later, so the HUD state differs by scenario. Not explained.

## UA8 chapter card, starts 1 and 2 (blocked)

- fxtest=113 (chapter_change, index from the E() list in pm_fxtest.c line 145) spawns fx_chapter_change(0, 100, 100, 0, 1, 0). Shots A and B (shot 2) show no card.
- Cause (inferred): the card position is set by the caller each frame (kmr_22/main.c:40-60 writes chapterPos and endOfPos). fxtest does not do that, so the card sits off screen.
- Next: a uishow=chapter hook that repeats the kmr_22 loop (set chapterPos x and y each frame) or a warp to kmr_22 with the story state that runs the card.
