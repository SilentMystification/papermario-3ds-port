# Row S5TTL: title screen images through the NdTRect record (S5 call sites)

(Model: Haiku. The pattern edit of row_S5TIMG.md, applied to one file. Blocker: S5TIMG merged: its step 0 adds `pad` bit 0 =
keep the walker's prim; set `r.pad = 1` at a site where the function sets no PrimColor before the pass.)

## Start
`sh 3ds/tools/wt_new.sh S5TTL`; PM_ROW=S5TTL PM_LANE=S5TTL. Only src/state_title_screen.c is edited. Start after S5TIMG merges.

## Sites (main ccdb7e3f5; re-grep `gDPLoadTexture\|gSPTextureRectangle\|gSPScisTextureRectangle` first, the lines may move)
| function | load | rect |
|---|---|---|
| the logo loop (:504-520) | :510 gDPLoadTextureTile RGBA 32b | :518 gSPScisTextureRectangle |
| title_screen_draw_menu | :619 gDPLoadTextureBlock IA 8b | :623 gSPTextureRectangle |
| title_screen_draw_menu | :629 gDPLoadTextureBlock IA 8b | :633 gSPTextureRectangle |
| title_screen_draw_menu | :639 gDPLoadTextureBlock IA 8b | :642 gSPTextureRectangle |
| title_screen_draw_copyright | :684 gDPLoadTextureTile_4b CI (after LoadTLUT_pal16 :683) | :688 gSPTextureRectangle |
| title_screen_draw_copyright loop | :695 gDPLoadTextureTile IA 8b | :699 gSPTextureRectangle |

## Exact edit per site (under `#ifdef TARGET_3DS`; `#else` keeps the old lines). Same rules as row_S5TIMG.md step 2.
- Keep every state op: the TitleSetupGfx calls (:504, :613, :667), the combine, TLUT and render mode lines, and the LoadTLUT.
   The S5TRECT fallback (nd_trect_run, nd_direct_ready() != 1) replays only the texture load and the rectangle: it relies on the
   state ops and the LoadTLUT kept in the stream. Remove none of them; the record carries the palette only for the direct path.
- Load and rect pairs become one NdTRect record:
  - gDPLoadTextureTile(img, fmt, siz, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, ...): copy the arguments.
  - gDPLoadTextureBlock(img, fmt, siz, width, height, pal, cms, cmt, masks, maskt, ...) loads the whole image: uls 0, ult 0,
    lrs width - 1, lrt height - 1.
  - The _4b form: siz G_IM_SIZ_4b.
- Rect words:
  - gSPTextureRectangle: ND_W0_TR, ND_W1_TR, ND_TR_H1, ND_TR_H2 with the call's expressions.
  - gSPScisTextureRectangle: copy the word arithmetic of hud_rect_direct in src/hud_element.c (the PM_SCIS_* form of w0, w1
    and hs), with this call's arguments.
- pso:
  - cw0 and cw1: the combine the function set before the pair. Where it comes from TitleSetupGfx, read that list for its
    combine and render mode and use those values.
  - oml: the render mode pair.
  - omh: TLUT (G_TT_RGBA16 for the copyright CI image, else G_TT_NONE) plus filter (from TitleSetupGfx).
  - prim: the prim set before the pair, else nd_direct_prim() through the runner.


## Merge rule (orchestrator, 2026-11-12)
Rows that append to the end of 3ds/include/nd_api.h, 3ds/src/nd_direct.c or 3ds/tools/nd_direct_test.c conflict when two merge in
sequence. Right before the gate runs: `git merge main`; at a conflict at those file ends keep both appends (main's first, then
yours), rebuild, nd_host; then take the bases at the new `git merge-base HEAD main` and run the gate.

## Gate
- Build, memcheck OK; nd_host PASS.
- A det title scene. Pre-flight `bench.py list --tag pic` and `bench.py show boot_logo_pic`. If none is det, add `title_det`, a
  copy of the title scenario with `det shots shotwin=<F>:4`. F is the frame of the `game mode 1` (title) line of a boot log in
  the base store (build3ds/basecache/*/spot_mac01_det/run.log has the boot lines) plus 60. Until `shot \d+ frame <F+63>\b`.
  Add the bench in the same commit.
- `bench.py base title_det`; one run (1 start); `bench.py cmp`: every shot byte-equal. No `trect fallback` lines. Budget 4.

## Stop
A byte difference: stop, commit, hand back. Delete this row file in the completing commit.
