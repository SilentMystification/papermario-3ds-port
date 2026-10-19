G01R done: golden.py deletes the old shot before each run and refuses a duplicate reference. toad and yard re-saved.
Next: run golden toad yard once on the next build to confirm ok.
F07-1 done: pm_wide in pm_gpu.c (switch wide or options.txt wide=1). Stretch only, aspect and UI come in F07-2 to F07-5. Frame grab D03 and grab strips still use 40 + x (row F07-5).
F07-2 done: 3D aspect x1.25 (CAM_DEFAULT, CAM_BATTLE) and cull +-1.25 with wide. Toad golden 0 diff. Not tested: walk pop-in, mac_00.
F07-3 done: with wide, 2D and orthographic UI draw 1:1 centered (pm_gpu.c ui_ortho, px), fills and scissors at the game edge reach the screen edge, anchors in include/pm_wide.h used in inventory, hud_element, msg_draw, draw_box, screen_render_util.
F07-3 not tested: pause menu, text box, battle menu in wide. pause_map and pause_partners clamps stay (content is centered). Background tiling is F07-4. 3dsx +1140 bytes.
