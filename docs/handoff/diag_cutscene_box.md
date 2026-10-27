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
