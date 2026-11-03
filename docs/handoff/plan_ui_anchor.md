# Plan UIANCHOR (UI anchors in wide mode)

Row UIANCHOR (UI anchor): UI elements that use fixed N64 coordinates sit in the wrong place in wide mode. Plan and scope only. Source at HEAD 37f4b1fc5 (branch uc2c), read after the ZOOM commit 789649bef.

Marks: CONFIRMED = read in the file at the line given. INFERRED = reasoned from the code, not seen on a shot.

## 1. Wide mode today, in numbers

| Quantity | Value | Mark |
|---|---|---|
| 3DS top screen | 400 x 240. The target buffer is 240 x 400, rotated. | CONFIRMED 3ds/src/pm_gpu.c:1044-1045 (comment) |
| N64 coordinate space | SCREEN_WIDTH 320, SCREEN_HEIGHT 240 | CONFIRMED include/macros.h:126-127 |
| Original game view | 296 x 200 at (12, 20), so rows 20 to 219 | CONFIRMED src/cam_main.c:462 |
| Wide game view | set_cam_viewport changes (12, 20, 296, 200) to (0, 0, 320, 240) for CAM_DEFAULT and CAM_BATTLE | CONFIRMED src/cam_main.c:462-464 |
| Wide perspective | vfov unchanged, aspect 320 / 240 x 1.25. The 200 view rows fill 240 rows: zoom 6 / 5 about screen (200, 120) | CONFIRMED src/cam_main.c:9-10, :84 |
| When wide is on | pm_wide = pm_wide_cfg, except in logos, title, language select and file select (4:3) | CONFIRMED src/game_modes.c:266-267 |
| 3D (perspective) draws | X scale 1.25, Y scale 1: screen x = 1.25 x game x | CONFIRMED 3ds/src/pm_gpu.c:196-197 |
| 2D draws (texrect, fillrect, orthographic) | 1:1, centered: screen x = 40 + game x, screen y = game y | CONFIRMED 3ds/src/pm_gpu.c:197 (ui_ortho), :2527-2528 (px, py) |
| 2D edge rule | a fill rect with x0 <= 0 or x1 >= 319 reaches screen x 0 or 400. A texrect with x0 == 0 or x1 == 319 extends at the same texel density. An orthographic vertex at clip x = +-w goes to x 1.25 w. | CONFIRMED 3ds/src/pm_gpu.c:2522, :2549-2550, :2591-2595 |
| Scissor | 2D: 40 + x, an edge at 0 or 320 goes to the screen edge. 3D: x * 5 / 4. | CONFIRMED 3ds/src/pm_gpu.c:1051-1053 |
| UI helpers | pm_wide_left(x) = x - 40, pm_wide_right(x) = x + 40 | CONFIRMED include/pm_wide.h:5-6 |
| Status bar | rest y 18 to -2 (moved up 20 rows), x pm_wide_left(12) = -28, right groups pm_wide_right | CONFIRMED src/inventory.c:6, :338, :847-895 |

What this means for UI, in game (N64) units:

- The screen shows game x -40 to 359 and y 0 to 239 for 2D draws. (CONFIRMED by the px rule.)
- The world view moved from rows 20 to 219 to rows 0 to 239. A UI element that sat a fixed distance from the view top now sits 20 rows too low. One at the view bottom sits 20 rows too high. This is the "text box too low" symptom. (INFERRED from the view change. The status bar fix at src/inventory.c:6 uses the same 20 rows.)
- A world point at original screen (x, y) now shows at (160 + (x - 160) x 6 / 5, 120 + (y - 120) x 6 / 5) in 2D units. (INFERRED from the perspective numbers above.)
- **World-to-screen projection is wrong in X.** get_screen_coords gives x = 160 + 160 ndc (viewport 320). The 3D draw puts that point at screen 200 + 200 ndc. A 2D draw at that x lands at 40 + 160 + 160 ndc = 200 + 160 ndc. So every icon placed from a world position is pulled to 0.8 of its distance from the center. Y is correct (both 120 + 120 ndc). CONFIRMED src/cam_main.c:500-524 has no wide term, and 3ds/src/pm_gpu.c:196-199.

## 2. Central option: one place in the port

Question: can pm_gpu.c / pm_gbi.c anchor UI by class without game code?

