# Row BTLUICEN: which draw path drops the battle UI background and the lower-left item (census, head against the build before NAPI)

(Model: Haiku. Bug: user fact: part of the battle UI does not render anymore: the background and the item in the lower left corner;
it worked in earlier builds. Code only: no shot reading.)

## Code read (oracle, main a2265beb8)
User detail: in the main battle menu the option icons draw; the background panel and the lower-left element do not.
btl_main_menu_draw (src/battle/btl_states_menus.c:852-990) draws everything with hud_element_draw_complex_hud_next, i.e.
render_complex_hud_element (src/hud_element.c, the TRANSFORM path, not NAPI2A's hud_element_draw_rect): the reel HID_ProjectorReelA/B
(lower left, pm_wide_left(40), 212; :657-675) and the beam HID_ProjectorBeam (the panel, :677-690) are HES_ProjectorReel and
HES_ProjectorBeam = hs_UseIA8 64x64 (src/battle_ui_gfx.c:522-546): case 2 of render_complex_hud_element: a quad in
transform->quadBuffers[FrameQuadIndex], gDPLoadTextureBlock IA8, PM_CC_47, gSPVertex + 2 gSP1Triangle through the walker ring path
(nd_geom_vtx, draw_tri, gv_tri). The option icons are CI4: case 1, imgfx_appendGfx_component (other vertices and loads). So the drop is
on the IA8 quad path. Paths that drop it with no fallback, each counted by an existing census line (switch census):
 1. a ring triangle refused and dropped (TRI-4, nd_geom.c:291-293; gv_tri :198-251: GVR_SCR, TSS, LIT, FOG, ZMARK, TEX1, REFUSE: the
    wide edge test of pm_gpu_gtrip, gtrip_wide :2901, for an ortho UI triangle in the margin): `tridrop f<n> none .. refuse ..`;
 2. the vertex load refused (no gmi or ring room: GVR_RING, nd_geom.c:328): in `tridrop none`;
 3. the batch refused at close (nd_raw_native, pm_gpu.c:1360-1389): `nrawwhy f<n> zmark .. tex1 .. unit0 .. tmpl ..`;
 4. the texture of the IA8 load refused (unit 0 off-screen or debug, nraw unit0; texture cache full: `tex cache full` line);
 5. the vertices read by core 0 after core 1 rewrote them (quadBuffers[FrameQuadIndex], a 3-slot ring; the walk is one frame behind).
The census at the menu frames (head) against the build before NAPI (9eeb74b1a) names the path, or shows the drop is older.

## Start
`sh 3ds/tools/wt_new.sh BTLUICEN`. PM_ROW=BTLUICEN PM_LANE=BTLUICEN.

## Exact edits (tool files only)
Scenario and bench btlui_cen: copy of battle_start_f182 with debug `warp=kmr_03:0 story=ch0_toad_town battle battlef=100 census
loglevel=debug det`, until `rectwhy f400`, cap 40, needs_picture false; bench azahar_stream, normal, default false. `bench.py rebuild`.
Commit both together.

## Gate (measurement)
Build (lane BTLUICEN), memcheck OK. `bench.py run btlui_cen --row BTLUICEN --why "battle UI drop path, head"` and `bench.py build-at
9eeb74b1a btlui_cen --row BTLUICEN --why "battle UI drop path, before NAPI"` (2 builds, 2 Azahar starts). Hand back, for both logs,
the f300 and f400 lines rect, rectwhy, tridrop, nrawwhy, ndm, sprnat, and every `ndc:` error line, as printed. Stop after it.

## At the end
Delete this row file in the completing commit.
