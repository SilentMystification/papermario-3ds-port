# V14 diagnosis: fortune read in mac_01 (read only, 2026-10-18)

## Method
- Build: build3ds/pm_3ds.3dsx (13:41, has coins50). Command: `pwsh 3ds/run_azahar.ps1 -Turbo -Timeout 400 -Until "smoke done" -Debug "fast warp=mac_01:0 story=ch1_kooper loadslot=0 coins50 padscript=@pad_t01b3.txt shots shotwin=600:1100 smokeframes=1800 smoke"`.
- The pad file is pad_t01b3.txt in sdmc/3ds/PaperMario (the B3b walk to Merlon, then A taps).
- One contact sheet, 12 frames, f600 to f1645 (scene frames). The read runs from about f600 to f1000. After f1150 only the hint text shows.
- Run notes: the run script deletes the shots folder at the start of every run. Copy the folder in the same command that runs Azahar.
- No source was edited. Frame numbers below are pm_frame_count.

## Faults
| # | Frame | Fault | Cause | R13b fixes it |
|---|-------|-------|-------|---------------|
| F1 | f710, f765, f875 | Black squares (some with a white dot) beside the ball and Merlon, in place of the three flames | Effect EFFECT_MOTION_BLUR_FLAME (src/effects/motion_blur_flame.c lines 241 to 268) copies the frame buffer with gDPLoadMultiTile into tile 1 and draws a 2 cycle texrect. R13b log 12:40 to 14:50: the port reads TEXEL1 as black. The cycle 2 TEXEL0/TEXEL1 order and the TEXEL1 uv window are the open causes. | YES, this is the R13b row. Do not work on it here. |
| F2 | f765, f875 | Hard edged white wedge (a light cone polygon) from the top left corner, crosses the room | Not confirmed. Likely the spotlight model of the room, drawn with a combiner or render mode that the port draws opaque white instead of translucent. Needs a grab of the draw at f765. | NO |
| F3 | f710 to f930 | Mirror ball shows coarse pastel blocks with no sparkle | Port texgen is done in pm_gbi.c vert_color (T01, 7e174174b). crystal_ball_gfx.c gfx_build_crystal_ball_pre sets gDPSetHilite1Tile and gDPSetHilite2Tile. grep shows pm_gbi.c and pm_gpu.c have no hilite code, so the tile origin that guLookAtHiliteF computes is not used. Block look may also be the real texture. Compare to a decomp reference shot before a fix. | NO |
| F4 | f710, f765 | Stars (merlin_house_stars) look grey and blocky at the edges | Uncertain. Effect uses PM_CC1_MERLIN_STARS two cycle combiner (merlin_house_stars.c line 151) with tile 1 (line 160). It may share the F1 TEXEL1 cause. | MAYBE |

Frames f600, f655, f930 to f985 show the crystal ball (decal inside), glow wave and the room tint without a visible fault.

## Fix plan
### F1 (R13b)
Wait for R13b. Test after: same run command, shots at f700 to f880, flames must be orange.

### F2 spotlight wedge
1. Grab the draw: add `imglog` and `xtrace=765:2` to the run, shotwin=765:2 (no source edit). Read which display list draws the wedge (rm word, combiner, vertex alpha).
2. Expected change (unconfirmed): pm_gbi.c, blend path for the render mode of that list (xlu with vertex alpha) so alpha is kept. File and function are decided after step 1.
3. Test: same run command, compare f765.

### F3 hilite
1. Read decomp: include/PR/gbi.h gDPSetHilite1Tile and src/os guLookAtHiliteF (hilite.h.x1, y1 set the SetTileSize origin).
2. Check pm_gbi.c handling of G_SETTILESIZE: it must use the hilite tile origin as tile_x0, tile_y0 for the texgen vertex uv (vert_uv, line 575 area).
3. Change: in pm_gbi.c vert_uv, tile_x0 and tile_y0 come from the tile size of the active tile. Verify the SetTileSize cmd writes both tile 0 and the render tile. Do not add a hilite special case.
4. Test: same run command, shotwin=700:1, crop the ball. Compare to a real N64 capture of Merlon mirror ball if the user has one. Then `python 3ds/tools/golden.py yard toad` (texgen is used only here, so no other scene should change).

### F4 stars
Retest after R13b. If the stars stay grey, grab f710 with imglog and compare cycle 2 combiner terms as in R13b 12:40.

## Order
R13b first (F1, maybe F4), then F2 (grab), then F3.

## Result (2026-10-18, no code change)
- Runs: 3 Azahar starts. Build pm_3ds.3dsx 19:54. drawlog=765:1 and drawlog=875:1, shots f700 to f889. Shots are 240x400, rotate 90 degrees counter clockwise to read them.
- F2: the only hard edged white wedge in f765 and f875 is the tail of the speech balloon (the balloon covers the top of the screen and points at Merlon). The game draws it (message box). Draw list of f765 and f875 is the same 24 chunks. No white or opaque light cone chunk exists. The light cone is chunk #19/#20 (IA 32x16, oml 0c184a50, prim f8f0f000). It shows as a soft cyan beam in f835. F2 is intended art. No change.
- F3: the pastel blocks are the wall and floor chunks #1 to #3 (I4 32x32 texture, 16+4+8 tris, box 0..312 x -23..177) enlarged by the room. The mirrorball texgen path runs (T01) and gives smooth arcs. The blocks are nearest sampling: pm_tex.c line 437 sets GPU_NEAREST for every game texture. The N64 uses G_TF_BILERP (omh 00082c00). Root cause is the port wide texture filter, not texgen or hilite. New row V15.
- F4: the stars at the wall edges are the same 32x32 textures with nearest sampling. R13c fixed the TEXEL1 part. Same row V15.
