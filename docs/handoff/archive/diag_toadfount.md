# Diagnosis: TOADFOUNT (Toad Town pond, mac_00)

## Status
- Console position: x -174, y 0, z 112 (user, Position HUD). Map: mac_00 (preset=toad, entry 4, pm_warp.c:35-40 table).
- Pond scenario toad_pond ran on HEAD (build 1488), 29f504d7b (lane final) and final6 (lane toadf6). Budget of 3 starts used.
- Cause NOT found. No game source edit.

## Map data [C]
- mac_00 entry 4 at (-100, 30, -370), src/world/area_mac/mac_00/settings.c.
- Water model MODEL_water (0xD6, mac_00_shape.h:9). Custom gfx: src/world/area_mac/mac_00/water_gfx.c:6-31.
- mac_01 decode (earlier): no leaf in the map origin. The mac_01 candidate at (-304, -329) is not the pond.

## Pond run toad_pond [M]
- Route: padscript from entry 4 to (-174, 112), wait 40. Route arrival at f188 (HEAD, lane final, final6), within 24 units.
- POSHUD bottom shot bot_0199 (HEAD): "x -168 y 0 z 93 yaw 195" (read by eye). Arrival radius is 24 units, so Mario stops short of the target.
- Frame grid differs by build: HEAD and final6 shots at 163, 167 ... 199. 29f504d7b shots at 160 ... 196.
- Top shot 0009 (HEAD, rotated upright, frame 199): TOAD TOWN arch, plaza, Mario at the edge of a round brown rim. A blue pool with a white stone edge fills the bottom edge. Blue water shows no background. Ripples visible (light lines).
- Layer order (code, not by eye): background, then blocker (opaque colour, const depth), then blended overlay (see source).
- shot_cmp (HEAD vs final6, same frames 163 to 199): 0 of 10 byte-equal. Differences 413 to 668 px, in a box x 93 to 137, y 82 to 187 (portrait). The box follows Mario's walk and stays on Mario's sprite after f191. Water region (y 160 to 240 bottom edge) is equal. [M]
- 29f504d7b shots are on other frames, so no per-frame compare. [M]

## Source path, water draw [C]
- Pass 1 (blocker): src/world/area_mac/mac_00/water_gfx.c:9-10. Depth source PRIM, prim depth 0x7FF0.
- Pass 1 render mode: water_gfx.c:11-12. Z_CMP, ZMODE_OPA, FORCE_BL, blend GBL_c2(CLR_IN, 0, CLR_IN, 1): output = colour in (opaque).
- Pass 1 combine and prim alpha: water_gfx.c:13-14 (alpha 128).
- Draw: water_gfx.c:19 (model display list of MODEL_water).
- Pass 2 (overlay): water_gfx.c:22-25. Depth source PIXEL, render mode G_RM_AA_ZB_XLU_SURF2 (translucent, z buffer), then mdl_draw_hidden_panel_surface (src/model.c:4679-4697).
- Texture panning of MODEL_water: src/world/area_mac/mac_00/main.c:97.
- Render mode map for leaves: src/model.c:1594-1655. Mode names: include/enums.h:3301-3350.

## Ranked candidates [E]
1. Pass 1 writes a fixed depth (0x7FF0) with an opaque colour. Pass 2 then tests against that depth. Any leaf drawn after pass 1 but with depth above 0x7FF0 is hidden, and any leaf with depth below it shows. Check: depth values of pass 1 and of the overlay pixels on the 3DS (not measured).
2. The mac_01 candidate (rm 17 and 34 translucent leaves, group at (-304, 39, -329)) is a different object. Not checked against the pond.
3. Equal-state skip (src/pm_gbi.c:2752, from diag_toadvis.md) may drop a mode word between pass 1 and pass 2. Not measured.

## Next step
- Fountain is not in the pond shots. The pond water and its layers look correct in one frame. Check the user's fountain view on the console with the same HUD values.
- For the water region, compare HEAD and final6 on frame 199 only, with a mask for x 93 to 137, y 82 to 187. Run a HEAD repeat first if a noise check is needed.

## Limits
- Azahar: budget used (3 of 3). Pond water is read by eye only.
- Bottom screen values read by eye (small font).
- Shot writer runs at about 3 frames per second.
