# Row S5TITLE: title screen and draw_img_util images as direct texture rectangles (NATIVEAPI S5)

Model: Sonnet (user decides).
- Why not Haiku: the 4 draw_img_util functions load CI and RGBA images in strips of TMEM size, with clipping. Each loop needs its
  own window arithmetic for nd_tex_win_desc, which this row works out from the loop. The oracle has not read each loop to the
  value, and a Haiku row without those values would be an open task.
- Why not Opus: the HUD rectangle pattern (NAPI2B) covers every draw.

## Blocker and base
NAPI2B merged (done). Can start when a slot frees: no file overlap with the running rows.
`sh 3ds/tools/wt_new.sh S5TITLE`; PM_ROW=S5TITLE PM_LANE=S5TITLE.

## Code read (gbi calls per function)
- src/state_title_screen.c: appendGfx_title_screen 17, title_screen_draw_menu 15, title_screen_draw_copyright 11,
  title_screen_draw_logo 5.
- src/draw_img_util.c: draw_ci_image_with_clipping 14, draw_adjustable_tiled_image 5, draw_image_with_clipping 4,
  draw_tiled_image 3. Callers: the title screen, logos, file select and pause pictures (grep the callers first and list them in
  the commit).

## Design
- Per strip or tile: an NdBindDesc from nd_tex_win_desc (image, palette for CI, strip window, wrap and masks of the load); the
  rectangle words through ND_W0_TR etc.; nd_direct_rect.
- The strips may become one rectangle per image only if the byte gate stays equal. Default: keep the strips, which are exact by
  construction.
- One pso per function (nd_pso_n64 of its combine and render words).
- Fallback (old ops) when nd_direct_ready refuses, counted.

## Files and places
src/state_title_screen.c, src/draw_img_util.c (the functions above only).

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- Pre-flight: `bench.py list --tag pic` for a det title scene. If there is none, add `title_det` (title screen, `det shots
  shotwin=<frame of the title menu>:4`, until the shot line; take the frame from the boot_logo or title scenario logs) in the
  same commit.
- One list with bases: title_det (or the scene found) and pause_tab_det_pic. 2 starts. Every shot byte-equal.
- listcen: build down by the moved calls. Budget 6.

## Stop
A byte difference: stop, commit, hand back. Delete this row file in the completing commit.
