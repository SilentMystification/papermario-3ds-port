# PT33 to PT40 (INVEND): ending, credits, outro

Method: 3 Azahar starts (PM_ROW=INVEND). Logs and shots: 3ds/logs/perf/invend/r1, r2, r3 (git-ignored). Build: build3ds/pm_3ds.3dsx of 2026-10-21. No source edited. Evidence levels: proven, likely, guess.

## PT34: credits warp (done, tested)
- Select menu entry "Credits": map `end_00`, entry 0, story 96 (STORY_EPILOGUE). Switch form: `warp=end_00:0 story=96`.
- No partner, item or flag is needed. pm_warp.c adds partner 1 for maps that do not start with "kmr_" (pm_warp.c, line "H06e"). The credits scripts do not read the partner.
- Chain (r1, 3ds/logs/perf/invend/r1/log.txt): end_00 (9150 game frames, parade, credits text) then GotoMap end_01 (about 5150 frames) then GotoMap kmr_30 (Mario house, THE END, fireworks). Source: src/world/area_end/end_00/parade_setup.c:1302, end_01/parade_setup.c:836.
- Longer chain with Peach scene first: `warp=kkj_00:6 story=96` (917 frames, then the same chain). Source: src/world/area_kkj/kkj_00/scene_ending.c:218 (GotoMapSpecial end_00). The real game reaches kkj_00 entry 6 through the door of osr_00 (osr_00/main.c:14), after kmr_20 set story 96 (kmr_20/main.c:38).
- Only story 96 or higher gives the Peach sprites: kkj_00 map_init (kkj_00/settings.c:5) selects PLAYER_SPRITES_COMBINED_EPILOGUE for story >= 96.
- Start to a map: sheet of 200 shots (every 30 frames) of end_00 runs in about 60 s with `fast`. Whole chain, 63000 frames, runs in 300 s.
- Shot windows: `shotwin=<f>:<n>` (one window, frame = pm_frame_count). `shotmode=4:<n>` gives n shots at the start of every map (mode 4 = enter world; r3 used it to capture kmr_30).
- Frame table of end_00 (shot index x 30): partners and Toads with banners 700 to 1250, confetti 1100 to 1500, Bowser 2100.

## PT37: confetti is a static square
- Repro: `fast fixedseed loglevel=debug warp=end_00:0 story=96 shots shotwin=1000:400`, shots 3ds/logs/perf/invend/r2/shots/0240.ppm to 0243.ppm (frames 1240 to 1243). Crop 5x of region (270,20,370,100) after rotating 90 degrees: all pieces are full squares with no change.
- Cause (likely, shots prove the symptom, code reading gives the place): the confetti rotation is a sprite sheet, not a matrix. src/effects/confetti.c:302 to :305 sets `gDPSetTileSize(0, uly<<2, ...)` with uly = frame * 16. The texture D_09000000_38BBA0 is 16 x 256 IA4 (16 cells of 16 rows, cell 0 is the full square, cell 3 and 11 are edge on). The tile has T wrap (G_TX_WRAP) and mask T 8 (period 256), loaded with gsDPLoadTextureTile_4b rows 0 to 255 (src/effects/gfx/D_09000940_38C4E0.gfx.inc.c). The RDP reads row (t - ult) mod 256.
- 3ds/src/pm_gbi.c:2884 (FX3f) binds the whole T period only if `tile_wrap[0][1] & 3` (T clamp or mirror). The confetti tile is T wrap (value 0), so the window stays 16 rows. The source starts at LoadTile row 0, tile_y0 = uly, so the sampler reads (t - uly) mod 16 = rows of cell 0 for every frame.
- Fix: in pm_gbi.c:2884 change the test to `!(tile_wrap[0][1] & 2)` (as the S axis test at :2888 does: not clamp). Then h = 256 and tile_y0 = uly gives cell (16 - k) mod 16 as on the N64. Check after the fix: same shots, pieces must change shape. Regression check: shape_spell (FX3f) and the emote icons.
- Owner: graphics agent (pm_gbi.c).

