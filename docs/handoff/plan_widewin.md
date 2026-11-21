# plan_widewin.md: clip audit for Native, Widescreen (pm_wide) and Fullscreen (pm_stretch) (oracle NATIVE20; code read at main a79f8ce548)
Tags: FACT (code read, file:line), GUESS (estimate). Modes: Native pm_wide 0 stretch 0; Widescreen 1/0; Fullscreen 0/1 (pm_gpu.c:1880-1881, pm_options.c:307-310).
## 1. The clip path (FACT)
- Game: nd_pso_scis (nd_st.c:72) -> ND_FS_SCISSOR unit -> ds_scissor sign rule (nd_ds.c:156-168, wide only) -> nd_sci_run (nd_set.c:84) ->
  pm_gpu_set_scissor (pm_gpu.c:4819-4834): raw edges to gsci_r (:4820), then a clamp to 0..320 x 0..240 and a swap into gsci_x0..y1.
- GPU: nd_native_submit (pm_gpu.c:1396), the one submit of every draw -> stage_scissor (:1198-1215). Wide ui (2D or ortho batch, :1202): x0 <= 0 -> 0,
  x1 >= 320 -> 400, else 40 + x. Wide 3D and every Fullscreen batch: x * 5 / 4. Native: 40 + x. Off-screen slots: scissor_off (:1080-1087), no mode.
- Placement: view matrix (pm_gpu.c:236-237): Fullscreen scales every view draw 1.25; Widescreen keeps an ortho draw 1:1 at 40 + x (wedge_x: an edge-only
  range 1.25, :3713-3716). Rectangles (texture rects, fills: text glyphs, HUD pieces, window patterns) use px = 40 + x (pm_gpu_rect.c:82, :161-162) and
  the fixed projection (pm_gpu.c:1926, pcomp RECT_GM :1071-1072). WCLIP2 (merged dfb703014f) moved Fullscreen rects to 1.25 x (pm_gpu_rect.c:83, :162-163). The user rejects it: section 6 (FSUI1 undoes it).
## 2. Root causes (ranked)
1. FACT: msg_draw.c:374 sets scissor 0, 0, 319, 239 (SCREEN_WIDTH - 1) for every message (flag 4: msg_draw.c:152, :189). Wide ui: x1 319 < 320 gives
   column 359 (pm_gpu.c:1205). A speech or tattle bubble reaches game x 342 (pm_wide_right(302), msg_draw.c:527-528) = column 382: columns 359..382 clip.
   Left side: x0 0 gives column 0: no clip. This is the user's tattle fault (right side only). Fullscreen: 319 * 5 / 4 = 398 (2 columns).
2. FACT: Fullscreen places rects at 40 + x and meshes and scissors at 1.25 x (section 1). A window scissor right of centre (x0 > 160) cuts the left
   0.25 x0 - 40 columns of its rect content; left of centre (x1 < 160) cuts the right 40 - 0.25 x1. The bubble mesh stretches, its text does not
   (text clip msg_draw.c:596-600 is native in Fullscreen). GUESS: the user's Fullscreen text box fault.
3. FACT: the clamp at pm_gpu.c:4821-4824 drops window edges in the wide margins. A popup or battle list at pm_wide_left(x) (8a860_len_3f30.c:488,
   btl_states_menus.c:1271, :2161) with x0 < 0 maps to column 0, not 40 + x0; x1 in 321..359 maps to 400. Content past the window edge is not cut.
4. FACT: status_icons.c:606 scissor 12, 20, 308, 220 in every mode. Wide: columns 52..348 cut actor status icons in the wide margins (GUESS: battle
   enemies near the edges). HUD sites use pm_wide ? full : safe area (hud_element.c:1994, :2058, :2110, :2183, inventory.c:752): no clip in wide.
