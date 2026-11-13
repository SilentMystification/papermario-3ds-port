# Design: state-call idioms of the root src files (ws_C, oracle 2026-11-13, main b2442aa97)

## Facts
- 899 state calls sit in root src/*.c (FACT, codemod --dry src at b2442aa97: hud_element.c 112, msg_draw.c 104, imgfx.c 87,
  item_entity.c 73, model.c 71, draw_box.c 56, screen_render_util.c 49, background_gfx.c 47, state_title_screen.c 31, 21 more files).
  The count includes calls in `#ifndef TARGET_3DS` blocks; N64DEL3 deletes those (GUESS: about 150 of the 899).
- Every state macro with packet gMainGfxPos++ records typed units only: nd_gbi.h:18 ND_GX -> nd_rec.c:442 nd_gx_w -> :459
  nd_rec_state (one unit), :454-455 and :450-451 (texture rectangle -> nd_rec_texrect :253), :457 (syncs dropped), :448 (a top-level
  G_ENDDL dropped). gMainGfxPos is reset at :447, so a macro moves gMainGfxPos by 0 and nd_rec_pos as nd_rec_state does.
- No consumer of a stated NdPsoDesc exists at the call sites (FACT: no ND_PSO_DESC in src). The native emit from NdPsoOut is a later
  core (plan_endgame.md section 6, CONV2D2 note). So an end-form desc cannot replace a state call byte-exactly today.
## The idiom (one per call shape): a native setter with the macro's arguments, minus the packet
Each setter records the same unit(s) as the macro: `nd_rec_state(ND_X0(w0, w1), ND_X1(w0, w1))` with w0, w1 from the gbi.h formula
(the nd_gbi.h ND_W0_* helpers). The stream is byte-equal by construction. The HUNIT harness proves it per shape on the host. The
call sites take their final names now. Later rows change only the setter bodies (S9DEL3) and fold runs into ND_PSO_DESC (NPSO5 form).
| shape (root count, FACT grep src/*.c) | native setter (new 3ds/include/nd_st.h, 3ds/src/nd_st.c) | units |
|---|---|---|
| gDPSetCombineMode 106, gDPSetCombineLERP | `#define nd_pso_cc(a, b) nd_pso_comb(ND_CC_W0(a, b), ND_CC_W1(a, b))`, ND_CC_W0(...) = ND_W0_CMB(__VA_ARGS__) (nd_gbi.h:36) | 1 COMB |
| gDPSetRenderMode 85 (gbi.h:2995), gDPSetCycleType 32 (:2919), gDPSetTextureFilter 28 (:2944), TexturePersp 21, TextureLUT 27, ColorDither 17, AlphaCompare 17, AlphaDither 12, TextureLOD 11, TextureDetail 9, TextureConvert 6, CombineKey 6, gSPSetOtherMode 9 | nd_pso_rm(c0, c1), nd_pso_cyc(t), nd_pso_filt(t), ... one per macro, same arguments; body = the macro's gSPSetOtherMode words (ND_W0_SOM, nd_gbi.h:20) | 1 OMH or OML |
| gSPSetGeometryMode 22, gSPClearGeometryMode 24 (gbi.h:2845-2847) | nd_pso_geom_set(w), nd_pso_geom_clr(w) (ND_W0_GEOM, nd_gbi.h:25) | 1 GEOM |
| gSPTexture 24 | nd_tex_on(s, t, level, tile, on) (ND_W0_TEX, nd_gbi.h:30) | 1 TEXTURE |
| gDPSetScissor 42 | nd_pso_scis(mode, ulx, uly, lrx, lry) (ND_W0_SCIS, nd_gbi.h:59) | 1 unit |
| gDPSetFogColor 10, gDPSetFillColor 9, gDPSetBlendColor | nd_val_fog, nd_val_fill, nd_val_blend in nd_val.c (value class) | 1 unit |
| gDPSetTextureImage 9, gDPSetTile 29, gDPLoadTile 8, gDPLoadBlock, gDPLoadTLUTCmd | nd_tex_img, nd_tex_tile, nd_tex_ldtile, nd_tex_ldblk, nd_tex_ldtlut (nd_gbi.h:95-125) | 1 each |
| gDPLoadTextureTile 36 (gbi.h:3884), _4b 26, gDPLoadTextureBlock 8, gDPLoadTLUT_pal16 20 (:4134) | nd_tex_load_tile(...), nd_tex_load_tile_4b, nd_tex_load_block, nd_tex_load_tlut16: body calls the base setters above in the gbi.h order; the syncs of the macro are not recorded (nd_gx_w :457) | n units |
| gSPTextureRectangle 32, gSPScisTextureRectangle 11, gDPFillRectangle 17 | nd_rect_tex(...) -> nd_rec_texrect(w0, w1, hs, hd) as nd_gx_w :454-455; nd_rect_scis(...); nd_rect_fill(...) (1 RECT unit) | NDC_TEXRECT or 1 |
| gSPViewport 20, gSPFogPosition 8 | nd_xf_viewport(v), nd_pso_fogpos(min, max) in nd_xf.c / nd_st.c | 1 unit |
| gDPLoadSync 9, gDPTileSync, gDPFullSync, top-level gSPEndDisplayList 7 | removed (as gDPPipeSync: nothing is recorded) | 0 |
| keep: gDPSetColorImage, gDPSetDepthImage 24 (NTGT core), gSPVertex 24, gSP1Triangle, gSP2Triangles 17 (NXF2 nd_mesh), gSPSetLights1 8 (lights core), gSPDisplayList 73 (ws_B entry form) | no setter now | - |
## Readers and writers of the recorded units (none changes: the units are identical)
- Writers: nd_gbi.h:18-19 (ND_GX, ND_GS), nd_op.h:58-118 ND_X0/ND_X1 (mirror nd_op.c nd_op_xlate), nd_rec.c:366 nd_rec_state,
  :351 rec_op_t, :75 sh_drop (shadow), :327 st_put, :253 nd_rec_texrect. New: nd_st.c (calls nd_rec_state and nd_rec_texrect only).
- Readers: nd_rec.c:578 stream_run -> nd_walk.c:363 nd_walk_state; nd_rec.c:577 -> nd_texrect_run; nd_model.c:262-299 nd_r11_op and
  nd_r11_walk (custom gfx ranges); nd_recs.c:254-262 POND; src/msg_draw.c:354 (gMainGfxPos and nd_rec_pos compare: both unchanged).
- Counters: C1 and C2 fall by 1 per rewritten call. C8 (listcen.py:48 READER_RX) rises by the NDO_ and G_ words of nd_st.c (GUESS 40).
## Order of edits (ws_C groups after N64DEL2-4)
1. nd_st.h, nd_st.c and 3ds/tools/nd_st_test.c (copy of the HUNIT harness nd_val_test.c): one case per setter, the macro on
   gMainGfxPos++ vs the setter, stream words, walker unit log and nd_r11_state equal; seeded fault (one wrong shift) must fail.
2. codemod.py: a `--state` option adds the setter names to MAP (same argument rule: first argument gMainGfxPos++, whole statement).
   nd_pso_cc keeps both arguments as tokens. One new case pair per name in 3ds/tests/codemod/cases. ws_A never passes --state.
3. Root files in groups, smallest first: draw_img_util.c, state_logos.c, windows.c, curtains.c, status_icons.c, effects.c, 43F0.c,
   8a860_len_3f30.c, main.c (group R1); background.c, background_gfx.c, screen_overlays.c, screen_render_util.c (R2); draw_box.c,
   msg.c, msg_draw.c (R3); hud_element.c, item_entity.c (R4); imgfx.c, sprite.c, sprite_shading.c (R5); model.c, entity_model.c,
   animator.c, cam_main.c (R6). Gate per group: nd_host, the codemod tests, 1 build, the exact listcen drop, 1-2 drec cmp runs.
OPEN: drec scenes that cover R3-R6 sites (COVMAP lists them per row); NXF2, NTGT and the lights core stay separate design cores.
