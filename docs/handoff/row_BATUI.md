# Row BATUI (waits on user facts): battle UI missing on the console

## User facts
- 2026-11-09 (O3DS, de415917f): "The battle UI is mostly missing right now. I see the icons for jump, item and strategies along with
  their text but the rest of the UI is not visible."
- 2026-11-09 (BATWIDE, now merged here): with Widescreen on, on Azahar and the console, the menu boxes are missing, the menu does not
  reach the left screen edge, the wheel light is not on the selected item.
- 2026-11-11: the battle UI is missing on a current console build (widescreen setting, menu and console not given).

## Trace (oracle code read, main 798ee92d9): how each part of the battle menu draws
- Text rectangles: option icons (btl_states_menus.c:1008 hud_element_draw_clipped) and the option name (:1016 draw_msg) -> nd_texrect
  (nd_rect.c:216) -> pm_gpu_tex_rect (pm_gpu_rect.c:267). Drop site: pm_gpu_rect.c:275 when tex_raw_rect refuses. These parts DO show.
- Triangles of transformed HUD elements (the parts that do NOT show): spotlights HID_Spotlights (:931-941), highlight (:950-959), beam
  (:968-975), reels A/B (:977-991): HES_ProjectorSpot/Beam/Reel are IA8 (battle_ui_gfx.c:522-545) -> render_complex_hud_element mode 2
  (hud_element.c:1569-1627): CAM_HUD ortho (hud_element.c:1811-1816), G_ZBUFFER|G_SHADE|G_LIGHTING (:1821), gSPVertex of
  transform->quadBuffers, 2 triangles. Port path: nd_geom_vtx -> gv_tri -> pm_gpu_gtrip. Since TRI-4 (c3e7afb19) there is no CPU
  triangle path: every refusal drops the triangle (nd_geom.c:291-293, counted in tri_drop and gv_fb). Drop arms that can hit these quads:
  (a) lit load without gv5 (nd_geom.c:321), (b) matrix table full: gv_matrix_id returns GV_NONE past 256 matrices per task
  (nd_mtx_set.c:73-74; a battle with many effects reaches the HUD last), (c) ring full (pm_gpu.c:3019 GRING_VTX), (d) gv_state_why LIT
  (nd_geom.c:164), (e) wide only: gtrip_wide refuses when wide_edge_k returns 1 or 2 (pm_gpu.c:2776-2786, :2752, :2761), (f) decal with
  blend (pm_gpu.c:2822), (g) a stale KZN04 dual_on scaling the quads' colour by a sprite shade (nd_geom.c:88, row WALLSHADE fixes it).
- Prim quads btl_draw_prim_quad(0,0,0,0, ...) (:1001, :1015) have alpha 0 (invisible on the N64 too): not a missing part.
- Wide only: BattleMenu_BasePosX = pm_wide_left(54) (:611) keeps the wheel 54 px from the screen edge, as 4:3 keeps it 54 px from the
  game edge: by design, not a fault; a wheel at the screen edge is a user decision (a smaller value).
- Status bar and sub-menu windows (draw_box): texrects (box_pattern_rect, nd_rect.c:105) and, for rotated windows, triangles (same
  drop arms as above).

## User facts 3 (2026-11-11, through the orchestrator)
Widescreen ON, O3DS XL.

## User facts 4 (2026-11-11): "it's the background circles in the HUD that don't render" (wide on, XL; icons and text draw)
The circles are HID_Spotlights[i] and HID_HighlightSpot (btl_states_menus.c:633, :646): HES_ProjectorSpot, IA8 32x32
(battle_ui_gfx.c:531), alpha 150 and 180 (TRANSPARENT: G_RM_CLD_SURF, hud_element.c:1613), combine PM_CC_47, lit quad (G_LIGHTING at
hud_element.c:1821: pm5 program, palette GPAL5 8 per batch, pm_gpu.c:132). Same path as the beam and reels (IA8 64x64), which differ
only in size and alpha. Checked and not the cause: the cell atlas (pm_tex.c:337/359 needs a TLUT: IA8 has none), the scissor
(pm_gpu.c:944-947 gives a ui batch the full 400 px), the KZN04/WALLSHADE leak (the IA8 load clears dual_on), the wide edge copy (the
spot vertices sit at game x >= 18, not on the game edge). Open fact (one question): with Widescreen OFF, do the circles show? Yes: the
cause is in the wide arms (rebuild_3d pm_gpu.c:176-177, gtrip_wide :2776-2786). No: the common lit IA8 quad path (pm5, nd_geom.c).

## Widescreen path of the triangle HUD elements (oracle code read; no arm found that drops all of them)
- Mapping: pm_gpu_set_mvp (pm_gpu.c:2604-2619) marks the CAM_HUD ortho as ui_ortho; rebuild_3d (pm_gpu.c:176-177) draws it 1:1 at
  screen x = 40 + game x (no 1.25 stretch), the same mapping as the texture rectangles (px, pm_gpu_rect.c:252): spots and icons share
  one x (BATWIDE run 0650a26c7: light x 97 = icon x 97). pm_gv_rejm 0x2c (pm_gpu.c:2612) keeps margin triangles (game x -40..0).
- Scissor: hud_element.c:1816 sets 0,0,320,240 in wide; the wide sign rule maps game x 0 to screen 0 (T_SCISSOR), so the margin is drawn.
- Wide refusal arms and their values for these quads: gtrip_wide (pm_gpu.c:2776-2786) refuses when wide_edge_k returns 1 or 2:
  r = 1 for a non-ortho matrix (the HUD modelview is a rotate/scale/translate: w column 0,0,0,1, wz true) or m[0][0] == 0 (the beam
  turns -43 degrees, the reel at most 45: m00 != 0) or no ring room (pm_gpu.c:2761); r = 2 for more than 192 indices (a quad has 6).
  A vertex past the game edge keeps its place (wide_on_edge :2715-2721, MSGBOX 4). None of these fires for every spot, beam and reel.
- Not wide-only and able to drop all late HUD triangles of a battle frame: (b) the 256-entry matrix table per task (nd_mtx_set.c:73-74:
  every part of every effect takes one entry; the HUD draws last) and (c) the ring (pm_gpu.c:3019). Their counts print as `tridrop`
  (nd_geom.c:256-261, gv_fb ring and refuse) every 300 frames in a debug push.
- The 2026-11-09 report predates KZN04; the WALLSHADE leak (g) cannot reach these quads (their IA8 load runs a tile 0 SETTILESIZE,
  nd_tex_set.c:196, which clears dual_on before the draw).
- No exact fix without the arm: CLAUDE.md 1 (no cause found: the trace goes back, the orchestrator asks the user).

## Questions for the user (the orchestrator asks)
1. 3D slider up or down? 30 or 60 fps? (Widescreen ON, XL: answered.)
2. Which parts are missing: the circles and light behind the wheel icons, the film reel, the HP/FP bar, the sub-menu windows (Jump,
   Items lists), the target names, the action command meter? Do the icons and their names still show?
3. Which console (XL .131, N3DS .224, O3DS .170), and every battle or a named one (map, enemy)?
4. Should the wheel touch the left screen edge in Wide mode (now 54 px from it, the 4:3 distance)?
5. Is there a log.txt from that session (a debug push writes `tridrop` and `rect` counts every 300 frames)? It names the drop arm.

## Next step
After the answers the oracle names the arm and writes the exact-edit Haiku row (bench battle_wheel_nowide_det, or the wide wheel
scenarios battle_wheel_wide_left and battle_wheel5_wide, as the byte gate of the changed path).
