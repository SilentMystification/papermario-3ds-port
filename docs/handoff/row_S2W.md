# Row S2W: draw_box windows (flat, non-ROTSCALE) drawn directly with two tiles (NATIVEAPI D2/E3)

Model: Sonnet (user decides).
- Why not Haiku: the window is a 2-cycle draw of two textures: the background pattern in tile 0 and the corner pieces in tile 1
  (gDPLoadMultiTile to tmem 0x100). The direct helpers of NAPI2B refuse exactly this:
  - nd_pso_decide returns NULL when the plan uses tex1 or the box pattern (dec_make, nd_direct.c, `e->ok = ... && !e->out.pat`);
  - nd_direct_ready refuses an off-screen tile 1.
  This row extends the helpers to tile 1 and to the box-pattern product (box_pat, box_prod, box_folded in nd_comb.c:24, :176-187).
  Every window in every scene passes through it, so a wrong arm shows everywhere.
- Why not Opus: the decision code exists (nd_comb.c box arms). The row states its inputs from the record, not from the walker
  state.

## Blocker and base
NAPI2B merged. FSELFIX (the ROTSCALE one-quad change) is not needed: this row leaves the ROTSCALE branch (draw_box.c:471-505 and
the contents pass :768-828) on the old path. The ROTSCALE contents pass waits for the S6T answer on off-screen targets.
`sh 3ds/tools/wt_new.sh S2W`; PM_ROW=S2W PM_LANE=S2W.

## Code read (src/draw_box.c draw_box :340-836)
- State:
  - PipeSync, 2-cycle, blend colour, fog colour (darkening), render mode PM_RM_SHROUD / XLU_SURF2, scissor;
  - the combine word from gBoxCombineModes or the custom style (:448-453 on 3DS, nd_gx_w);
  - filter point, texture on, LUT none, detail clamp, LOD tile, prim and env colours.
- The background image: LoadTextureTile of 4, 8, 16 or 32 bit, wrap, masks (:515-529); tile size, scrolled when
  ANIMATED_BACKGROUND (:531-542).
- The corners: 4 x LoadMultiTile to tmem 0x100, tile 1, clamp (:545-), each followed by a gSPScisTextureRectangle. The edges and
  the middle are rectangles that sample tile 0 (the pattern) and tile 1 (the corner or edge piece). Read the rest (:560-762)
  before the design: the count of rectangles and which tile each one reads.
- napicen3: draw_box.o 144 ops a frame (mac01 and btl), 99 (talk).

## Design
1. Helpers in nd_direct.c:
   - `int nd_direct_tex1(const NdBindDesc* d)`: tile 1 = d through nd_tex_bind_desc(d, 1), with the tile-1 bookkeeping
     set_tilesize leaves for tile 1;
   - the decision for 2-cycle box combines. The key gets box_prod, box_pat and box_folded from the record (the pattern image is
     white or not). The decision path is the walker's own nd_comb_decide with those inputs set in the NdCombIn, not read from
     the walker.
2. Record NdBoxRec: the pso fields (combine words, render mode, darkening as fog colour, prim, env), the background NdBindDesc
   (tile 0 with the scroll as the window origin), the 4 corner NdBindDesc (tile 1), and the rectangle words. The rectangle words
   are computed on core 1 with the ND_W0_TR / ND_W1_TR / ND_TR_H1 / ND_TR_H2 macros (nd_gbi.h:201-204) from the same expressions.
   The record size is about 4 + 5 desc + 9 rects: check that it fits nd_rec_napi's 255 units.
3. Runner: nd_direct_ready, nd_direct_bind (2-cycle form), the tile 0 bind, per corner the tile 1 bind and its rectangles,
   nd_direct_end. Fallback: the old ops into a local list and nd_walk_state (as hud_rect_run).
4. Windows with contents (fpDrawContents) draw their contents after the box. Contents are other producers and stay as they are.

## Files and places
src/draw_box.c (the non-ROTSCALE path of draw_box only: `quads == nullptr`), 3ds/src/nd_direct.c (new helpers at the end),
3ds/include/nd_api.h (declarations at the end). Pre-flight: grep `box_pat` and `box_folded` readers (nd_comb.c:24, :50, :176,
:187, :439, :452, :460, :470, :481, :513) and keep every one consistent after a direct box draw (nd_direct_end must leave
box_pat and box_folded as the walker would, or reset them as the next SETCOMBINE does).

## Gate (expected values)
- Build, memcheck OK; nd_host PASS (host case: the 2-cycle box decision through the record inputs equals the walker decision for
  the 4 default styles).
- One list: pause_tab_det_pic, msgbox_talk_nowide_pic (if it is not det, use the talk det scene; pre-flight `bench.py show`),
  spot_mac01_det (3 starts), with bases at the merge-base. Every shot byte-equal.
- napicen_mac01 (1 start): draw_box.o drops to the ROTSCALE windows only (hand back the numbers).
- Budget 8 Azahar starts.

## Stop
A byte difference: stop, commit, hand back (2 tries per named cause). Delete this row file in the completing commit.