## PT38: ground path stutters
- Repro: same run as PT37, frames 1040 to 1100 (shots 40 to 100). Measure: sub-pixel shift of region rows 128 to 140, x 250 to 350 between consecutive frames. From frame about 1040: -0.55, -0.55, +1.87 px, repeated every 3 frames (sawtooth). Sky, hills and grass shift 0 px per frame. The first 40 shots (pan not started yet) show a steady -0.73.
- Cause (likely, measurement is proven, code place is read): the ground pan is `Add(LVar0, 300)` per frame (parade_setup.c:150 to :157), 0.293 texel per frame. src/model.c:1959 gives `value >> 8` = quarter texels in G_SETTILESIZE (10.2). The port drops the quarter texel: pm_gbi.c:2877 `x0 = uls >> 2` and :2919, :2929, :2951 `tile_x0 = x0 - ...` are integers, and the u coordinate at :634, :1194, :1198 subtracts the integer tile_x0. The path moves in whole texel jumps (1.87 px per texel) every 3 or 4 frames, while the camera moves it 0.55 px per frame in the other direction. The N64 moves it in quarter texels with bilinear filter.
- Fix: keep `tile_fx = (uls & 3) * 0.25f` and `tile_fy = (ult & 3) * 0.25f` (set in G_SETTILESIZE after the clamp at :2790, also in the PG1 skip path :2929 and the MultiTile path), and subtract them where tile_x0 and tile_y0 go into u and v (pm_gbi.c:634, :635, :1194 to :1199, :2641, :2642). The GPU texture filter must be linear (the path looks smooth in the shots, so it is). Check: same shots, shift per frame must be about -0.55 plus 0.55 = 0 jitter. Also check lava, water and cloud pans (all use SetTexPanOffset).
- Owner: graphics agent.

## PT39: star on the Toad banner flickers (NOT reproduced)
- Shots: frames 1200 to 1260 (r2 shots 200 to 260), crop (80,70,150,125) 4x to 16x. The star is stable. Yellow pixel mean colour 184 to 187, 146 to 150, 28 to 31 for 100 frames. The pixel count (75 to 97) follows the sprite animation (period about 16 frames), no alternating frame.
- Cause: guess. The banner is a sprite with the star as a second component (src/sprite.c:434, :452 dz = compPos.z * zscale). Opaque depth test is GPU_GREATER (pm_gpu.c:686), a strict test: parts with the same depth show the winner by float24 rounding. Azahar uses a host depth buffer and hides it. The same class as PT10 (docs/plans/PT10_fix.md) and PT44.
- Not tested on Azahar: nodecal2, nogv1 (budget). Test for the user on the console: -GameDebug `nodecal2`, then `nogv1`, then `fs1off`. The one that stops the flicker names the cause. If fs1off stops it, the cause is the animation step between drawn frames.
- Possible fix if it is depth: a small depth bias per component index in spr_appendGfx_component (src/sprite.c:337), or GEQUAL for sprite components of the same sprite.

## PT40: no Mario sign in the outro (reproduced in Azahar)
- Repro: `fast fixedseed loglevel=debug warp=kmr_30:0 story=96 shots` (r3 reached it through the chain, shot 391, 3ds/logs/perf/invend/r3/shots/0391.ppm). The oval sign plate above the door of the house is empty.
- What belongs there: textures kmr_m_kanban2tif and kmr_m_kanban3tif (assets/us/mapfs/tex/kmr_tex, 128 x 32 IA4, clamp, combine Multiply, white letters "MARIO'S H..." and "MARIO"). kmr_30 is a night map (SHADING_KMR_30, kmr_30/main.c:7).
- Cause: guess. The text is not drawn at all. Candidates in order: (1) the sign model uses a decal render mode (ZMODE_DEC), then pm_gpu.c:775 (nodecal2) is the place; (2) the IA4 Multiply texture with the night shading gives black on black; (3) a model flag. Not tested (budget). First test: same warp with `nodecal2`, then `warp=kmr_20:0 story=96` to see if the daylight sign draws in the port.
- Note: the user says "Mario logo". The only logo-like texture of the house is this sign. No other candidate was found.

