# Row S5ITEM: item entities (world and UI) drawn by native records (NATIVEAPI S5)

Model: Sonnet (user decides).
- Why not Haiku: appendGfx_item_entity loads one CI8 raster and reads it through two tiles (tile 1 as CI4 with a tile-size
  offset, tile 2 as RGBA16 for the palette), with the RM1/RM2 alpha-test lists. Stating that needs the tile-1 helper of S2W and
  a compiled state range of S6T.
- Why not Opus: both helpers exist when this row starts.

## Blocker and base
S6T and S2W merged (tile 1 bind nd_direct_tex1, compiled state ranges). `sh 3ds/tools/wt_new.sh S5ITEM`; PM_ROW=S5ITEM PM_LANE=S5ITEM.

## Code read (src/item_entity.c; gbi calls per function)
- appendGfx_item_entity 49: a matrix; Gfx_RM2_ALPHATEST or Gfx_RM1_ALPHATEST; clear geometry; the setup list D_8014C620; render
  mode and combine PM_CC_2F/32 with prim alpha in the transparent case; LoadTLUT_pal16; CI8 SetTextureImage, LoadTile 0..0x2E x
  0..0x5C; tile 1 CI4 with tile size 0x400..0x45C; the render tile; tile 2 RGBA16 at 0x100 with tile size 0..0xFC; then
  nd_rec_ent_draw(D_8014C678) (DIRECT D3).
- render_item_entities 49: the UI and HUD item draws.
- draw_coin_sparkles 16.
- napicen3 mac01: item_entity.o 57 ops a frame.

## Design
1. The state part (render-mode list, setup list, geometry, combine choice) becomes one compiled range from S6T, keyed by (alpha
   test 1 or 2, transparent or not). prim alpha is a value.
2. The texture part becomes NdBindDesc for tile 0 and tile 1 (nd_direct_tex, nd_direct_tex1). The palette goes in as the
   descriptor's pal. Check how the walker binds tile 2 (RGBA16 at tmem 0x100) today: if no draw reads tile 2, say so and drop
   it. Otherwise state it as tile 2 bookkeeping.
3. The entity draw record (NDC_ENT) stays. render_item_entities and draw_coin_sparkles: the same pattern per draw.
4. The fallback (old ops) is counted and kept for walker states the helpers refuse.

## Files and places
src/item_entity.c (the 3 functions). No edit to nd_api.h or nd_direct.c, unless the orchestrator confirms no parallel append.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- One list with bases: spot_mac01_det (coins and items in mac_01), pause_tab_det_pic (item icons). 2 starts. Every shot
  byte-equal.
- napicen_mac01 (1 start): item_entity.o 0 or the counted fallback only.
- listcen: build down by the calls moved. Budget 6.

## Stop
A byte difference: stop, commit, hand back (2 tries per named cause). Delete this row file in the completing commit.