Answer: no. The port sees one primitive at a time (a texrect, a fill rect, an orthographic triangle batch, a scissor). It does not know the element class. Reasons, from the code:

1. One element is many primitives. draw_box draws a window frame as an orthographic mesh, and the text inside is drawn as separate glyph texrects (src/msg_draw.c, src/draw_box.c). A region rule (move a primitive by the screen half it is in) moves the frame halves apart and leaves glyphs at the center behind. (INFERRED)
2. The same screen region holds different classes. The pause menu (center), the battle wheel (near Mario) and enemy icons from get_screen_coords (world) all draw in the left half. A rule by position moves all of them. (INFERRED)
3. Full-screen layers are already handled by the edge rule (section 1). A region rule would conflict with it. (CONFIRMED 3ds/src/pm_gpu.c:2549-2550)
4. Windows and HUD elements are drawn later in central passes, not where the game sets their position: render_windows (src/windows.c:236, position at :280-281) and render_hud_elements_frontUI / backUI (src/hud_element.c:1200, :1324). So a "set anchor, draw, reset anchor" bracket around a game draw function does not reach them. (CONFIRMED)

A port display-list op (G_PM_ANCHOR dx dy, next to G_PM_RM 0x09 to G_PM_MODEL 0x0C in include/gbi_custom.h:204-218) is possible. The port would add dx, dy in px, py, the ui_ortho view translation (pm_gpu.c:197-199) and the scissor. It still needs a game-side tag per element (reason 4), and it changes shared draw code (rule 5: byte-equal checks on every scene that uses the path). It gives no coverage that per-site edits do not give. Not recommended now.

## 3. Inventory

Count rules: "draw calls" = `grep -c` of `hud_element_set_render_pos|draw_box(|draw_msg(|set_window_properties(|gDPSetScissor|gDPFillRectangle|gSPTextureRectangle|draw_number(|startup_draw_prim_rect|draw_ci_image_with_clipping|hud_element_draw_clipped` per file. "SW/SH" = `grep -c "SCREEN_WIDTH\|SCREEN_HEIGHT"`. "Sites to change" = the exact lines that hold an edge or view constant for this class. All counts are exact grep counts at HEAD 37f4b1fc5.

Anchor classes:

- TOP: dy = -20 (view top moved from row 20 to row 0).
- BOTTOM: dy = +20 (view bottom moved from row 219 to row 239).
- LEFT / RIGHT: dx = -40 / +40 (pm_wide_left / pm_wide_right).
- VIEW: placed next to actors in a fixed camera (battle). Follow the world zoom: x' = x + (x - 160) / 5, y' = y + (y - 120) / 5.
- WORLD: from get_screen_coords or an own projection. Fix the projection once.
- CENTER: no change. The 1:1 centered 2D rule already puts it in the middle.
- FULL: full-screen layer. Already handled by the port edge rule.
- OFF: wide is off in this game mode.

