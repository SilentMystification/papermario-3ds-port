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

## Questions for the user (the orchestrator asks)
1. Widescreen option: Native, Wide or Fullscreen? 3D slider up or down? 30 or 60 fps?
2. Which parts are missing: the circles and light behind the wheel icons, the film reel, the HP/FP bar, the sub-menu windows (Jump,
   Items lists), the target names, the action command meter? Do the icons and their names still show?
3. Which console (XL .131, N3DS .224, O3DS .170), and every battle or a named one (map, enemy)?
4. Should the wheel touch the left screen edge in Wide mode (now 54 px from it, the 4:3 distance)?
5. Is there a log.txt from that session (a debug push writes `tridrop` and `rect` counts every 300 frames)? It names the drop arm.

## Next step
After the answers the oracle names the arm and writes the exact-edit Haiku row (bench battle_wheel_nowide_det, or the wide wheel
scenarios battle_wheel_wide_left and battle_wheel5_wide, as the byte gate of the changed path).