5. FACT: battle/16C8E0.c:784 (tattle camera COPY background, viewport - 1): a full viewport gives x1 319 -> column 359 in wide (2D fill). :850 is the 3D fix (TATTLEAR).
- Not new: no scissor code changed since f0a9f642e4 (git diff: pm_gpu.c scissor lines, nd_set.c, nd_ds.c, msg_draw.c, gbi.h, pm_gpu_rect.c unchanged). The
  conversion is F07-3 (989f73581c), the bubble wide clamp UA2 (411b4f146c), Fullscreen MENUGEN (55b3803ff8). NC0S, NC0V, GD, DO, MCOPY, PIPE: no change.
## 3. Class table (scissor the game sets; W = Widescreen result; F = Fullscreen result)
| class | scissor site | W | F |
|---|---|---|---|
| speech, tattle bubble, arrow, choice | msg_draw.c:374 0..319 | right clip at 359 FACT (cause 1) | 398; text 1:1 in a 1.25 bubble FACT (cause 2) |
| fixed message styles (sign, postcard, inspect) | same; clip msg_draw.c:742-797 | inside 0..319 GUESS: no clip | rect vs mesh offset FACT |
| status bar, HUD elements, coin, item icons | inventory.c:752, hud_element.c:1994-2183 | full FACT | safe area 15..385, rects 52..348 FACT: no cut |
| actor status icons | status_icons.c:606 12..308 | cut at 52/348 FACT (cause 4) | no cut FACT |
| popup menus, battle menus, desc boxes | 8a860:1463-1741, btl_states_menus.c:1754, :2398 | left edge over-extends FACT (cause 3) | window cut FACT (cause 2) |
| pause menu pages | pause_*.c x1..x2, pause_main.c:516, :597 | 40 + x FACT; 4:3 layout GUESS: no cut | window cut FACT (cause 2) |
| file select, name entry | filemenu_createfile.c:208-288 | pm_wide 0 in FILE_SELECT (game_modes.c:281) FACT | window cut FACT (cause 2) |
| item roulette | mystery.c:75, strange_cake.c:58 108..139 | correct FACT | 135..173 vs 148..179: right 6 columns cut FACT |
| tattle camera window | 16C8E0.c:784, :824, :850 | 2D fill cut at 359 if full FACT (cause 5) | 398 |
| 3D world, effects, shadows | model.c:3970, effects.c:178, sprite_shading.c:408, flame.c:193, partner_buff.c:287 | 0..400 FACT | 0..400 FACT |
| backgrounds, overlays, wipes, title, intro | background_gfx.c:316, :351, screen_overlays.c:166, :175, screen_render_util.c:209, :274, main.c:155 | full FACT | full FACT |
| draw_box windows | draw_box.c:456 full; :764-775 off-screen slot | full FACT | full FACT |
| damage numbers, action bars, HP/FP, star bar, enemy HP | HUD element sites or the last scissor GUESS | census decides | census decides |
- Other wide reads (not clips, OPEN): depth and frame reads at column 40 + x (pm_gpu.c:2242, :4857) ignore the 1.25 of 3D in wide and Fullscreen.
## 4. Scissor rule (WCLIP1, merged; FSUI1 extends the ui side to Fullscreen)
- A scissor equal to the full native screen (0, 0, 320 or 319, 240 or 239) or its safe area (12, 20, 308, 220) is the full screen in W and F.
- Else each side keeps its own place: an edge at game x 0 or 319/320 is the screen edge; any other edge maps 40 + raw x (ui: 2D or ortho batch) or 1.25 x (3D).
- Native is unchanged (byte-equal). After FSUI1 the W rule and the F rule are the same (nd_sci.c:17 `s = ui`).
## 5. Rows
| row | model | what | gate |
|---|---|---|---|
| WCLIP0, WCLIP1 | Haiku | clip census, nd_sci.c rule (merged) | done |
| WCLIP2 | Haiku | Fullscreen rects at 1.25 x (merged dfb703014f, rejected by the user 21 Nov) | FSUI1 undoes it |
| FSUI1 | Haiku | pm_wout output flag, undo WCLIP2, Native picture in the 4:3 game modes, marker fsui | msgw_wide_drec byte-equal, pause_full_drec expected-diff |
| FSUI2 | Haiku | game reads: pm_wout_cam (3D to 2D x), screen bounds, TATTLEAR, box frustum, 3 effects, PM_SCIS | host tattlear ok 9, msgbox_talk_nowide byte-equal, msgw_full_drec expected-diff |
| FSUI3 | Haiku | scene background bracket G_PM_ND_SCENE2D: sky and fills at 1.25 x in Fullscreen | msgbox_talk_nowide byte-equal, msgw_full_drec expected-diff, scn2d opens >= 1 |
Order: FSUI1, then FSUI2 and FSUI3 (no shared file, both start on the FSUI1 gate commit). Model Haiku: closed edits with before and after text.
## 6. Fullscreen UI at Native size (user order 21 Nov; oracle code read at main dfb703014f)
User (verbatim): "I dont like how the start menu was stretched, you can keep the original aspect ratio, also the text boxes should stay the same size,
not stretch, its just they should render properly outside of native resolution". Fullscreen = pm_wide 0, pm_stretch 1 (pm_gpu.c:1942, game_modes.c:284).
- Mechanism: Fullscreen takes the Widescreen output path. Widescreen already draws 3D at 1.25 x and 2D and ortho 1:1 at 40 + x (rebuild_3d pm_gpu.c:238-239,
  ui_ortho :223, wedge_x and wide_edge_k :3561-3620 for draws on both game edges, stage_scissor :1259). Today pm_stretch overrides it: :238-239 scale every
  draw, nd_sci.c:17 maps every F edge 1.25 x, pm_gpu_rect.c:83 and :162-163 (WCLIP2) move rects. FSUI1 adds `pm_wout = pm_wide || pm_stretch` and reads it at
  each GPU output site that read pm_wide (15 sites, ws_FSUI1.md). Discriminator: ui_ortho (G_MTX orthographic, pm_gpu.c:3455) and `!nb.draw_3d` (rects).
