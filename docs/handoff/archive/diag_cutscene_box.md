# Diagnosis: intro cutscene border box

Status: partial. The intro draw call was not located. Candidates below are not verified.

## Scene switch
- `demoscene=<n>` debug value: `3ds/src/pm_warp.c:602` (index into DemoScenes in `src/state_demo.c`; 0 = nok_12, 5 = jan_04 unchecked).
- No intro-specific switch found.

## Candidate draw calls
1. Scissor fallback for 2D or ortho batches: `3ds/src/pm_gpu.c:903`
   `sh_scissor(GPU_SCISSOR_NORMAL, 0, pm_wide ? 0 : 40, 240, pm_wide ? 400 : 360);`
   Value used: x 0..240, y 0..400 (wide) or 40..360 (non-wide). Filter and wrap: not set here.
   This runs when the scissor is empty. It may draw the full picture rectangle, which matches the visible box.
2. Fill rectangle with fill colour: `3ds/src/pm_gbi.c:2912-2944` (G_FILLRECT). Colour is `prim` or `fill` (1-cycle vs fill cycle).
3. Test pattern fill: `3ds/src/pm_gbi.c:3395` (`gDPFillRectangle(0,0,319,239)`, colour 0x22292229). This is the test pattern, not the intro. Ruled out for the intro unless the intro reuses it.

## Likely cause (hypothesis)
Candidate 1 is the best fit. The fallback scissor covers the full picture, and the cutscene area is drawn with an edge that is not cleared. Confirm with a log line at `pm_gpu.c:903` during the intro.

## Fix (hypothesis, not tested)
- Clear the area outside the cutscene with the stage colour before the cutscene draw.
- Or set the scissor to the cutscene rectangle only, not the fallback.

## Next step
Find the intro draw by log: add a read-only log at `pm_gpu.c:901-903` and read `sci_x0 sci_y0 sci_x1 sci_y1` in the intro. Record the values here.

## Run log (box agent, 2026-10-27)
- Lane build PM_LANE=box: build3ds/lane-box/pm_3ds.3dsx (baseline, no box log).
- Added switch boxlog (pm_gpu.c flush_draw and pm_gpu_fill_rect): one log line per GPU draw and per fill rect, with scissor, combine, tex, prim, blend, rgba.
- Azahar start 1 of 8 used (row box), run with -Until "game mode 14 frame" -Timeout 240: waited 900 s on the Azahar lock. Lock holder: Azahar pid 5180, started 16:24, not owned by this agent. Run stopped before start.
- Azahar start 2 queued with -Timeout 90 (-Until "game mode 16 frame", boxlog shots loglevel=debug): blocked by the same lock holder. Not started at the time of this note.
- BLOCKER: Azahar global lock (Global\pm_azahar_run) held by pid 5180. No shot of the intro exists yet. No fix is made.
- Next: when pid 5180 exits, run PM_ROW=box PM_LANE=box run_azahar.ps1 -Timeout 90 -Until "game mode 16 frame" -Debug "shots loglevel=debug boxlog", read the box lines for the intro frames, then name the draw value.
- Azahar start 2 ran (box, -Timeout 90, -Until "game mode 16 frame"): lock cleared, intro reached at frame 327 (game mode 16). 10 shots in build3ds/lane-box/shots, PPM.
- Shot 0009 (intro): stage frame, grey panel in the centre, checker walls left, red and gold curtain right, black bars top and bottom. The grey panel matches the per-draw log line: fill rgba c9c9c9 (prim) over full screen, rep 1 blend 1, scissor 0..320 x 0..240, combine 0.
- Not done: no draw in the log is yet tied to the box edge. The fill rect lines (rgba 000000ff, x 0..319 y 0..27 and y 213..239 range) match the black bars, not a border.
- Status: NOT FIXED. Next: a shot with a log window on f327 to f360, then compare the box edge pixels with the draw rectangles (fill x0 y0 x1 y1).
- Start 3 (box, shotwin=327:34, -Timeout 90, Until 'shot \d+ frame 360'): queued behind Azahar pid 28396 (another session, started 16:35:59). Not started. Budget used 3 of 8.
- BLOCKER: shared Azahar lock held by another session. No frame 327 to 360 shot yet. No fix yet.