| System | File | Constants (file:line) | Draw calls / SW/SH (grep) | Sites to change | Anchor | Mark |
|---|---|---|---|---|---|---|
| Speech box (left, right, center styles) | src/msg_draw.c | windowBasePos (32, 13) :420-421, rewind arrow (253, 56) :426-427 (MSG_NORMAL_ARROW_X/Y :178-179) | 21 / 13 | 2 | TOP (y 13 to 0; -7 clips 7 rows, see row UA2) | CONFIRMED values, INFERRED class |
| Speech bubble placed from the speaker (tattle and other styles) | src/msg_draw.c | x clamp 18 and SCREEN_WIDTH - 18 :493-497, y clamp 20 and 170 :499-504 | (same file) | 4 | x LEFT/RIGHT, y TOP/BOTTOM (20 to 0, 170 to 190) | CONFIRMED values |
| Speaker origin clamp | src/msg.c | y 0 to 220 :1543-1548 (x already wide :1533-1542) | 5 / 3 | 1 | BOTTOM (220 to 240) | CONFIRMED |
| Sign, center popups, other msg windows | src/msg_draw.c, src/msg.c | (40/20, 28) msg_draw.c:693-694, :771-772, msg.c:778-783; (160 - w/2, 56) msg_draw.c:159-160, :824-825; (60, 110) msg_draw.c:834-835 | (above) | 0 | CENTER | INFERRED |
| World message clip | src/msg_draw.c | clipX already pm_wide :319-321 | (above) | 0 | done | CONFIRMED |
| Status bar (HP, FP, star points, coins) | src/inventory.c | FULLY_EXTENDED_Y :6, pm_wide_left(12) :338, :1535, pm_wide_right :847-895 | 41 / 3 (pm_wide 15) | 0 | done (TOP, LEFT, RIGHT) | CONFIRMED |
| Coin counter window | src/inventory.c | (32, 164) :1231 | (above) | 1 | BOTTOM + LEFT | CONFIRMED value |
| Field popup menus (item, partner, star piece) | src/8a860_len_3f30.c | PopupWinX/Y 32 or 20, 72 :368-373 (62 at :418); PopupDescY 186 :376; star piece counter (32, 164) :379-380; prompt (16, 88) :377-378; child title windows at (x, -6) are relative to the parent | 66 set_window_properties | 3 (desc y, counter x y) | desc BOTTOM, counter BOTTOM + LEFT, menu CENTER | CONFIRMED values |
| Item pickup boxes | src/item_entity.c | desc y 186 :2593; header (160 - w/2, 76) :2609, title (36) :2598 | 6 set_window_properties | 1 | desc BOTTOM, others CENTER | CONFIRMED values |
| Shop windows | src/world/script_api/shops.c | name (100, 66) :972; desc y 184 :974, :976 | 3 set_window_properties | 2 | desc BOTTOM, name CENTER | CONFIRMED values |
| Battle message popups and action tips | src/battle/popup_messages.c | posY 192 :1800, :2067 (also BattleMessage_CurBoxPosY 192 :1041); 120 :1803, :2072; default 80 :1717 | 122 / 0; 13 set_window_properties | 3 | 192: BOTTOM; 80, 120: CENTER | CONFIRMED values |
| Level-up description box | src/battle/level_up.c | y 186 :1163, :1167 | 71 / 3 | 2 | BOTTOM | CONFIRMED values |
| Merlee popup | src/status_icons.c | y 80 :305, :317, :333 | 22 / 1 | 0 | CENTER | CONFIRMED values |
| Battle wheel menu | src/battle/btl_states_menus.c | BattleMenu_BasePosX/Y (54, 173) :604-605 | 94 / 3; 17 set_window_properties | 2 | VIEW | CONFIRMED values, INFERRED class |
| Battle moves and strats menus | src/battle/btl_states_menus.c | moves (POS_X, 68) :1218-1219; strats (32 or 20, 68) :2106-2110 | (above) | 0 in first pass | CENTER (promote to VIEW only if the shot shows it apart from Mario) | INFERRED |
| Battle target cursor | src/battle/btl_states_menus.c | get_screen_coords :5548, :5575 | (above) | 0 | WORLD | CONFIRMED |
| Action command HUD | src/battle/action_cmd/*.c (47 files, 23 set hudPosX) | hudPosX = 50: 24, hudPosX = -48: 23, hudPosY = 80: 22, hudPosY = 96: 1; hudPosX compares with a literal: 23 | 185 hud_element_set_render_pos | 70 assignments + 23 compares = 93 | VIEW | CONFIRMED counts |
| Status icons, HP and status over actors | src/status_icons.c | get_screen_coords 16 calls | 22 / 1 | 0 | WORLD | CONFIRMED |
| Other world-anchored icons | 21 files call get_screen_coords, 59 calls outside its definition (encounter.c 5, evt/msg_api.c 5, battle/181810.c 3, battle/partner/parakarry.c 4, ...) | src/cam_main.c:500-524 | - | 1 (the function) | WORLD | CONFIRMED counts |
| Star wipe / battle-start wipe center | src/screen_overlays.c | own projection :416-429; fixed centers (15, 28), (305, 156) :595-596, :677-678 | 4 / 11 | 1 (projection) | WORLD; fixed centers FULL | CONFIRMED |
| Fades, screen overlays | src/screen_overlays.c, src/screen_render_util.c, src/curtains.c | full-screen rects 0..319 :219-220; screen_render_util wide branches :235-282; curtains frustum x 1.25 :257 | 4 / 11, 11 / 16, 0 / 3 | 0 | FULL (done) | CONFIRMED |
| HUD element and draw_box clipping | src/hud_element.c :453-462, :1896; src/draw_box.c :422-426 | already pm_wide_left / right | - | 0 | done | CONFIRMED |
| Pause menus | src/pause/*.c (badges 32, items 14, main 13, map 5, partners 16, spirits 3, stats 58, tabs 12 draw calls) | WIN_PAUSE_* windows | 153 draw calls | 0 | CENTER | INFERRED |
| Title screen | src/state_title_screen.c | - | 6 / 4 | 0 | OFF | CONFIRMED src/game_modes.c:266 |
| File select | src/filemenu/*.c (common 1, createfile 9, info 13, main 19, selectlanguage 3, yesno 5) | - | 50 draw calls | 0 | OFF | CONFIRMED src/game_modes.c:266 |
| Credits | src/world/area_end/common/Credits.inc.c | clip (10, 20, 300, 200) :341, :381, :497 | 3 SW/SH | 0 | CENTER | INFERRED |
| Damage numbers, action ratings | effects (3D, world space) | - | - | 0 | none (perspective path) | INFERRED |

Totals of sites to change: speech and bubble 7, bottom boxes and counters 9, battle wheel 2, action command 93, world projection 2. Total 113 sites.

## 4. Options with cost

### A. Per-element patches with helpers (recommended)

Add to include/pm_wide.h (the existing helper file):

```c
static inline int pm_wide_top(int y)    { return pm_wide ? y - 20 : y; }
static inline int pm_wide_bottom(int y) { return pm_wide ? y + 20 : y; }
static inline int pm_wide_vx(int x)     { return pm_wide ? x + (x - 160) / 5 : x; } /* VIEW: the world zoom 6 / 5 */
static inline int pm_wide_vy(int y)     { return pm_wide ? y + (y - 120) / 5 : y; }
```

Division is outside hot loops (positions set once per element), so hw_guidelines.md is not affected. (INFERRED)

World projection: in get_screen_coords (src/cam_main.c:522) and set_screen_overlay_center_worldpos (src/screen_overlays.c:428), when pm_wide_cam(camera): x = 1.25 x - 40. One line each. It covers 59 calls. Logic readers of screenX: encounter.c:605 suspend bounds (-160..480; with the fix the bounds match the wider screen, INFERRED as wanted), audio/sfx_control.c pan (3 calls, pan grows by 1.25, INFERRED harmless), cam_main.c:102 targetScreenCoords (only .y is read, src/7B440.c:211, CONFIRMED). The pra/tst reflection maps (6 calls) use screen x for a reflection test. (INFERRED: check on the row.)

Lines: helpers 4, projection 2, speech and bubble 7, bottom boxes and counters 9, battle wheel 2, action command 93 (token replace with two constants in src/battle/action_cmd.h). Total about 117 lines.

Wide-off risk: none by construction. Every helper returns its input when pm_wide is 0, and the projection term is under pm_wide_cam. The display list is the same, so wide-off shots are byte-equal. (INFERRED, checked per row.)

### B. Central anchor in the port by region

About 40 lines in 3ds/src/pm_gpu.c (px, py, rebuild_3d ui_ortho branch, scissor). It cannot know the class (section 2): it tears windows from their text, moves center menus and world icons, and fights the full-screen edge rule. It changes shared draw code, so rule 5 asks for byte-equal checks on every scene with 2D draws. Wide-off risk: low if gated by pm_wide. Wide-on risk: high. Rejected.

### C. Hybrid: port op G_PM_ANCHOR plus game tags

Port: about 25 lines (op define in include/gbi_custom.h, case in 3ds/src/pm_gbi.c, offsets in pm_gpu.c). Game: a tag per window ID in render_windows (src/windows.c:280) and per HUD element in hud_element.c, plus brackets in msg_draw.c. About 60 game lines. WIN_BTL_POPUP holds both 192 (BOTTOM) and 80 / 120 (CENTER) boxes (src/battle/popup_messages.c:1717, :1800, :1803), so a tag per window ID still needs the per-site test. Total about 85 lines plus the shared draw code risk of B. It saves lines only on the action command (93 token edits become one tag in the action command HUD create). Keep it as the upgrade path if A's action command row is too large.

### Test per class (Azahar, docs/handoff/test_catalog.md)

Each row: one wide shot that shows the subject (gate), and one wide-off shot of the same scenario byte-equal to the HEAD binary (shot_cmp.py, catalog line 133). The until-line must be a log event for the subject. msg.c, msg_draw.c, popup_messages.c and btl_states_menus.c have no pm_log today, so each row adds one debug log line at its subject (allowed: debug tools may stay).

| Class | Scenario | State | Subject and shot window |
|---|---|---|---|
| WORLD | new `battle_target_wide`: battle_start debug string + `wide` + padscript A to target select | new | target cursor over the Goomba. Until: new log line at target select (btl_states_menus.c near :5548). |
| TOP (speech box) | new `talk_wide`: preset toad + `wide` + padscript walk to a Toad, A | new | speech box at the top. Until: new log line when a printer opens (msg.c msg_printer_set_origin_pos :1528). Route found by the row with one discovery start. |
| BOTTOM | new `popup_wide`: preset toad + `wide` + padscript that opens the field item menu | new | description box at 186 + 20 and star piece / coin counter. Until: new log line at popup open (8a860_len_3f30.c near :368). |
| VIEW (wheel) | new `battle_wide`: battle_start debug string + `wide` | new | wheel menu next to Mario. battle_start shots 153 to 182 (verified) are the window. Until: new log line at the wheel open (btl_states_menus.c :604). |
| VIEW (action command) | `battle_target_wide` + A on the target | new | action command HUD at hudPosX 50 to 28, y 80 to 72. Until: new log line at action command start. |
| Wide off, byte-equal | `battle_start` (verified), `yard` (verified), `text_menu` (verified) | existing | top shots byte-equal to HEAD |

New scenarios go into 3ds/tools/azt_scenarios.json and the catalog table in the same commit (rule 5).

## 5. Recommended rows

Each row: at most 60 changed lines, one gate, wide-off byte-equal on the scenario of that row.

| Row | Name | Change | Lines | Gate (wide on) | Wide-off check |
|---|---|---|---|---|---|
| UA1 | UI anchor 1, world projection | pm_wide.h helpers (4), get_screen_coords x fix (cam_main.c:522), wipe center fix (screen_overlays.c:428), target-select log line, scenario battle_target_wide | about 15 | target cursor sits on the Goomba (shot) | battle_start top shots byte-equal |
| UA2 | UI anchor 2, speech box top | msg_draw.c:421 and :427 with pm_wide_top, clamped at row 0 (13 - 20 = -7 clips 7 rows; the row starts at 0 and reads the shot), bubble clamps :493-504, msg.c:1543-1548 bottom 240, printer-open log line, scenario talk_wide | about 15 | box top edge at row 0 to 2, arrow inside the box | yard byte-equal (no dialog there: also run talk_wide without `wide` against HEAD) |
| UA3 | UI anchor 3, bottom boxes | popup_messages.c :1041 :1800 :2067, level_up.c :1163 :1167, item_entity.c :2593, shops.c :974 :976, 8a860 :376 :379-380, inventory.c :1231, popup log line, scenario popup_wide | about 20 | description box bottom edge at row 238 to 239, counter at bottom-left | popup_wide without `wide` byte-equal to HEAD |
| UA4 | UI anchor 4, battle wheel | btl_states_menus.c :604-605 with pm_wide_vx / vy, wheel log line, scenario battle_wide | about 8 | wheel next to Mario at about (33, 184) | battle_start byte-equal |
| UA5 | UI anchor 5, action command (part 1) | constants ACMD_HUD_X / ACMD_HUD_X0 / ACMD_HUD_Y in src/battle/action_cmd.h, replace in the first 12 of 23 files | about 50 | Jump action command HUD at (28, 72) | battle_start byte-equal |
| UA6 | UI anchor 6, action command (part 2) | the other 11 files | about 45 | one action command of the part-2 files | battle_start byte-equal |

Order: UA1 first (it fixes every world-anchored icon with 2 lines). UA2 is the reported symptom. UA5 and UA6 are the largest and the class (VIEW versus LEFT) must be confirmed by the UA4 shot before they start.

Open items:

- UA2 value: -7 (true view-relative) clips the top 7 rows of the box frame. The plan starts at 0. The user may prefer another margin.
- Moves and strats menus (btl_states_menus.c :1218, :2106) stay CENTER until a battle_wide shot shows them apart from Mario.
- The pra and tst reflection maps read get_screen_coords x for a test (6 calls). Check one map after UA1 only if the user reports a reflection fault.
