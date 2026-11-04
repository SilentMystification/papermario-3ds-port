# Investigation: console UI triage (8 items), 2026-11-03

Branch native. Read-only: no source edit. No Azahar start used (UTRI budget 0 of 6). No push.
Scope: the eight console playtest UI bugs listed in the task. Each cause is PROVEN (read in code, the line states the fact) or INFERRED (a cause that fits the code and the symptom, not tested). None is tested on a shot yet.

Related existing notes:
- docs/plans/playtest_2026-10-20.md:12 (PT04, save choice double text and glove) and 3ds/SLICES.md:36 and :104 (PT04 and PT72: not reproduced in Azahar, console only).
- docs/handoff/investigate_render_triage.md (commit 627b3bb16) covers a different set of seven items. Its groups do not cover items 1 to 8 here.
- docs/handoff/agent_uianchor.md covers the wide anchor rows (UA1 to UA10). Those rows are on another branch. Item 5 is audio and does not come from a UI draw.

## Shared mechanisms found

Stereo runs every display list twice (one pass per eye). A perspective matrix gets the eye shift. An orthographic matrix does not.
- pm_nusys.c:81: `if (pm_gpu_stereo()) { pm_gpu_eye(1); pm_gbi_run(gfxList, n); pm_gpu_eye(0); }`. The whole task runs again for the right eye.
- pm_gbi.c:446-456 (`upload_mvp`): the shift applies only when the projection has a perspective row (`proj[0..2][3] != 0`). The comment says an orthographic matrix (UI) stays at the screen plane.
- The in-game HUD complex path builds an orthographic matrix (hud_element.c:1817, `guOrthoF(camera->mtxPerspective, 0, 320, -240, 0, ...)`). It gets no shift.
- The 3D cameras take the perspective branch (cam_main.c:84, `guPerspectiveF`). The orthographic branch is at cam_main.c:99. The battle camera is a 3D camera, so its matrix is perspective (INFERRED from the branch, not read per frame).

HUD rectangles have an edge rule and a clip (hud_element.c). Both affect a few pixels at the screen edge or at the bottom row.
- hud_element.c:432-437 (rows) and :466-472 (columns): a texture of 16 or fewer texels takes `lry = baseY + drawSizeY` (no minus 1). A larger texture takes `drawSizeY - 1`. The same rule applies to x.
- hud_element.c:1895-1897: a clipped draw sets the scissor to x 12 to 308 and y 20 to 220 when not wide. With wide on the scissor is the full screen.

## Group G1: stereo second eye pass (items 1 and 3)

### Item 1: menu selector draws the text twice and two gloves

Probable cause (INFERRED):
- The right eye pass is a full second run of the display list (pm_nusys.c:81). The UI is orthographic and takes no shift (pm_gbi.c:446-456), so an orthographic UI element alone does not double. Only a perspective element shows parallax when the 3D slider is up. Two images with an offset look like a double text and a double glove. This fits "console only" because Azahar runs in mono (slider 0) by default.
- The clear differs per eye. The left target clears at frame end (pm_gpu.c:2003, `if (want_clear && drew)`) or mid frame (pm_gpu.c:631, 1846). The right target clears at the start of its pass (pm_gpu.c:1771, `if (right && !r_cleared) ... C3D_RenderTargetClear(targetR ...)`). If a frame does not clear, the two targets keep different old content. This is PROVEN as a difference in the code. Its effect on this item is not PROVEN.
- A second candidate is the rainbow text path (PT08_fix.md:18 notes the rainbow palette reload, `MSG_PRINT_FLAG_10`). Not checked here.
- The user asked whether the second UI draw is the cause. The second draw here is the right eye pass, not a second draw in one image.

Fix proposal:
- If the cause is stereo: the UI draw of this window must have no eye shift (one change in pm_gbi.c:446-456, or the UI path for the message window), and the clears must match per eye.
- If not stereo: read the rainbow palette path (msg_draw.c:1347-1349).