- What differs from Widescreen: pm_wide stays 0, so the game keeps the Native layout (pm_wide_left/right, wide boxes) and the 4:3 field of view
  (cam_main.c:85, cam_math.c:398). The 3D picture is the Native view stretched 1.25 x (viewport 12..308 at columns 15..385). Widescreen shows more world.
- Check by class (FACT code read): start menu = title and file select: game_modes.c:281 list, FSUI1 gives them pm_stretch 0 (exact Native, as Widescreen).
  Pause menu: draw_box windows and rects (2D or ortho: ui side); its rotating windows use a perspective frustum (draw_box.c:469, UA7): FSUI2. Pause picture:
  the grab slot holds 400 columns (pm_gpu.c:2787); FSUI1 gives F the wide read shift (:2401, pm_main.c:271, nd_tex_set.c:221) and the gutters (background_gfx.c:374).
  Speech and tattle bubbles: ortho mesh plus glyph rects (ui side), scissor msg_draw.c:374 is full (nd_sci_full); the bubble anchor comes from get_screen_coords:
  FSUI2 pm_wout_cam gives 1.25 x - 40 (cam_main.c:513). HUD, status bar, popups, battle menus: 2D or ortho (CAM_HUD ortho hud_element.c:2105): ui side.
- Scene parts drawn as 2D: the background (sky texrects, background colour fill, side bars, background_gfx.c:376-479) must follow the 3D: FSUI3 bracket.
  Tattle camera (cam_main.c:478, 16C8E0.c:849), circle wipe (screen_render_util.c:228-265), bulb glow, star spirit flash, underwater grid: FSUI2 (as Widescreen).
- Defaults taken (no user decision): title, file select, intro and chapter screen stay Native in Fullscreen (the user asked for the original aspect ratio).
  3D-space effects (damage numbers, emotes) stay stretched with the 3D scene. A live options flip keeps the mode until the next mode change (as Widescreen).
- OPEN: depth and frame reads at column 40 + x (pm_gpu.c:2304, :4927) ignore the 1.25 x of 3D in W and F (unchanged by FSUI).
