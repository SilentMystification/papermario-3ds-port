# plan_entity_npc: entity model, NPC sprite and task tail ops in mac_01 (O3DS)
Plan only. Rules: hw_guidelines.md. Units: 2 us per dispatched op, 101 us per non-empty close (console). [E] estimate, [C] code read, [M] measured (diag_toadt tour f899).

## 1. What the producers draw (code read)
- mac_01 makes one entity only: Entity_SavePoint (src/world/area_mac/mac_01/entity.c:14). The other entity model draws are shadows. Each NPC and the player has one circle shadow: render_shadows (src/entity.c:450) calls draw_entity_model_A. About 29 of the 30 draws per frame are shadows [C, E]. A shadow is 2 tris.
- Census counts every dispatched op, also the ops in nested G_DL lists and ENDDL. It does not count G1 syncs or G2 equal skips (pm_gbi.c:3275-3302).
- Shadow draw (appendGfx_entity_model, renderMode SHADOW, FOG_DISABLED): MTX, G_DL Gfx_RM1_DECAL_XLU_NO_AA, GEOMETRYMODE, [callback entity_model_set_shadow_color: SETCOMBINE, PRIMCOLOR], SETCOMBINE PM_CC1_SHADOW, G_DL shape, POPMTX. RM list: OML, OMH, GEOMETRYMODE x2, ENDDL. Shape Entity_RenderCircularShadow (src/entity/Shadow.c): G_DL LoadTexCircle -> G_DL GfxCommon -> TEXTURE, SETCOMBINE, OMH x6, ENDDL; then OMH(LUT), SETTIMG, SETTILE, LOADTILE, SETTILE, SETTILESIZE, SETCOMBINE, ENDDL (texture load); then SETCOMBINE, GEOMETRYMODE, VTX, TRI2, ENDDL. About 37 ops emitted, about 26 dispatched after G2 skips. Matches 26.5 per draw [M]. Geometry: 2 ops. Matrix: 2. All others are state. SETCOMBINE with the same words runs 5 times per shadow.
- NPC sprite draw (appendGfx_npc emits no op; spr_appendGfx_component, src/sprite.c:322, flat path): MTX, G_DL OpaqueSpriteGfx -> GEOMETRYMODE, SETCOMBINE, TEXTURE, GEOMETRYMODE, OML, OMH, ENDDL; gDPLoadTLUT_pal16 -> SETTIMG, SETTILE, LOADTLUT; gDPScrollTextureBlock_4b -> SETTIMG, SETTILE, LOADBLOCK, SETTILE, SETTILESIZE; VTX, TRI2; POPMTX. 20 ops emitted, about 18 dispatched. Matches 18.1 per draw [M]. Geometry 2, matrix 2, texture load 8, state setup 6 to 8.
- Closes: shadows 0 (same circle texture, same state). NPC 0.07 per draw. Closes do not drop with any row below. The cost is op count.

## 2. Task tail 0xF0FE (owner)
- The mark is set after the last task in execute_render_tasks (src/model.c:4905). Every main-list op after it goes to 0xF0FE until the next frame.
- Code after that point (src/cam_main.c:243-268, src/main_loop.c:200-265): render_transformed_hud_elements, camera end ops, player_render_interact_prompts, render_screen_overlay_backUI, render_workers_backUI, render_hud_elements_backUI, render_effects_UI, render_game_mode_backUI (state_drawUI_world -> draw_status_ui, src/inventory.c:1275), render_window_root, render_frame(true) (CAM_HUD, item entities), render_messages, front UI, curtains.
- Warp view tail: 95 ops, 0 closes. Tour view: 332 ops, 19 closes [M]. A UI part that shows only in the tour is the likely owner: the status bar (draw_status_ui; one texture per icon or digit) [E, not measured]. Row T1 names it.
- Each of the 19 closes is a real texture change (HUD icons and digits). 19 x 101 us = about 1.9 ms console [E]. Only an atlas removes them. Not in this plan.

