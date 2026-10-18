# X02 darkness notes (read only study)

## Decomp code
- `appendGfx_darkness_stencil` (src/screen_render_util.c:129) draws the dark circle. It draws into the z-buffer memory as a 64 x 64 image (32 bit, then 8 bit). Then it uses that image as a texture for the screen overlay.
- `src/screen_overlays.c:344` calls it for OVERLAY_WORLD_DARKNESS. Line 351 calls it for OVERLAY_BATTLE_DARKNESS (battle only, toy_tank).
- Dark rooms kgr_01, kgr_02 and omo_12 use `EVS_CreateDarkness` (src/world/common/atomic/CreateDarkness.inc.c). kzn_08 has its own copy (src/world/area_kzn/kzn_08/extra.c:20-62). omo_14: no darkness call found in the grep.
- Watt has no separate light code. The script `DarkRoomUpdate` lowers an alpha value from 255 to 90 (step 8 per frame) while `partnerActionState != NONE` and `curPartner == PARTNER_WATT`. It raises it back to 255 when the action ends. It passes the value with `set_screen_overlay_alpha`.
- The stencil function turns alpha into `texScale` (world: 0.5 + (255 - alpha) * 9.5 / 255) and moves the center toward the screen center. A lower alpha gives a bigger circle. So the Watt radius is the same code path as the plain circle.

## PC port (reference/paperboat)
- `nuGfxZBuffer` is NULL on the port (src/port/nusys_overrides.c uses a dummy z-buffer). The real stencil would draw to address 1 (the screen).
- The port replaces the function: `port_appendGfx_darkness_stencil` in src/port/patches/DarknessStencilPatches.cpp. It builds a static 64 x 64 I8 radial gradient once (C array). It draws one textured rectangle with that texture. No render to texture.
- It keeps the original `texScale` and center math. So the Watt radius works: alpha 255 to 90 changes dsdx and the start s, t.
- Alpha uses combine (1 - TEXEL0) * ENV + PRIM. The port also widens the rectangle for widescreen.
- sm64-3ds and soh-3ds have no equivalent (no dark room effect that uses color image redirect).

## X02 in the main tree (uncommitted)
- PROGRESS.log: 07:08 X01 found the cause (stencil renders to the z-buffer image). 01:31 X02 found nuGfxZBuffer NULL. No log line says X02 is done or tested.
- Diff: `pm_gbi.c` tracks the pixel size of the color image (`cimg_siz`) and sends non-16-bit images off-screen. `pm_gpu.c` has 64 x 64 slots (16 KB VRAM). `pm_main.c` wraps `appendGfx_darkness_stencil` and allocates the z-buffer on first use (`pm_zbuf_need`). `pm_tex.c` binds off-screen textures. A `tmpnofill` debug check is still in (marked TMP X02).
- X02 runs the REAL decomp function with the off-screen render to texture. It does not use the paperboat shortcut.

## Does X02 cover Watt?
- By design yes: the alpha to texScale math is inside the real function, so the Watt radius uses the same code. No Watt specific code is needed in the port.
- Not verified. No test with Watt (partner action, alpha below 255) exists in the log. The plain circle (alpha 255) is also not logged as verified.

## What X02 still needs
1. Remove the `tmpnofill` TMP check.
2. Test the plain circle in kgr_01 (alpha 255). Take a text check (draw log or one pixel diff).
3. Test Watt: partner Watt, action state set, alpha 255 to 90 over about 21 frames. Check that the circle grows. Check the texrect with large scale (texScale about 6.6, texture mirror and clamp at mask 6) on the 8 bit slot.
4. Check the second use in the same frame: the 32 bit slot and the 8 bit slot share one memory address (two slots). Check kzn_08 (own alpha) and omo_12 (alpha 255 and 0 switching).
5. Commit X02 and mark the row done.
