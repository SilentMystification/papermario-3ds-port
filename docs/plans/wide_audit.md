# Wide audit (PT58): 4:3 assumptions

Status words: FIXED (commit in this slice), OK (reads the wide value already), OPEN (not fixed), NOT TESTED (code read only).
Wide switch: `wide` or the Options row. `pm_wide` is 0 in logos, title and file select (4:3 by design).

| item | file:line | symptom | fix size | status |
| --- | --- | --- | --- | --- |
| Standard game view 12 20 296 200 | src/cam_main.c set_cam_viewport | black bars on all four sides, world stops 15 px from the sides | 15 lines | FIXED (PT53, PT54) |
| Vertical field of view and aspect | src/cam_main.c:71, src/cam_math.c:397 | the view was 296 x 200 stretched | 3 lines | FIXED (PT53, PT54) |
| Camera lead ratio | src/cam_math.c:525 | lead amount would change with the new ratio | 1 line | FIXED (old 296 / 200 kept) |
| Sky image 296 x 200 | src/background.c bg texture, src/background_gfx.c fill | sky missing in rows 0 to 19 and 220 to 239 | 20 lines | FIXED (first and last image row continue) |
| Wave sky (gBackroundWaveEnabled) | src/background.c wave branch | rows above and below are not extended | 20 lines | OPEN (rare, no map checked) |
| Pause background | src/background_gfx.c, 3ds/src/pm_gpu.c pm_gpu_read_frame | saved picture 320 wide, sides stretched | 40 lines | FIXED (PT56). The pause blur is not applied to the gutters. |
| Status bar groups | src/inventory.c status bar draw | HP, FP left, star points, coins right | 12 lines | FIXED (PT55, wide only) |
| Status bar and HUD scissor y 20 to 220 | src/inventory.c:748, src/hud_element.c:1880 | bar top and bottom rows cut off in wide | 2 lines | FIXED (full 0 to 240 with wide) |
| Message bubble origin clamp 0 to 320 | src/msg.c:1527 | an NPC in the gutter moves its bubble to the screen edge | 8 lines | FIXED (clamp -40 to 360 with wide) |
| Message window centre 160 | src/msg_draw.c, src/draw_box.c | windows stay centred in the middle of the wide picture | 0 | OK (intended) |
| Fades, screen overlays, fill rects | src/screen_overlays.c:220, :236 | fill 0 to 319 | 0 | OK (the port extends a rect on the game edge to the screen edge, F07-7). NOT TESTED with a shot of a fade in this slice. |
| Battle swirl, circle transition | src/screen_render_util.c:233, :268 | rect reaches both edges | 0 | OK (F07-3, F07-6). NOT TESTED in this slice. |
| Curtains, intro | src/curtains.c:257 | frustum scaled 1.25 | 0 | OK (F07-4) |
| Cutscene viewports 29 28 262 162 and 15 28 290 128 | src/world/world.c:232, end_00 and end_01 main.c, dig battle scripts | bars stay (pillar box, scaled by 1.25) | 0 | OK (intended cinematic frame) |
| Tattle view | src/battle/16C8E0.c:777 to :856 | uses the camera viewport, fill 0 to 319 | 0 | NOT TESTED |
| Sprite shading scissor | src/sprite_shading.c:407 | follows the viewport | 0 | OK |
| Entity and NPC culling | src/entity.c:343 (distance only), src/model.c:2835 (wsCullX 1.25) | no screen frustum test for entities. The model test has 25 percent extra. | 0 | OK. wsCullX could be 1.0 now (cost only). |
| Screen bounds helpers | src/cam_main.c:500 to :523 (get_screen_coords users, is_point_visible) | follow the viewport and the new matrix | 0 | OK |
| Message pan 160 | src/msg.c:286 | speech sound pan uses the 320 centre | 1 line | OPEN (no visible effect) |
| Title screen, file select, logos | src/state_title_screen.c, src/filemenu | 4:3 by design | 0 | OK |
| Sun effect (PT57) | src/effects/sun.c | rays flicker in wide (user report) | ? | NOT REPRODUCED in Azahar (jan_03, 12 frame diff: only the Yoshi NPC in the gutter changes) |
| Minimap, weather | none found in game code | | | none |