## 3. Rows
| row | change | file, function | ops/frame removed (tour; console scene A) | console ms saved [E] | risk | Azahar check | order |
|---|---|---|---|---|---|---|---|
| EN2 | Shadow texture setup as one G_PM_TEXI: under TARGET_3DS, flat copies of Entity_RenderCircularShadow and Entity_RenderSquareShadow. Word 0 = TEXI header with n (circle 18, square 17). Then the same words in the same order, then CC, CLEARGEOM, VTX, TRI2, ENDDL. Add gsSPPmTexInline(n) next to G_PM_TEXI in gbi_custom.h | src/entity/Shadow.c; include/gbi_custom.h | about 11 per shadow, 320 tour; under 30 scene A | 0.64 tour; under 0.06 scene A | low | mac01t: entity ops per draw 26.5 down to <= 16, closes 0, tris 146 same; PPM byte-equal both scenarios | 1 |
| NP1 | Sprite texture load as one G_PM_TEXI: header before gDPLoadTLUT_pal16 (sprite.c:235), set n after the load macro in each branch. Copy msg_draw.c:1758/1802 | src/sprite.c spr_appendGfx_component_flat | 7 per sprite, 189 tour; about 44 scene A | 0.38 tour; 0.09 scene A | low | mac01t: npc ops per draw 18.1 down to <= 12; PPM byte-equal; also yard and pause_menu | 2 |
| T1 | Name the tail: under TARGET_3DS and pm_ph_on, call pm_gbi_mdl_mark(0xF0E0 + k, 0) at each FI_OPEN section of gfx_draw_frame after render_frame(false), and after render_transformed_hud_elements in render_frame | src/main_loop.c gfx_draw_frame; src/cam_main.c render_frame | 0 | 0 | none | mac01t: 0xF0FE row near 0; 332 ops and 19 closes split over 0xF0E0+k rows; PPM byte-equal | 3 |
| EN1 | Entity RM list as G_PM_RM: save gMainGfxPos before the two RM switches; after them, if the saved word is G_DL, set its w0 to G_PM_RM<<24 | src/entity_model.c appendGfx_entity_model, lines 281-365 | 3 per draw, 90 tour | 0.18 tour | low | rmsplit n up by draws, RS_RMSK 0; PPM byte-equal | 4 |
| NP2 | Sprite setup list as G_PM_RM: rm_apply also accepts SETCOMBINE and TEXTURE and calls tex_op for them. sprite.c swaps the 4 setup gSPDisplayList calls to gSPPmRenderMode under TARGET_3DS | 3ds/src/pm_gbi.c rm_apply; src/sprite.c:362-375 | 5 per sprite, 135 tour; about 30 scene A | 0.27 tour; 0.06 scene A | medium | as NP1, plus tour10 and yard PPM byte-equal, rmsplit fb 0 | 5 |
| HT1 | HUD texture loads as G_PM_TEXI in hud_element_draw_rect. Gate: T1 shows the HUD owner with > 150 ops | src/hud_element.c | about 7 per element [E] | about 0.2 to 0.3 [E] | low | T1 rows down; closes same; PPM byte-equal | 6 |

## 4. Rows that do not pay
- Skip-if-equal state at model level (R1 to R3): repeats are rare [M agent_modelops].
- MTX skip: each shadow and each sprite has its own matrix.
- Drop the duplicate shadow SETCOMBINE: about 0.2 ms [E], handler order changes. Only if EN2 leaves shadows above 12 ops.
- Tail closes (19): real texture changes. Atlas only. High picture risk.
- USE_IMAGE entity path and animator.c RM swap (C1-3): mac_01 does not use them.
- Sort or batch shadows: no closes now.

## 5. Hardware
- TEXI runs words in tb_inline/tex_op: no prologue per word, sequential reads of one static list. No new tables, no division.
- T1 marks are cold and run only with the phase on.

## 6. First 3 rows for one Haiku code agent
1. EN2 (about 40 lines). 2. NP1 (about 10 lines). 3. T1 (about 15 lines).
- One Azahar start per row: azt mac01 -Try, then azt mac01t -Try. Take the mac01t baseline shots before EN2. Revert a row that changes one picture byte.
- Projected mac_01 [E]: tour view 2315 ops/frame down to about 1805, about -1.0 ms console. Console scene A has few shadow and sprite ops (sprite 114 ops/frame), so the saving there is 0.1 to 0.3 ms. Frame 60.6 ms goes to about 59.6 to 60.5 ms.
- These rows do not reach 33.4 ms. The scene A gap is appendGfx_model (1899 ops), texture closes (+5.3 ms) and the GPU draw side (diag_toadnpc). T1 names the owner of the 19 tail closes (about 1.9 ms) for the next plan.
