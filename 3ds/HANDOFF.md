G01R done: golden.py deletes the old shot before each run and refuses a duplicate reference. toad and yard re-saved.
Next: run golden toad yard once on the next build to confirm ok.
F07-1 done: pm_wide in pm_gpu.c (switch wide or options.txt wide=1). Stretch only, aspect and UI come in F07-2 to F07-5. Frame grab D03 and grab strips still use 40 + x (row F07-5).
F07-2 done: 3D aspect x1.25 (CAM_DEFAULT, CAM_BATTLE) and cull +-1.25 with wide. Toad golden 0 diff. Not tested: walk pop-in, mac_00.
F07-3 done: with wide, 2D and orthographic UI draw 1:1 centered (pm_gpu.c ui_ortho, px), fills and scissors at the game edge reach the screen edge, anchors in include/pm_wide.h used in inventory, hud_element, msg_draw, draw_box, screen_render_util.
F07-3 not tested: pause menu, text box, battle menu in wide. pause_map and pause_partners clamps stay (content is centered). Background tiling is F07-4. 3dsx +1140 bytes.
F07-4 done: with wide the sky tiles over -40 to 360 (background.c bg_texrect), side fills skipped for CAM_DEFAULT and CAM_BATTLE (background_gfx.c), curtain frustum x1.25 (curtains.c). Toad golden 0 diff.
F07-4 not tested: wave sky, battle sky, mac_00, flo_10 (the tour stopped in the intro at the 25 s smoke limit). 3dsx +40 bytes.
F06-1 done: 3ds/src/pm_interp.c records every guMtxF2L as a leaf (start in __wrap_step_game_loop, stop in nuGfxTaskStart), switch fps60 / pm_opt.fps60. Leaf peak 297 of 4096. Toad golden 0 diff. 3dsx +908 bytes.
F06-2 code done, NOT verified (3 Azahar starts used): fps60 passes in pm_nusys.c (t = 0.5 in the draw iteration, key frame replay in the idle iteration), pm_interp_lerp, load_mtx lookup, nodes in main_loop.c and cam_main.c. Tables are on the heap (a BSS growth put gDisplayContext past is_host, 0x01400000).
F06-2 open: golden toad switch off after the heap fix, fps60 smoke (25 s limit hit, game runs slower with two draws), pacestat, true leaf peak (F06-1 peak 297 missed gfx_draw_frame: the background task stopped the record). See row F06-2v.
F06-2v: golden toad off 0.00, shots no holes, leaf peak 49 (297 in F06-1), pace 11 to 14 steps/s with fps60 on Azahar (draw cost, F06-5). Rows F06-2 and F06-2v stay open.
F06-2v: tables resized to 384 leaves and 32 nodes and open/close fixed, built but not run (budget used). Rerun toad fps60 smoke once to check.
F06-3 done: node hooks in model, entity, animator, model_api, sprite, imgfx, item_entity, worker, effects (header include/pm_interp_fi.h), `f06 pair` log. Tables 1024 leaves and 1024 nodes (peak 50 leaves, 440 nodes). fps60 smoke ok, pair 98 to 100, toad golden off 0.00. 3dsx +1480 bytes.
F06-3 not checked: pair in battle and during a map change, tour stopped at the 200 s smoke cap. F06-2 and F06-2v moved to SLICES_DONE (pace stays with F06-5).
F06-4 done: nodes in msg, hud_element, peach_star_beam, snowfall (wrapEpoch), flower_trail, flower_splash. Screen anchor (pm_anchor_* in hud_element.c, used by shops and draw_number) and camera epoch (pm_interp_dont_camera). Battle fps60 shots steady, golden toad off 0.00, 3dsx +1920.
F06-4 not checked: pause1 text box, map change smear, shop anchor, star beam, snowfall, flowers (budget used).
V16V17: V21 fixed (fire dlist fall-through, CMake EFFECT_GFX_SRC), shot checked. Golden battle not run.
V16 and V17 stay open with leads in SLICES.md (f2070 box is the tape graphic, no battle flicker frame found, partial fade open).
V22: scan of src/ for dlists and Vtx arrays read past their end. 5 effect gfx files added to EFFECT_GFX_SRC. screen_overlays D_8014EA48 unsure (docs/plans/dlist_noend.md). Not run in Azahar.
