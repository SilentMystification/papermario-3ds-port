# agent_bg: background image faults (rows BG1, BG2)

## STATE
- BG1 flicker mac_00 -57 0 76: open. Scenario and bench not yet added.
- BG2 garbled row: open (agent ac8b933e18b4832cf).

## How the background is drawn
- src/background_gfx.c gfx_draw_background (L336): default branch fills the screen, then calls appendGfx_background_texture (src/background.c L130) when BACKGROUND_FLAG_TEXTURE is set.
- 3DS path of appendGfx_background_texture (background.c, `#ifdef TARGET_3DS`, not wave): ONE CI8 bind of the whole sky: gDPLoadTextureTile(raster, G_IM_FMT_CI, G_IM_SIZ_8b, bgMaxX (296), h = backgroundMaxY, lrs 295, lrt h-1, WRAP both). No strips on 3DS.
- Source row offset y0 = gBackroundTextureYOffset (y0 -= h if y0 >= h). Two calls of bg_sky_rows (background.c ~L300): screen rows [0, h-y0) from source t = y0*32, screen rows [h-y0, h) from t = 0.
- bg_sky_rows: two texrects per row group. Left: screen x minX..minX+xOff-1 from s = (maxX - xOff)*32. Right: from s = 0. xOff = scroll (backgroundXOffset) mod 296. dsdx 4096, dtdy 1024 (texrect s/t in 10.5, dsdx/dtdy in 5.10).
- bg_texrect/bg_emit: with pm_wide adds copies at +-296 px. bg_zoom (wide cam, 296x200 at 20) scales the rect by 6/5.
- Texrect coordinates are 10.2 (x*4, y*4). Lower and right edges are inclusive: height = (yh - yl)/4 + 1.
- Walker: src/pm_gbi.c gbi_texrect (L4417). COPY mode (cyc 2): dsdx /= 4 (4096 = 1 texel per pixel), width and height += 1 (PT06, L4466). Tile fetch via ensure_tile / pm_tex_size. The fold path (fold_axis, L4470 area) runs only for tile_wrap == 3 (not the sky tile, which is WRAP).
- Palette: gBackgroundPalette is rebuilt each call when BACKGROUND_FLAG_FOG (fog, tint); TLUT loaded with gDPLoadTLUT_pal256 every call.
- Wave path (gBackroundWaveEnabled): still 6-row strips on 3DS (tile 296 x 6, per strip texrects, x shift by sin). Not used by mac_00 unless the wave is on.
- Texture cache: pm_tex.c (texture slots, keys by address, size, format, palette). Sky raster sits at gGameStatusPtr->backgroundRaster (decoded by load_map_bg, background.c L23).

## BG1 notes
(BG1 is filling in)

## BG2 notes
- Shots read (2026-11-06): spot_mac00_pic 0000_1_1 (zoom path, rotated to 400x240), yard_shot 0000_0_11 (letterboxed, plain path). PPM is stored rotated: transpose(ROTATE_90) gives screen orientation.
- Row outlier scan (row differs from both neighbours, neighbours agree): sky rows show no single bad row. Hits in mac00 are hedge geometry (false positive).
- Yard bottom bg rows 208..219 and top row 20 look normal. Not a seam.
- No row named. Open: the 296 wide, 200 high texture sampled with WRAP: a sample at t >= 200 (zoom bottom row, t about 199.2 plus edge) would read texture padding if the walker pads the texture. Check tw/th from pm_tex_size and the sampled t of the last row. Needs a log of t1 per sky row (diagnostic, bglog switch) and one Azahar shot, or the ROM bg asset (mapfs, Yay0) to compare exact texels.
(BG2 is filling in)