Test:
- One Azahar shot with the existing debug switch `stereo` (pm_gpu.c:1971 `if (pm_debug_has("stereo")) eye_slider = 1.f;`) on the save choice. The double text and glove should appear if the cause is stereo. This needs a new scenario in azt_scenarios.json and a catalog row in the same commit.
- Console check (the user): the same save choice with the 3D slider at 0 and then up.

### Item 3: battle star points at the wrong depth in 3D

Probable cause (INFERRED):
- PROVEN: the battle star point elements are created with render depth 20 (src/battle/16C8E0.c:217-231) and drawn with `hud_element_draw_clipped` in the battle HUD code (16C8E0.c:1030-1060). They have no manual render flag.
- PROVEN: the in-game HUD complex path is orthographic (hud_element.c:1817). It gets no eye shift.
- INFERRED: the battle star points draw under the battle camera (perspective). They then get the eye shift and sit at a battle depth, not at the screen plane of the in-game HUD. The draw call uses whatever camera matrix is current, so the camera at the draw must be confirmed.

Fix proposal:
- Draw the battle star points through the orthographic path, or set the HUD draws to no eye shift. One change in pm_gbi.c:446-456 (a flag set by the HUD draw) keeps the rest of the battle 3D unchanged.
- Add one log line: `gCurrentCamID` at the star point draw.

Test:
- One Azahar shot with the `stereo` switch on the battle_start scene (an existing scenario is used in the handoffs). Compare the star points with the stereo switch off. The bar must move relative to the enemy in the shot when it changes.

## Group G2: HUD rectangle edge and clip (items 2 and 7)

### Item 2: enemy health bar 1 pixel too tall (bottom)

Probable cause (PROVEN for the extra row, INFERRED for the console symptom):
- The enemy bar is `HES_HPBar`, a 32 by 16 image (battle_ui_gfx.c:259; assets/us/ui/battle/hp_bar.png is 32 by 16). It is drawn at 16C8E0.c:918-924.
- The height rule at hud_element.c:432-437 gives `lry = baseY + drawSizeY` for height 16. So the element draws 17 rows for a 16-row image. The bottom row is the 17th row.
- The width rule gives the correct width for 32 texels (hud_element.c:466-472 takes `drawSizeX - 1`). So the rule does not explain a "too long" side. The report says "too tall/long on the bottom", so the length part is not explained.
- The rule looks like a copy of the N64 edge rule. It is not known whether the N64 draws the 17th row. Do not change it before one shot shows the row.

Fix proposal:
- If the 17th row is visible and wrong: use `drawSizeY - 1` for height 16 too. This changes every 16-high HUD element (status icons, menu icons, the danger icon). Check the battle menu and the pause screen after the change.
- Decide with the user first, because the rule may match the N64 output.

Test:
- One Azahar shot of the battle_start scene at the enemy bar, with the bar at full size. Read the bottom row of the bar in the PPM. If the row is the bar fill colour, the cause is this rule.

### Item 7: battle danger notifier, letter D cut off

Probable cause (INFERRED):
- PROVEN: the danger icon is `HES_Danger`, a 40 by 16 image (global_hud_scripts.c:2151; assets/us/ui/battle/status/danger.png is 40 by 16 and shows the full word DANGER).
- PROVEN: the icon draws with `hud_element_draw_next` at `screenX + 2` (status_icons.c, the danger block near :946-962). The next draw does not set a scissor. It uses the scissor left by the last clipped draw.
- PROVEN: a clipped draw sets the left scissor edge to x 12 when not wide (hud_element.c:1895-1897).
- INFERRED: when the battle actor is near the left screen edge, the D sits at x less than 12 and the scissor cuts it. With wide on the scissor is x 0, so the cut should not happen with wide on. The console state (wide on or off) is not known.