## Run 1 result (after handoff)
- Azahar start 1 of 8 (box, shotwin=327:34). Shots 0000 to 0009 = frames 327, 331, 335, 339, 342, 345, 348, 352, 356, 359.
- Shot 0000 (240x400, PNG via PIL): grey panel x 50..210, y 70..328 (screen pixels). Black bars y 0..40 and 360..400. Checker wall x 0..50. Gold checker bands y 40..70 and 328..360. Red curtain x 210..240.
- Screen mapping (non-wide, full scissor): screen x = 240 - game y, screen y = 40 + game x. Panel in game units: x 30..288, y 30..190 (derived, not logged).
- Frame 327 draws: fill d0d0d0 full 0..319 x 0..239 (line 8009); 2D draws prim c6c6c6 r2d 1 nv 6 (8000..8008); 3D textured draws combine 8 tex 1 sci 0..320 x 0..240 (8011..8018). Vertex extent not logged.
- Edit: boxlog line in pm_gpu.c now also logs vertex bounds (vbox x, y).

## Run 2 result (Azahar start 3 of 8, row box)
- Shots 0000 to 0009 again, frames 327 to 359. Log has "shot 0 frame 327".
- Frame 327 vbox (2D draws, screen-side game coords): #8010 fill d0d0d0ff, blend 0, rep 0, vbox x 40..360 y 0..240 (full picture). This is the grey panel source.
- #8000 to #8008 (c6c6c6ff, blend 1, rep 1): thin strips y 211..240 and full-screen rects. Not the panel edge.
- #8011 to #8018 (graw 1, combine 8, tex 1, 3D raw batch): vbox -1..-1. The raw batch vertices are in gring (16 bytes per vertex, GPU transforms them). The screen extent is NOT logged.
- Result: the grey panel is what remains of the d0d0d0 fill (#8010) where the 3D raw batches (#8011 to #8018) do not cover. Panel edge = edge of the 3D raw batch coverage. Not matched to a single draw.
- BLOCKER: missing log line. The raw batch screen bounds (clip-space or screen x, y per draw from gring after the GPU transform, or the gidx range) are not logged. Add it to the graw path in flush_draw (pm_gpu.c near line 925) to name the draw that ends at the box edge.
- No fix committed. Box still visible in frames 327 to 359.

## Summary for the next agent (state at 6 of 8 Azahar starts)
- Shots: build3ds/lane-box/shots/0000.ppm to 0009.ppm (frames 327, 331, 335, 339, 342, 345, 348, 352, 356, 359). PNG copies: build3ds/lane-box/shots/0000.png to 0009.png (PIL).
- Box edge pixels (shot 0000, 240 x 400): grey panel x 50..210, y 70..328. Black bars y 0..40 and 360..400. Panel colour about 208 (d0).
- Log: build3ds/lane-box/last_log.txt (run 6). Frame 327 lines: "shot 0 frame 327" at the first shot.
- Draw #8315 line is the raw batch gi 192 n 24 (NDC x -583..758, y -649..649), its draw line is #8600 in run 6 numbering. Pixel mapping: x_pix = 120 * (ndc + 1), y_pix = 200 * (1 - ndc) for the 240 x 400 target (fits x 50 and 210, y 70 and 328).
- Draw #8600 state: slot 28, 32 x 16, fmt 2 (CI), siz 0 (CI4), stride 32, combine 8, blend 1, rep 0, prim c5c5c5ff, tex 1 (unit 0 bound).
- Texels: (0,0) index 9, (31,0) index 2, (0,15) and (31,15) index 0, (16,8) index 6.
- Palette RGBA5551, entries 0 to 15: e5d7 9bc9 7285 bccf ee9f dd93 a40d 9345 6203 c515 e61b ac4f cd11 8309 b4d5 d555. Entry 0 alpha bit is 1 (opaque). The alpha-cut hypothesis is not supported.
- Background draw #8305 (fill d0d0d0ff, blend 0, rep 0, full screen) is the grey source. Draws #8307 to #8319 are 3D raw batches with combine 8 and tex 1.
- Open question: draw #8600 palette colour is tan, but the panel pixels at frame 327 are neutral grey. So #8600 may not cover the panel interior, or its UV samples another texel area.
- MISSING LOG (exact): in the graw path of flush_draw (3ds/src/pm_gpu.c, near the "box raw" block, about line 905 to 935), log per raw vertex the UV (gring bytes 8 to 11 as s16 u, v) and the final pixel colour is not available on CPU. Log the UV for gidx range gi_start..gi_start+gni, and read back the framebuffer pixel at the box corner (x 50, y 70) after the draw if possible. Without the UV, the texel for the panel interior is unknown.
- No fix committed. Box still visible in frames 327 to 359.

## Run 3 result (Azahar start 4 of 8, row box, raw NDC extent log)
- The "box raw" line is written before its "box draw" line. The raw extent belongs to the next draw line.
- NDC check (target 240 x 400, shot 0000): panel edge pixel x 50 = NDC -0.583, x 210 = NDC +0.75. Pixel y 70 = NDC +0.65 (flipped), y 328 = NDC -0.64.
- Match: raw batch gi 192 n 24 (frame 327) has NDC x -583..758, y -649..649. Its draw line is #8315: graw 1, combine 8, tex 1, prim c5c5c5ff, blend 1, rep 0, z 0.
- The grey panel is the d0d0d0 opaque full-screen fill (#8305, blend 0, rep 0) visible where raw batch #8315 paints nothing. Panel edge = extent of #8315.
- Not yet proven: whether #8315 texels are transparent inside the panel. Texture id and texel alpha are not logged.
- Next: log the texture id and texel alpha inside the #8315 extent. No fix committed yet.

## Run 5 and 6 result (Azahar starts 5 and 6 of 8, row box)
- Draw #8600 (after "box raw f327 ndc x -583..758", gi 192): slot 28, 32 x 16, fmt 2 (CI), siz 0 (CI4), stride 32, blend 1, rep 0, combine 8, prim c5c5c5ff.
- Corner texels: (0,0) index 9, (31,0) index 2, (0,15) index 0, (31,15) index 0, centre (16,8) index 6.
- Palette (RGBA5551, first 16 entries): 0 e5d7, 1 9bc9, 2 7285, 3 bccf, 4 ee9f, 5 dd93, 6 a40d, 7 9345, 8 6203, 9 c515, a e61b, b ac4f, c cd11, d 8309, e b4d5, f d555.
- Entry 0 has alpha bit 1 (opaque). The corner alpha test hypothesis is NOT supported for this draw.
- Shot 0000 panel pixels are neutral grey (about 208). The palette colours of this draw are tan, so this draw does not show in the panel interior at frame 327. The UV mapping to texels is not logged.
- BLOCKER: per-vertex UV and the final colour per pixel of raw draw #8600 are not logged. Name for the next log: the raw vertex UV (gring bytes 8 to 11) in pm_gpu.c flush_draw graw path.
- No fix committed. Box still visible in frames 327 to 359.

## Handoff (rotation at 219k tokens, 2026-10-27)
State: BLOCKED. Box not fixed. No box edge pixels measured. No draw matched.
Commits on worktree-agent-a0f129493821e7d00: c956456dc (boxlog switch, run log), 16bd92aaa (blocker note). Merge of dlc-compiler-c1c2 done at start.
Box edge pixels: none measured. Frame 327 shot (intro): grey panel centre, black bars top and bottom, checker walls left, red and gold curtain right (build3ds/lane-box/shots/0009.ppm, PNG copy in the agent scratchpad).
Draws seen at intro frame 327 (boxlog): fill rgba 000000ff x 0..319 y 0..27; draw prim c9c9c9ff combine 0 tex 0 rep 1 blend 1, scissor 0..320 x 0..240. Not yet matched to the box edge.
Azahar budget (row box): 3 of 8 used. Start 3 (shotwin=327:34) queued behind Azahar pid 28396 (other session, since 16:35:59); the wait may have timed out.
Open items: (1) frame 327 to 360 shot with boxlog, pixel-measure the box edge; (2) match edge to fill/draw rect, scissor or texture edge; (3) one fix; (4) rebuild, reshoot, commit.
Next steps and exact commands:
  cd <worktree>
  PowerShell: $env:PM_ROW='box'; ='box'; powershell -File 3ds/run_azahar.ps1 -Until 'shot \d+ frame 360' -Timeout 90 -Debug 'shots shotwin=327:34 loglevel=debug boxlog'
  Build: PowerShell $env:PM_LANE='box'; & 'C:\Program Files\Git\bin\bash.exe' 3ds/build.sh
  Before build: python 3ds/tools/gen_map_restore.py, then restore 3ds/src/map_restore_table.c (never commit it).
  Convert PPM to PNG with PIL, read the PNG, measure the edge.

## Box2 agent (2026-10-27, stopped on user order before any Azahar start)
- Build: PM_LANE=box2, build3ds/lane-box2/pm_3ds.3dsx (no code change, boxlog as merged). Azahar starts used: 0 of 5.
- Code finding: frame 327 is GAME_MODE_INTRO (16), intro part 0, map hos_05. src/state_intro.c:36-44 sets the back screen overlay OVERLAY_SCREEN_COLOR, colour 208 208 208 (d0d0d0), alpha 255. This is the fill #8305 (d0d0d0ff, blend 0, full screen). The decomp draws the same grey on N64 until INTRO_FADE_IN lowers the alpha by 4 per step (state_intro.c:246-258).
- Code finding: draw #8600 (CI4 32x16, tan palette) is theater_floor_png from D_800758A0 (src/curtains.c:35-42). TheaterLeftWallGfx and TheaterRightWallGfx use it (the tan checker walls). Its texels do not cover the grey panel, so it is not the panel source.
- Code finding: the dark gradient at the panel left and right edges is TheaterInsetShadowsGfx (curtains.c:82-92, G_CC_SHADE, XLU). It also draws on N64.
- Pixel check (agent-a0f129493821e7d00 shot 0009, frame 359): panel is flat 210 210 210. No line or rectangle edge other than the inset shadow ramp (132 to 198 over 8 px) and the wall and curtain edges. Frames 327 to 359 match the N64 draw order (overlay grey, then theater on top). The box is not visible in the 327:34 window.
- Value the draw used so far: panel = back overlay fill d0d0d0ff alpha 255 (IntroOverlayAlpha 255 - 4 = 251 or more), theater prim c5c5c5ff (gCurtainFade about 0.23 to 0.3).
- Open item: the box the user sees is likely after the fade starts (alpha below 255, hos_05 world visible in the panel), or only on the console. Not proven.
- Next: one Azahar start with PM_ROW=box2 PM_LANE=box2, -Debug 'shots shotwin=327:300 loglevel=debug', -Timeout 90, Until 'shot \d+ frame 6[0-9][0-9]'. Find the first frame where the world shows in the panel and a box edge appears. Then add boxlog lines for the world camera viewport and scissor (pm_gpu.c sci_* near line 903) on that frame.

## Box3 agent (2026-10-27): first visible box edge, draw and value
- Build: PM_LANE=box3, build3ds/lane-box3/pm_3ds.3dsx (boxlog as merged, no code change). Azahar starts used: 1 of 8 (row box3). Run: shotwin=327:300, Until 'shot \d+ frame 6[5-9][0-9]'. Shots are about every 3 to 4 frames (88 shots, frames 327 to 625), not every 10 frames. Log: build3ds/lane-box3/last_log.txt. Shots: build3ds/lane-box3/shots/0000.ppm to 0087.ppm, PNG copies in the agent scratchpad.
- Frames 327 to 583: no box edge. Panel is grey (about 210) with the checker wall, gold bands and red curtain. Frame 584 (shot 76) has one dark pixel at x 125, y 329. This is a one-pixel artefact, not a box.
- FIRST BOX EDGE: frame 588 (shot 77). A hard horizontal edge at screen y 303 across the panel, x 50 to about 198. Below it: dark blue sky (world) to y 329. Above it: grey. Column 125 jumps by 585 (grey 214 to 8).
- The edge then moves up: y 261 (f591, shot 78), y 219 (f594, shot 79), y 168 (f598, shot 80), y 86 (f601, shot 81). From f605 (shot 82) the sky fills the panel to the top. The grey top strip is gone by f605.
- Matching draw: the 2D textured strip set with prim b4b4b4ff, combine 8, tex 1, blend 0, rep 0, scissor sci x0 29 y0 28 x1 291 y1 190. Each strip is 7 game units tall (y 50..212 in 7-unit steps, 6 vertices, nv 6). Its right vertex x is the box edge.
- Value the strip draw used (vbox x max, game units): f584 303, f588 261, f591 219, f594 168, f598 86. Left vbox x is always 40. The first strip line for f588 is "box draw f588 ... vbox x 40..261 y 50..51" (log line #18728).
- Screen mapping for this scissor: screen x = 240 - game y, screen y = 40 + game x. Edge at f588: screen y 303 = game x 263, log value 261 (2 units off, mapping approximate).
- Not yet known: which source function emits this strip set (the 7-unit strips, combine 8, prim b4). The N64 draw order is not verified. The 3D world raw batches (gi 0 to 159) are static from f584 to f598 and do not move the edge.
- No code fix. No commit of code. The box is still visible (f588 to f601). Map restore file: 3ds/src/map_restore_table.c was regenerated by gen_map_restore.py before the build. The copy made at 17:19 was not saved first, and it is not restored. The file stays uncommitted.
- SPEC for the next agent (fix, expected above 30 changed lines until the emitter is found):
  1. Find the caller of the 2D strip loop in src/ that uses prim 180 (b4), combine 8, 7-unit strips from game y 50 to 212, and a moving right x. Search for the loop that steps y by 6 or 7 and sets x1 from a variable.
  2. Compare with the N64 decomp at the same function. Name the N64 value of the right x at frames 588 to 601, and the frame where the N64 draw ends.
  3. The fix must make the 3DS strip right x match the N64 value at each frame, or stop the strip draw at the frame where the N64 stops it. One change, same row.
  4. Test: PM_ROW=box3 (budget 1 of 8 used), shotwin=327:300, Until 'shot \d+ frame 6[5-9][0-9]'. Pass: frames 588 to 601 show no hard edge at y 303 to 86.

### Box3 follow-up: emitter found (no code change yet)
- The 7-unit strip set is `appendGfx_image_ci` in `src/world/area_hos/hos_05/intro.c:1403-1424`. It draws the story page image (264 wide, 162 tall, CI, 23 strips of 7 rows plus one remainder row) at `vpX + backImgPosX/frontImgPosX` and `vpY + ...PosY`, from `worker_draw_story_graphics` (intro.c:1426-1475). Strip rows y 50..212 match this image.
- Called with combine PM_CC_43 and prim 208,208,208 alpha = back overlay alpha when `overlayAlpha != 0` (intro.c:1442-1446). This is where the grey panel and the strip set meet.
- Open: the logged right vbox x (303, 261, 219, 168, 86) is not 264 wide from a fixed base x 40. So the right edge shrinks per frame. Either the logged vbox is clipped by the texture rectangle, or the positions change per frame. Check `StoryGraphicsPtr` backImgPosX/frontImgPosX and the story page alpha (`storyPageAlpha`, `tapeAlpha`) over frames 584 to 601 in the log (add one log line in `appendGfx_image_ci`).
- Log run (box3, 2 of 8 starts): shots 0000 to 0006 = frames 580, 584, 588, 591, 594, 598, 602. PNGs: 3ds/logs/perf/box3/box3_f580_shot0000.png, box3_f584_shot0001.png, box3_f588_shot0002.png, box3_f591_shot0003.png, box3_f594_shot0004.png, box3_f598_shot0005.png, box3_f602_shot0006.png.
- Log line added in the port layer: 3ds/src/pm_gbi.c G_TEXRECT case, "box texrect f<frame> x0 x1 ... prim ... oml omh" (boxlog, frames 580 to 602). appendGfx_image_ci was not edited.
- The story image strips (y 50..217, 7-row) are texrects x 12..111 and 112..307, constant over frames 580 to 602. They are NOT the moving edge.
- The moving edge is a different texrect: rows y 56..63 (8 rows), x0 0, x1 moving: f584 263, f588 221, f591 179, f594 128, f598 46. Its mode is oml 0f0a4200 omh 00008cc0, prim b2b2b2ff. Its x0 29 x1 293 companion (same rows) is constant.
- Emitter of the moving 8-row texrect: NOT in src/world/area_hos/hos_05/intro.c (grep finds only the 1388, 1413, 1420 texrects there). Candidates to read next: src/background.c, src/main.c, src/state_logos.c, src/draw_img_util.c. Find the caller with x0 0 and a shrinking x1.
- Decision needed: is the shrinking x1 the N64 page wipe? Decide from the N64 value of x1 per frame (the game variable that sets it on N64, same decomp code), not from the picture. The port emits the same x1 as the game code computes, so a difference only appears if the 3DS draws the texels differently.
- Blocker for the fix: the N64 value that ends the story image draw is not yet named.
- Exact commands for the next agent (worktree root, PowerShell):
  1. Find the emitter: grep -n "x0 0\|gSPScisTextureRectangle" src/background.c src/main.c src/state_logos.c src/draw_img_util.c, and the callers of the texrect helper in src/world/area_hos/hos_05/intro.c.
  2. Build: python 3ds/tools/gen_map_restore.py, then $env:PM_LANE='box3'; & 'C:\Program Files\Git\bin\bash.exe' 3ds/build.sh (map_restore_table.c stays uncommitted).
  3. Shot: $env:PM_ROW='box3'; $env:PM_LANE='box3'; powershell -File 3ds/run_azahar.ps1 -Until 'shot \d+ frame 6[5-9][0-9]' -Timeout 90 -LockWait 180 -Debug 'shots shotwin=580:24 loglevel=debug boxlog'. Budget: 2 of 8 used.
  4. PNGs: python on the shots in build3ds/lane-box3/shots (PPM) to PNG. Saved copies: 3ds/logs/perf/box3/box3_f580_shot0000.png to box3_f602_shot0006.png (git-ignored, on disk only). The next step is that log line, then compare with the N64 `worker_draw_story_graphics` flow.

## Run log (box5 agent, 2026-10-27)
- Step 3 edit: `3ds/src/pm_gbi.c` texrect case, line 2965 (boxlog). Filter now also needs `yl/4 == 56` and `yh/4 - yl/4 == 8`. Extra fields: cmd (pc-3), list (pm_gbi_run list start), sp, st0 and st1 (stack[sp-1], stack[sp-2]: return list addresses), tex (timg).
- Hypothesis to test: the 8-row texrect at y 56..63 is a G_TEXRECT inside a list whose address the log names. Its x1 shrink (263, 221, 179, 128, 46 at frames 584, 588, 591, 594, 598) is the moving edge.
- Next: build PM_LANE=box5, one Azahar start (PM_ROW=box5, -Timeout 90, -LockWait 180, Debug 'loglevel=debug boxlog shots shotwin=580:25', Until 'shot \d+ frame 604'). Read the cmd and list addresses from the log, then map them to the emitter.
- Build box5 ok (3ds/build.sh, PM_LANE=box5). No .bak file existed, so no restore step. map_restore_table.c stays modified and uncommitted.
- Azahar start 1 of 8 (box5): PM_ROW=box5, -Timeout 90, -LockWait 180, Debug 'loglevel=debug boxlog shots shotwin=580:25', Until 'shot \d+ frame 604'.
- State: boxlog edit done (pm_gbi.c:2965). Build OK in lane box5. Azahar run denied by the permission classifier. Not retried. Next step: the run.