## PT33: Peach not visible in the castle (NOT reproduced)
- Repro with the real flow: `fast fixedseed autoa loglevel=debug warp=kkj_00:6 story=96 shots shotmode=4:90`. Shots 3ds/logs/perf/invend/r3/shots/0094.ppm to 0112.ppm show Peach (pink dress) at the door and in the speech. The scene runs frames 49 to 966, then end_00.
- So the script and the sprite loading work in Azahar with story 96.
- Cause on the console: guess. Peach is drawn with spr_draw_player_sprite(PLAYER_SPRITE_AUX1) (src/npc.c:924, NPC flag NO_ANIMS_LOADED, create_peach_npc). It needs the player sprite set COMBINED_EPILOGUE (10 cache entries, 0x900 raster size, src/sprite.c:125) from kkj_00 map_init at story >= 96. Cases that differ on the console: (1) the story value at kkj_00 entry is below 96 (a save made before kmr_20 entry 2; GB_StoryProgress is set to 96 in kmr_20/main.c:38 only for entry 2). Then the set is the Mario world set and Peach has no raster. (2) The sprite heap or raster cache is full on the O3DS (spr_load_player_sprite fails). 
- Check for the user: the log of that run with `loglevel=debug`, line "warp" or map load, and the story value. A fix for (1) is in the game flow, not in the port: do not enter kkj_00:6 below story 96. A fix for (2) needs a log line from spr_load_player_sprite (src/sprite.c:810 loop).
- Not tested: a console run, a run with a save file.

## Direct warps per scene (no credits replay)
Run each with `PM_ROW=<row> sh 3ds/run_azahar.sh -F -t 60 -u "<regex>" -d "<switches>"`. The window mode and the burst mode log one line "shot <n> frame <f>" per shot, so `-u "shot 399 frame"` stops the run right after a window of 400 shots (use the last index of your window). Game frames run at about 200 per second in -F, so frame 1400 takes under 15 s. In r2 no -u was set and the run lasted the full 200 s: this was the waste.
- Peach scene (PT33): `fast fixedseed autoa loglevel=debug warp=kkj_00:6 story=96 shots shotmode=4:90` and `-u "game mode 5 frame"` (the scene ends at frame 966).
- Confetti with the partners (PT37), Toad banners (PT39), ground path (PT38): no map entry exists in the middle of the parade. end_00 starts the parade at camera x -3080 and the scroll runs at 2.67 units per frame. Use `fast fixedseed loglevel=debug warp=end_00:0 story=96 shots shotwin=1000:60` and `-u "shot 59 frame"`. Frames 1000 to 1060 show banners and ground path, frames 1200 to 1260 show confetti, partners and banners (about 5 to 8 s of wall time). Do not use more than 60 frames per check.
- Mario house outro (PT40): `fast fixedseed loglevel=debug warp=kmr_30:0 story=96 shots shotwin=300:30` and `-u "shot 29 frame"` (the scene is static after frame 300, the night house is already drawn at frame 100).
- end_01 (second parade map, float scenes): `fast fixedseed loglevel=debug warp=end_01:0 story=96 shots shotwin=<f>:<n>`.
- Seek idea (not built): a switch `creditsseek=<frame>` in pm_warp.c that, after the map load of end_00, calls the parade script loop with the camera set to x = -3080 + 2.67 * frame and starts the phase scripts (ParadePhase_Partners and ParadePhase_Toads) directly. Phase scripts start by camera x (end_00.h:148 PARADE_PHASE_PARTNERS = -2845, TOAD_TOWN = -2550), so a camera x set above these values makes the phases run at once. This saves the 1000 frames of the wait. Needed only if 5 to 8 s is too slow.