Fix proposal:
- Set a full screen scissor before the danger draw (the same call as star_power_shimmer_draw uses, status_star_shimmer.c:159), or draw the icon with a full screen scissor in the non-wide mode too. Keep the other icons unchanged.

Test:
- One Azahar shot in battle with Mario at `curHP <= DANGER_THRESHOLD` and an actor at the left edge (wide off). Log the scissor x0 at the danger draw. If x0 is 12 and the D is cut, the cause is confirmed.

## Group G3: audio voices (item 5)

### Item 5: sound effects go missing when many play at once

Probable cause (PROVEN for the voice count, INFERRED for the effect on the jingle):
- PROVEN: the game creates 24 physical voices (src/audio/core/system.c:49, `config.num_pvoice = 24`).
- PROVEN: the port plays 23 of them. `PM_DSPA_VOICES 23` (3ds/include/pm_dspaud.h:7). The loop in src/audio/core/dsp_voice.c:90 runs `i < PM_DSPA_VOICES`. Physical voice 23 is never started.
- PROVEN: ndsp has 24 channels. The port uses channel 23 for the effect return (pm_dspaud.c:386-388 and :320). So the 24th voice has no free channel.
- PROVEN: the sound effect manager has 8 players (src/audio.h:840, `players[8]`). It starts at voice 16 (engine.c:250, `au_sfx_init(... , 16)`; sfx_player.c:478-482). The players therefore use voices 16 to 23 (sfx_player.c:1151-1158).
- PROVEN: the player search starts at the lower 3 bits of the sound ID and goes down (sfx_player.c, the search loops near :870 to :900). Player 7 (voice 23) is therefore used first only by a sound whose low bits are 7. Any sound that takes player 7 is silent in the port.
- So the answer to "do we run out of voices" is: the game has 24 voices and 8 sound effect players. The port has one silent voice, the 24th. The steal rule is the game rule (sfx_player.c:865-905, priority compare; bgm_player.c:1182-1243). It is not a port rule.
- INFERRED: the file select jingle is silent only sometimes. A silent voice 23 would be silent every time, so a missing jingle is probably the steal rule or the BGM voices, not voice 23 alone. Not checked.

Fix proposal:
- Give voice 23 a playing channel. Options: (a) move the effect return off channel 23 (for example into the aux callback path already used for the wet mix, pm_dspaud.c:428-441); (b) share channel 23 with voice 23 when the effect return is idle. Choose with the user, because (a) and (b) change the sound.

Test:
- One Azahar run with `audiostat` (the line `voices max`). Count the voices in use. With 8 effects in a battle, voice 23 should show use. This needs a new scenario in azt_scenarios.json and a catalog row in the same commit.

## Group G4: status bar state (item 4)

### Item 4: a hit HP block restores stats, the HUD does not update in real time

Probable cause (INFERRED):
- PROVEN: the status bar shows only when `displayHP != curHP` and `ignoreChanges` is false (src/inventory.c:607-615).
- PROVEN: the displayed HP moves one point each 4 game steps (inventory.c:640-657, `frameCounter % 4`). A heal of 10 HP takes 40 game steps. At 30 game steps a second this is about 1.3 s. The N64 has the same rule, so this is a delay, not the bug.
- INFERRED: the bar stays hidden when `ignoreChanges` is true. Two callers set it: src/evt/f8f60_len_1560.c:527 (`ShowWorldStatusBar` with TRUE) and src/8a860_len_3f30.c:2231 (the popup menu). If the hit block script leaves the flag set, the HUD does not show the change.
- The hit block itself was not traced to its script. The heal path is the item block, for example src/world/area_tst/tst_01/entity.c:7-8 (two heart blocks, a test map).

Fix proposal:
- Read the block script and the item pickup script for a call that sets `ignoreChanges` and does not clear it. Clear it on the heal path, or show the bar on any HP change while the flag is set.

Test:
- One Azahar run on tst_01 with `hp=5`. One log line per frame for 60 frames: `displayHP`, `curHP`, `hidden`, `ignoreChanges`. The HP value must change on the HUD in the shot.

