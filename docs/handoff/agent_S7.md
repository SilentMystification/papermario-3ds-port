# S7: WBO-1 site list (custom gfx builders in src/world and src/battle)

STATE (WBO-1 done, read-only, no code, no Azahar run)
- Scope: src/world and src/battle, grep gMainGfxPos, gSPEndDisplayList, gDPSetColorImage, gSPDisplayList, Gfx* and Gfx name[] side buffers.
- Result: 54 files, 57 function sites. World 44 files, battle 10 files. src/world/dead/ excluded (not in any 3ds/*.cmake).
- Kinds: 52 main-list words, 24 gSPDisplayList to side list (D3: nd_dlc ND_CALL), 5 side buffer, 4 colour image, 1 gSPEndDisplayList (crystal_ball_gfx.c).
- Colour image sites: 16C8E0 tattle_cam_pre_render, flo_10 waterfx, hos_05 intro, mac_01 crystal_ball_gfx.
- Build: battle.cmake GLOBs all src/battle/*.c (lang variants _jp/_fr/... excluded). .inc.c files build via #include of the parent named below.
- Function names: column-0 scan. "unnamed N() macro" = name is an N() macro, anchor by grep string.
- Scenario column: candidate scenarios whose text names the map code (azt_scenarios.json). Not proof that the site draws in that scene.
- Rows: WBO-2a to WBO-2e (world, by area), WBO-3 (battle, 10 files, one row). Grouping at the end.

## Sites

Format: file (build) | function (line of first hit) | grep anchor (first matching line in the function) | kind | scenario candidates

- battle/16C8E0.c (GLOB) | tattle_cam_pre_render (line 770) | `gDPPipeSync(gMainGfxPos++);` | colour image; main-list words | battle_start, battle_menu_bar, battle_danger (generic)
- battle/190B20.c (GLOB) | file-scope array (line 44) | `Gfx D_80293970[] = {` | side buffer | battle_start, battle_menu_bar, battle_danger (generic)
- battle/190B20.c (GLOB) | btl_appendGfx_prim_quad (line 3020) | `gDPPipeSync(gMainGfxPos++);` | main-list words; gSPDisplayList to side list | battle_start, battle_menu_bar, battle_danger (generic)
- battle/area/kzn2/actor/lava_piranha.c (GLOB) | unnamed N() macro (line 453) | `gDPPipeSync(gMainGfxPos++);` | main-list words; gSPDisplayList to side list | battle_start, battle_menu_bar, battle_danger (generic)
- battle/btl_states_menus.c (GLOB) | btl_menu_moves_draw_content (line 1711) | `gMainGfxPos++, G_SC_NON_INTERLACE,` | main-list words | battle_start, battle_menu_bar, battle_danger (generic)
- battle/btl_states_menus.c (GLOB) | btl_menu_strats_draw_content (line 2352) | `gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, xPos, yPos, x + STRATS_SUBMENU_WIDTH, var` | main-list words | battle_start, battle_menu_bar, battle_danger (generic)
- battle/common/stage/area_hos/hos_01.inc.c (via #include in battle/area/hos/stage/hos_01.c) | unnamed N() macro (line 29) | `gSPDisplayList(gMainGfxPos++, mdl_get_copied_gfx(index));` | main-list words; gSPDisplayList to side list | battle_start, battle_menu_bar, battle_danger (generic)
- battle/common/stage/lib/Snowflakes.inc.c (via #include in battle/common/stage/area_sam/sam_01.inc.c) | unnamed N() macro (line 27) | `gSPDisplayList(gMainGfxPos++, mdl_get_copied_gfx(index));` | main-list words; gSPDisplayList to side list | battle_start, battle_menu_bar, battle_danger (generic)
- battle/entity_model_icons.c (GLOB) | file-scope array (line 20) | `Gfx BtlCursorModelGfx[] = {` | side buffer | battle_start, battle_menu_bar, battle_danger (generic)
- battle/move/item/mystery.c (GLOB) | unnamed N() macro (line 93) | `gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, 108, 90, 139, 118);` | main-list words | battle_start, battle_menu_bar, battle_danger (generic)
- battle/move/item/strange_cake.c (GLOB) | unnamed N() macro (line 57) | `gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, 108, 90, 139, 118);` | main-list words | battle_start, battle_menu_bar, battle_danger (generic)
- battle/popup_messages.c (GLOB) | btl_bonk_setup_gfx (line 564) | `gDPSetPrimColor(gMainGfxPos++, 0, 0, 0, 0, 0, (alphaAmt * 255) / 10);` | main-list words | battle_start, battle_menu_bar, battle_danger (generic)
- world/area_dro/dro_02/npc_merlee.c (via #include in world/area_dro/dro_02/npc.c) | unnamed N() macro (line 221) | `gSPDisplayList(gMainGfxPos++, N(card_setup_gfx));` | main-list words; gSPDisplayList to side list | cand: merlee_spell
- world/area_end/common/Credits.inc.c (via #include in world/area_end/end_00/credits.c) | unnamed N() macro (line 130) | `gSPMatrix(gMainGfxPos++, OS_PHYSICAL_TO_K0(&gDisplayContext->matrixStack[gMatrixListPos++]` | main-list words | none (shared file)
- world/area_end/end_01/opera_float.c (build) | unnamed N() macro (line 114) | `gDPSetCombineMode(gMainGfxPos++, PM_CC_3F, PM_CC_3F);` | main-list words | none
- world/area_flo/flo_10/waterfx.c (build) | unnamed N() macro (line 71) | `gDPSetPrimColor(gMainGfxPos++, 0, 0, 255, 255, 255, alpha);` | colour image; main-list words; gSPDisplayList to side list | none
- world/area_flo/flo_14/bubbles.c (build) | unnamed N() macro (line 233) | `gSPMatrix(gMainGfxPos++, &gDisplayContext->matrixStack[gMatrixListPos++],` | main-list words; gSPDisplayList to side list | none
- world/area_hos/hos_00/shade_gfx.c (build) | unnamed N() macro (line 12) | `gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);` | main-list words | cand: tour10
- world/area_hos/hos_01/model_fx.c (build) | unnamed N() macro (line 38) | `gDPSetCombineMode(gMainGfxPos++, PM_CC_05, PM_CC_05);` | main-list words | cand: tex_hos_01, hos01_texword, texwrap_wall
- world/area_hos/hos_02/model_fx.c (build) | unnamed N() macro (line 13) | `gSPMatrix(gMainGfxPos++, &gDisplayContext->matrixStack[gMatrixListPos++],` | main-list words; gSPDisplayList to side list | none
- world/area_hos/hos_03/model_fx.c (build) | unnamed N() macro (line 18) | `gSPMatrix(gMainGfxPos++, &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_NOPUSH / G` | main-list words; gSPDisplayList to side list | none
- world/area_hos/hos_04/fountains.c (build) | unnamed N() macro (line 9) | `gSPMatrix(gMainGfxPos++, &gDisplayContext->matrixStack[gMatrixListPos++],` | main-list words | none
- world/area_hos/hos_05/intro.c (build) | unnamed N() macro (line 1362) | `gDPPipeSync(gMainGfxPos++);` | colour image; main-list words; gSPDisplayList to side list | none
- world/area_hos/hos_05/model_fx.c (build) | unnamed N() macro (line 56) | `gDPSetCombineMode(gMainGfxPos++, PM_CC_05, PM_CC_05);` | main-list words | none
- world/area_isk/isk_05/npc.c (build) | unnamed N() macro (line 29) | `gSPViewport(gMainGfxPos++, &cam->vp);` | main-list words | none
- world/area_isk/isk_13/npc.c (build) | unnamed N() macro (line 29) | `gSPViewport(gMainGfxPos++, &cam->vp);` | main-list words | none
- world/area_isk/isk_14/npc.c (build) | unnamed N() macro (line 29) | `gSPViewport(gMainGfxPos++, &cam->vp);` | main-list words | none
- world/area_iwa/iwa_01/splash.c (build) | unnamed N() macro (line 57) | `gDPSetPrimColor(gMainGfxPos++, 0, 0, 230, 255, 255, 255);` | main-list words; gSPDisplayList to side list | none
- world/area_jan/jan_13/puzzle.c (build) | unnamed N() macro (line 26) | `gSPMatrix(gMainGfxPos++, &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_NOPUSH / G` | main-list words | none
- world/area_kgr/kgr_01/wiggle.c (build) | unnamed N() macro (line 66) | `gSPDisplayList(gMainGfxPos++, mdl_get_copied_gfx(VTX_COPY_1));` | main-list words; gSPDisplayList to side list | none
- world/area_kgr/kgr_02/unused.c (build) | unnamed N() macro (line 51) | `gDPSetPrimColor(gMainGfxPos++, 0, 0, 255, 255, 255, 255);` | main-list words | none
- world/area_kkj/kkj_14/interactables.c (build) | unnamed N() macro (line 213) | `gDPSetCycleType(gMainGfxPos++, G_CYC_2CYCLE);` | main-list words | none
- world/area_kkj/kkj_27/rotating_wall.c (build) | unnamed N() macro (line 29) | `gDPSetCombineMode(gMainGfxPos++, PM_CC_KKJ_SPILL_LIGHT, PM_CC_KKJ_SPILL_LIGHT);` | main-list words | none
- world/area_kmr/kmr_06/sticker_sign.c (build) | unnamed N() macro (line 32) | `gDPPipeSync(gMainGfxPos++);` | main-list words | cand: tex_kmr_06
- world/area_kmr/kmr_20/records.c (build) | unnamed N() macro (line 71) | `gSPDisplayList(gMainGfxPos++, N(records_screen_gfx));` | main-list words; gSPDisplayList to side list | cand: warpmenu_sub, yard, yard_shot
- world/area_kmr/kmr_21/main.c (build) | worker_render_title_image (line 45) | `gSPDisplayList(gMainGfxPos++, N(Gfx_TexSetup_TitleImage));` | main-list words; gSPDisplayList to side list | none
- world/area_kmr/kmr_23/npc.c (build) | func_80240DA4_9087D4 (line 457) | `gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, ulx, uly, lrx, lry);` | main-list words | none
- world/area_kmr/kmr_30/the_end.c (build) | unnamed N() macro (line 101) | `gDPSetCombineMode(gMainGfxPos++, PM_CC_05, PM_CC_05);` | main-list words | cand: tour_k30
- world/area_mac/mac_00/water_gfx.c (build) | unnamed N() macro (line 9) | `gDPSetDepthSource(gMainGfxPos++, G_ZS_PRIM);` | main-list words; gSPDisplayList to side list | cand: profmenu_row, bg_flicker_mac00, bg_edge_mac00
- world/area_mac/mac_01/crystal_ball_gfx.c (build) | unnamed N() macro (line 21) | `gSPLookAtX(gMainGfxPos++, &gDisplayContext->lookAt.l[0]);` | colour image; main-list words; gSPDisplayList to side list; gSPEndDisplayList | cand: stale2_walk_mac01, mac01_sp0, mac01_ndref200
- world/area_mac/mac_01/npc/flower_gate.inc.c (via #include in world/area_mac/mac_01/npc.c) | unnamed N() macro (line 200) | `gDPSetCombineMode(gMainGfxPos++, PM_CC_FLOWER_GATE_GLOW, PM_CC_FLOWER_GATE_GLOW);` | main-list words | cand: stale2_walk_mac01, mac01_sp0, mac01_ndref200
- world/area_mac/mac_01/npc/rowf_and_rhuff.inc.c (via #include in world/area_mac/mac_01/npc.c) | unnamed N() macro (line 81) | `gSPDisplayList(gMainGfxPos++, mdl_get_copied_gfx(VTX_COPY_0));` | main-list words; gSPDisplayList to side list | cand: stale2_walk_mac01, mac01_sp0, mac01_ndref200
- world/area_mac/mac_05/club64_sign.c (build) | unnamed N() macro (line 88) | `gSPDisplayList(gMainGfxPos++, mdl_get_copied_gfx(VTX_COPY_3));` | main-list words; gSPDisplayList to side list | none
- world/area_mgm/mgm_00/scoreboard.c (build) | unnamed N() macro (line 100) | `gSPDisplayList(gMainGfxPos++, N(Gfx_RecordDisplay_Init));` | main-list words; gSPDisplayList to side list | none
- world/area_mim/mim_04/hint.c (build) | unnamed N() macro (line 45) | `gDPSetCycleType(gMainGfxPos++, G_CYC_2CYCLE);` | main-list words | none
- world/area_mim/mim_07/hint.c (build) | unnamed N() macro (line 57) | `gDPSetCycleType(gMainGfxPos++, G_CYC_2CYCLE);` | main-list words | none
- world/area_nok/nok_04/npc.c (build) | unnamed N() macro (line 295) | `gSPDisplayList(gMainGfxPos++, N(ThreadGfx));` | main-list words; gSPDisplayList to side list | none
- world/area_omo/omo_02/barricade.c (build) | unnamed N() macro (line 40) | `void N(DetermineSphericalSize)(Gfx* displayList, s32* outDist) {` | side buffer | none
- world/area_pra/pra_29/bridge.c (build) | unnamed N() macro (line 39) | `gSPDisplayList(gMainGfxPos++, mdl_get_copied_gfx(index * 3));` | main-list words; gSPDisplayList to side list | none
- world/area_trd/trd_05/trap.c (build) | unnamed N() macro (line 25) | `gDPPipeSync(gMainGfxPos++);` | main-list words | none
- world/area_trd/trd_06/falling.c (build) | unnamed N() macro (line 28) | `gDPPipeSync(gMainGfxPos++);` | main-list words | none
- world/area_tst/tst_13/main.c (build) | file-scope array (line 3) | `void mdl_project_tex_coords(s32 modelID, Gfx* destGfx, Matrix4f destMtx, void* destVertice` | side buffer | none
- world/area_tst/tst_13/main.c (build) | unnamed N() macro (line 301) | `gDPPipeSync(gMainGfxPos++);` | main-list words; gSPDisplayList to side list | none
- world/common/atomic/WhaleAnim.inc.c (via #include in world/area_jan/jan_00/whale.c) | unnamed N() macro (line 114) | `gSPDisplayList(gMainGfxPos++, mdl_get_copied_gfx(VTX_COPY_1));` | main-list words; gSPDisplayList to side list | none (shared file)
- world/common/complete/KnockDownPlayer.inc.c (via #include in world/area_kkj/kkj_25/peach_free.c) | unnamed N() macro (line 14) | `gDPPipeSync(gMainGfxPos++);` | main-list words | none (shared file)
- world/common/lava_piranha/part2.inc.c (via #include in world/area_kmr/kmr_22/npc/lava_piranha.inc.c) | unnamed N() macro (line 198) | `gDPPipeSync(gMainGfxPos++);` | main-list words; gSPDisplayList to side list | none (shared file)
- world/common/util/GetFirstTriangleNormal.inc.c (via #include in world/area_mim/mim_01/hint.c) | unnamed N() macro (line 11) | `void N(GetFirstTriangleNormal)(Gfx* inGfx, f32* outNx, f32* outNy, f32* outNz) {` | side buffer | none (shared file)

## WBO-2 and WBO-3 rows (about 10 files each)

- WBO-2a (10 files): world/area_dro/dro_02/npc_merlee.c; world/area_end/common/Credits.inc.c, world/area_end/end_01/opera_float.c; world/area_flo/flo_10/waterfx.c, world/area_flo/flo_14/bubbles.c; world/area_hos/hos_00/shade_gfx.c, hos_01/model_fx.c, hos_02/model_fx.c, hos_03/model_fx.c, hos_04/fountains.c.
- WBO-2b (10 files): world/area_hos/hos_05/intro.c, hos_05/model_fx.c; world/area_isk/isk_05/npc.c, isk_13/npc.c, isk_14/npc.c; world/area_iwa/iwa_01/splash.c; world/area_jan/jan_13/puzzle.c, world/common/atomic/WhaleAnim.inc.c; world/area_kgr/kgr_01/wiggle.c, kgr_02/unused.c.
- WBO-2c (10 files): world/area_kkj/kkj_14/interactables.c, kkj_27/rotating_wall.c, world/common/complete/KnockDownPlayer.inc.c; world/area_kmr/kmr_06/sticker_sign.c, kmr_20/records.c, kmr_21/main.c, kmr_23/npc.c, kmr_30/the_end.c; world/common/lava_piranha/part2.inc.c; world/area_mgm/mgm_00/scoreboard.c.
- WBO-2d (10 files): world/area_mac/mac_00/water_gfx.c, mac_01/crystal_ball_gfx.c, mac_01/npc/flower_gate.inc.c, mac_01/npc/rowf_and_rhuff.inc.c, mac_05/club64_sign.c; world/area_mim/mim_04/hint.c, mim_07/hint.c, world/common/util/GetFirstTriangleNormal.inc.c; world/area_nok/nok_04/npc.c; world/area_omo/omo_02/barricade.c.
- WBO-2e (4 files): world/area_pra/pra_29/bridge.c; world/area_trd/trd_05/trap.c, trd_06/falling.c; world/area_tst/tst_13/main.c.
- WBO-3 (10 files, battle): battle/16C8E0.c, battle/190B20.c, battle/area/kzn2/actor/lava_piranha.c, battle/btl_states_menus.c, battle/common/stage/area_hos/hos_01.inc.c, battle/common/stage/lib/Snowflakes.inc.c, battle/entity_model_icons.c, battle/move/item/mystery.c, battle/move/item/strange_cake.c, battle/popup_messages.c.
