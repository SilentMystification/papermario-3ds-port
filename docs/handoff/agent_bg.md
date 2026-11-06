# agent_bg: background image faults (rows BG1, BG2)

## STATE
- BG1 flicker mac_00 -57 0 76: open, no cause named. Scenario bg_flicker_mac00 and bgrect switch committed 7a89f7289. Azahar starts used 7 of 8. Next: texstale on the sky (ts_bad, needs 300+ frames), palette rebuild under fog, sky texture slot reuse (pm_tex.c).
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
- Scenario bg_flicker_mac00 (azt_scenarios.json, bench bg_flicker_mac00, not default). Debug switch bgrect (background.c: bg_emit prints each sky texrect, appendGfx_background_texture prints bgsky w h y0 xoff) with texlog.
- Build: lane bgf, HEAD 370f67196 plus the bgrect lines only.
- Runs 1 to 7 (Azahar, budget 7 of 8 used). Run 1 left mac_00 at scan 126 (walk left 60 scans goes through the exit into kmr_10): walk right first. Shots come every 2 frames, not every frame; the until line uses a frame number, the shot index is a burst number.
- Run 4 (frames 100 to 159, walk right 40 then left 40): the sky scroll was fixed at xoff 25 from frame 91 (Mario stood still). No sky motion, so no flicker expected.
- Sky scroll (bgsky xoff) moves only when the camera moves: run 6 log: xoff 96 at frames 37 to 80, jump to 14 at frame 81 (camera cut into the street view), 14 to 0 at frame 123, wrap to 295 at frame 124, 295 to 0 at frame 149 (the seam crossings).
- Emitted texrects (bgrect, 4 per frame, zoom 1, wide cam): s range 0 to 296 texels (s0 200 width 96 for the left copy, s0 0 width 200 for the other), t 0 to 200 (dsdx 3413, dtdy 853). No s above 296 and no t above 200 in the emitted list. The padding (WRAP) hypothesis is not seen in the list. BG2 bglog agrees (mac_00 zoom path).
- Shots of runs 3 and 7 (frames 100 to 158, walk, seam crossings at 123 and 149): no flicker chunk seen by eye. Sky mask (blue pixels) per frame: no frame where the sky mask differs from both neighbours while the neighbours agree. The moving sprites (Goomba, Mario) give the only A-B-A differences.
- Not yet: the seam frames with a sky-band crop at full resolution, a non-blue sky colour check (the raster has green and white parts), the pm_tex slot of the sky (texture re-upload or eviction, VRAM at 36 KB free at mac_00). Next code point: the sky texture slot (pm_tex.c, key and reuse) and the palette (gBackgroundPalette rebuilt every call under fog).

## BG2 notes
- Shots read (2026-11-06): spot_mac00_pic 0000_1_1 (zoom path, rotated to 400x240), yard_shot 0000_0_11 (letterboxed, plain path). PPM is stored rotated: transpose(ROTATE_90) gives screen orientation.
- Row outlier scan (row differs from both neighbours, neighbours agree): sky rows show no single bad row. Hits in mac00 are hedge geometry (false positive).
- Yard bottom bg rows 208..219 and top row 20 look normal. Not a seam.
- bglog run (2026-11-06, lane bgr, build 2370, scenario bg2_mac00_bglog, mac_00 at -135 0 85): GPU texture 512x256 for the sky bind. Zoom path: 4 texrects per frame cover screen rows 0..239. Texel s range 0..296 (s1 296 is the exclusive end, the last sampled column is about 295.x), t range 0..199.92 (dtdy 853/1024 times 240 rows). No sample reaches column 296 or row 200, so the padding hypothesis (WRAP past 296 or 200) is FALSE for mac_00. The bottom row 239 samples texel row 199.
- Still no cause for the garbled row. Next candidates: the rounding of the 6/5 zoom (every 6th screen row repeats a texel row, not a garble), the walker sampling at integer t boundaries, the wide copies (bg_texrect shift +-296 px), and the tile 1 bind. Compare the shot row against the ROM sky texels (mapfs Yay0), which is the only exact check.
(BG2 is filling in)