## Group G5: Bombette charge bar (item 6)

### Item 6: brief long horizontal line at the charge bar height

Probable cause (not found):
- PROVEN: Bombette's bomb command uses the mash meter: `SetupMashMeter` at src/battle/partner/bombette.c:1029-1044 and `LoadActionCommand` at :1021. The bar draws in src/battle/action_cmd.c:133-222 as COPY rectangles 5 pixels tall (`posY` to `posY + 5`).
- PROVEN: `LoadActionCommand` and `SetupMashMeter` run in the same script step. No frame draws between them, so a stale meter cannot draw in that gap.
- PROVEN: `acs->meterFillWidth` resets to 0 at load (action_cmd/bomb.c:44 and :92).
- INFERRED: the cause is not in the bomb path. Candidate: the fill width or the interval values carry over from a previous mash command into the first frames of the meter draw (action_cmd.c:146-150 smooths the fill toward a target). Not confirmed.

Fix proposal:
- Not decided. Next step is a read of the values, not a fix.

Test:
- One Azahar run on the bomb command. One log line per frame for 15 frames: `mashMeterNumIntervals`, the cutoffs, `meterFillWidth`, and the rectangle arguments. The shot must show the bar. Then decide.

## Group G6: HUD star meter (item 8)

### Item 8: HUD star meter does not render properly

Probable cause (INFERRED, low confidence):
- PROVEN: the star power shimmer draw uses a fixed orthographic matrix of 0 to 320 by 0 to 240 and a full viewport (src/status_star_shimmer.c:159-163). It does not use the wide helpers.
- PROVEN: the star power bar and the other status items use the wide helpers (PT55 in docs/plans/wide_audit.md:14, src/inventory.c). The bar moves with wide. The shimmer does not.
- INFERRED: with wide on, the shimmer particles sit at the 4:3 position while the bar sits at the wide position. The report does not say which part is wrong. Not checked.

Fix proposal:
- Use the wide helper for the shimmer origin, or the same offset as the bar. Decide after the shot.

Test:
- One Azahar shot with wide on and the star power full (`sp=<n>` or the existing debug switch), at the bar. Compare the particle position with the bar.

## Summary table

| Item | Group | Cause | Status | Test |
|---|---|---|---|---|
| 1 | G1 stereo | right eye pass, perspective element parallax | INFERRED | Azahar `stereo` switch on save choice (new scenario) |
| 2 | G2 HUD edge | hud_element.c:432-437, 17th row for 16-high images | PROVEN row, INFERRED symptom | battle_start, enemy bar bottom row |
| 3 | G1 stereo | battle star points perspective, no shift on ortho | INFERRED | battle_start with `stereo` on and off |
| 4 | G4 status bar | ignoreChanges left set, or 4-step ramp | INFERRED | tst_01 with hp=5, 60-frame log |
| 5 | G3 audio | 24 voices, 23 played, voice 23 silent (SFX player 7) | PROVEN count, INFERRED jingle | audiostat voices max, 8 effects |
| 6 | G5 mash meter | no cause in the bomb path | not found | bomb command, 15-frame log and shot |
| 7 | G2 HUD clip | scissor x 12 cuts the icon left edge | INFERRED | battle at left edge, scissor x0 log |
| 8 | G6 shimmer | fixed 320 ortho, not wide | INFERRED, low | wide on, star power full |

## Next steps (order)

1. Decide the stereo test (items 1 and 3). This test needs one Azahar start and a new scenario. Justification line first.
2. Item 5: decide the channel 23 option with the user (option a or b). The audiostat check needs one start.
3. Item 2: one shot of the enemy bar bottom row, then decide the rule change with the user.
4. Item 7: scissor log in one battle run.
5. Items 4, 6, 8: one log or shot each.

No code was changed. No commit other than this file. No Azahar start was used.
