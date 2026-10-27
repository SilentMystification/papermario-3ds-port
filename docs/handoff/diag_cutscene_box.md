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
