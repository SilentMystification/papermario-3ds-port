# Row S5SPR: the state ops around sprite records go into the NDC_SPR record (NATIVEAPI E2)

Model: Sonnet (user decides).
- Why not Haiku: the NDC_SPR record layout changes, so every reader and writer must move together (the list below). The sprite
  path also has the shadow, pause-viewport and imgfx arms.
- Why not Opus: the record and its native draw exist (DIRECT D6). This row moves the state ops in front of it.

## Blocker and base
NAPI2B merged (nd_pso_n64, nd_direct_bind for the state decision). `sh 3ds/tools/wt_new.sh S5SPR`; PM_ROW=S5SPR PM_LANE=S5SPR.

## Code read (src/sprite.c spr_appendGfx_component_flat :234-369)
- Per sprite component, before the record:
  - gDPLoadTLUT_pal16 and the raster load: gDPScrollMultiTile2_4b, or gDPScrollTextureBlock_4b, plus a SetTile and SetTileSize of
    tile 2;
  - for the shadow pass: gSPViewport (pause or camera vpAlt), the render mode (an AA ZB pass mode or G_RM_PASS/ZB_CLD_SURF2), env
    alpha, combine PM_CC_0B/0C or PM_CC_0A, gSPVertex plus 2 triangles, PipeSync;
  - for the main pass: render mode, env (100,100,100,255), prim alpha, combine PM_CC_3D, colour dither;
  - viewport back.
- The final vertex and triangle ops go with the record (spr_geo_out, DIRECT D6, :325-333 under TARGET_3DS). napicen3 mac01:
  sprite.o 96 ops a frame (spr_appendGfx_component_flat 78, spr_appendGfx_component 18).
- Readers and writers of NDC_SPR (grep `nd_rec_spr`, `NDC_SPR`, `pm_gpu_spr_build` before the edit; list them in the commit):
  nd_rec.c nd_rec_spr (writer), nd_walk.c (the NDC_SPR case), pm_gpu.c pm_gpu_spr_build (:3950 region, the native quad), and
  sprite.c (both component functions).

## Design
1. The record carries a state block: the pso (nd_pso_n64 of the render mode and combine pair of each pass), env and prim, the
   viewport choice, and the texture as an NdBindDesc (CI4 raster plus palette; the TLUT load as the descriptor's pal).
   - Core 1 fills it in place of the ops.
   - Core 0 binds by pso id and NdBindDesc (nd_direct_bind and nd_direct_tex), then runs the native sprite draw as today, then
     nd_direct_end.
2. The shadow pass is a second draw in the same record (a flag and its pso).
3. The imgfx arm (spr_appendGfx_component with imgfx) stays on the old path until S5IMGFX.
4. Fallback: a component the record cannot state keeps today's ops.

## Risks
- Tile 2 (SetTile/SetTileSize of tile 2, :248-249) is part of the walker state that later draws may read. The record must leave
  it the same: write nd_ws.td[2] and tile_sz as the ops would, or show that no later op reads tile 2 before it is set again.
- Interp twin: the sprite matrix path is unchanged.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- DIRECT byte set, one list: spot_mac01_det, mdl5_iwa10_det, spot_mac00_det (3 starts). Every shot byte-equal.
- napicen_mac01 (1 start): sprite.o drops to the imgfx arm only (hand back the per-function numbers).
- Budget 8 Azahar starts.

## Stop
A byte difference: stop, commit, hand back (2 tries per named cause). Delete this row file in the completing commit.
