# PT06: Shooting Star Summit, sky and hills look wrong (root cause of the sky found, scroll snap not proven)

Map: hos_00 (area 5 map 0, GB_WorldLocation = LOCATION_SHOOTING_STAR_SUMMIT). Evidence level per part is named.

## Cause 1 (PROVEN): COPY mode texrect height is one row short
- The sky and the hills are the map background image hos_bg (CI8, 296 wide). src/background.c:230-251 (appendGfx_background_texture) draws it in strips of 6 rows. Each strip is one gDPLoadTextureTile of 296x6 and two gSPTextureRectangle calls in G_CYC_COPY with dsdx 4096 and dtdy 1024.
- In COPY mode both lower right corners are inclusive. The game writes yh = (lineHeight*i + lineHeight - 1 + bgMinY) * 4, so a strip covers 6 pixel rows.
- 3ds/src/pm_gbi.c:2302-2309 adds 1 pixel to the width in COPY mode (`width += 1.f`) and does not add 1 to the height. Line 2303 gives height = (yh - yl) / 4 = 5. Line 2311 then gives t1 = t0 + dtdy * 5 / th. pm_gpu_tex_rect (pm_gpu.c:1550-1554) draws the quad over y0 .. y1 + 1 = 6 rows. The 6 rows show the texture span of 5 rows. Each strip stretches by 6/5 and loses its last texture row.
- Evidence: Azahar, row diff of the sky and hill area in shot 3ds/logs/perf/invscene/c/shots/0000.ppm (switches in the Run section). Every 6th screen row (rows 23, 29, 35, 41, 47, 53, 59, 65, 71 of the 400x240 view) is a bit-exact copy of the row above it (sum diff 0.0, the other rows 5 to 60). Mean row diff by row mod 6: 17.3, 19.2, 49.9, 22.4, 22.4, 0.8.
- Fix text (file 3ds/src/pm_gbi.c, line 2307 to 2308). After `width += 1.f;` add:
  `height += 1.f; /* PT06: COPY mode corners are inclusive in both axes */`
  Then t1 = t0 + dtdy * 6 / th and the 6 rows map 1 to 1. pm_gpu_tex_rect needs no change (it already draws y0 .. y1 + 1).
- Risk: every COPY mode texrect with more than 1 row changes (title, other backgrounds). Run golden for title, toad and yard. A 1-row strip now draws 1 row (before: t1 = t0).
- Owner: the VIS-1 agent owns the graphics files. This investigator did not edit the file.

## Cause 2 (GUESS): the "erratic snap" while moving
- The background scroll is `scrollValue = -f5 * 0.25` with a hysteresis of 0.3 and an (s32) cut (src/background.c:196-212). The code runs only when a frame is drawn. A step that FS1 skips (pm_main.c:339-344) leaves gBackroundLastScrollValue and the offset old. The next drawn frame jumps by 2 steps. The N64 loses draws the same way, but the O3DS runs 15 to 25 steps per second, so the jump is larger.
- No panner is involved. The decomp script N(EVS_TexPan_Unknown) (hos_00/main.c:31-39) sets TEX_PANNER_D, and no model of hos_00 uses panner D: all 65 nodes of assets/us/mapfs/geom/hos_00_shape.bin have property 0x5F (special) equal to 0, so the panner is never read. The name "Unknown" fits.
- Not reproduced: the warp shots have a still camera. Azahar runs at 30 steps per second without skips.
- What the user must provide: two or three hardware shots one step apart while Mario walks right along the path (the hills must show), with `loglevel=debug` and `fs1off` in -GameDebug for one run, and the same run without `fs1off`. If the snap goes away with `fs1off`, the cause is the skipped draw (fix: keep the scroll state in the step, not in the draw).

## Run
- PM_ROW=INVSCENE, start 3 of 3: `fast fixedseed warp=kkj_14:2 story=ch0_toad_town tour=hos_00:0 shots shotwin=130:120 drawlog=70:1 smokeframes=290 smoke loglevel=debug`. hos_00 is in from frame 124. 120 shots in 3ds/logs/perf/invscene/c/shots (git-ignored). Diff of 120 frames: only Mario changes (13 to 21 pixels), no hill or sky pixel changes while the camera is still.
