# Row S5BG: the background task (gfx_draw_background, gfx_init_state) and scrolling backgrounds as native records (NATIVEAPI E6)

Model: Sonnet (user decides).
- Why not Haiku: gfx_draw_background has several modes:
  - the pause picture with blur strips that read the previous frame (S1PREV's FB bind);
  - the z-buffer clear fill;
  - the background colour fill;
  - the frozen-frame copy;
  - the DISABLE_DRAW_FRAME early return (background_gfx.c:446-449, which INTERPB3 depends on).
  appendGfx_background_texture draws the scrolled strips with wrap windows.
- Why not Opus: each mode maps to an existing helper (fill, FB rect, tex rect) or to an S6S range.

## Blocker and base
S1PREV, S6S and S5PQUAD merged (FB bind arm, compiled state ranges, nd_fill_run with kinds).
`sh 3ds/tools/wt_new.sh S5BG`; PM_ROW=S5BG PM_LANE=S5BG.

## Code read (gbi calls per function)
- src/background_gfx.c: gfx_draw_background 56, func_80027BAC 11, gfx_init_state 3.
- src/background.c: appendGfx_background_texture 23, bg_emit 2 (already partly native: bg_emit).
- napicen3: background.o 36, background_gfx.o 17-25.

## Design
1. Fills (the z-buffer clear into nuGfxZBuffer, the bgColor fill) become nd_fill_run kinds. The z-image fill is an off-screen or
   depth target: keep it as ops if nd_direct_ready refuses it, counted. The S6T design question 3 answer decides it.
2. The pause picture strips become S1PREV's direct FB rectangle helper.
3. appendGfx_background_texture: each strip becomes a direct texture rectangle with the scroll as the window origin (TILEWIN2
   rule), using nd_tex_win_desc with the wrap masks of the load.
4. Keep the DISABLE_DRAW_FRAME return and its SetColorImage exactly (INTERPB3's unpatched copy counts on a background-only frame
   drawing nothing).

## Files and places
src/background_gfx.c (gfx_draw_background, func_80027BAC), src/background.c (appendGfx_background_texture). gfx_init_state is S6S's.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- One list with bases: spot_mac01_det (mac_01 background), pause_spin_grab_det_w300 (pause picture), xfer_map_shots (map change,
  background task only frames). 3 starts. Every shot byte-equal.
- napicen_mac01: background.o and background_gfx.o down to the counted fallbacks. Budget 6.

## Stop
A byte difference: stop, commit, hand back. Delete this row file in the completing commit.
