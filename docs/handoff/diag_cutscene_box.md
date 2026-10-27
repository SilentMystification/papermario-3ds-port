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

## Run 3 result (Azahar start 4 of 8, row box, raw NDC extent log)
- The "box raw" line is written before its "box draw" line. The raw extent belongs to the next draw line.
- NDC check (target 240 x 400, shot 0000): panel edge pixel x 50 = NDC -0.583, x 210 = NDC +0.75. Pixel y 70 = NDC +0.65 (flipped), y 328 = NDC -0.64.
- Match: raw batch gi 192 n 24 (frame 327) has NDC x -583..758, y -649..649. Its draw line is #8315: graw 1, combine 8, tex 1, prim c5c5c5ff, blend 1, rep 0, z 0.
- The grey panel is the d0d0d0 opaque full-screen fill (#8305, blend 0, rep 0) visible where raw batch #8315 paints nothing. Panel edge = extent of #8315.
- Not yet proven: whether #8315 texels are transparent inside the panel. Texture id and texel alpha are not logged.
- Next: log the texture id and texel alpha inside the #8315 extent. No fix committed yet.

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
