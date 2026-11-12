# Row S1PREV: the previous-frame overlay as one native rectangle record (NATIVEAPI B3)

Model: Sonnet (user decides). Why not Haiku: the FB1 bind arm (a clamp window of the previous colour buffer bound as one 320 x 240
texture, nd_tex_set.c:384-412) must be reached with an NdTexDesc from a record, not from the tile state, and the result must stay
byte-equal across the strip-to-rect change. Why not Opus: one producer, one bind arm, one rect.

## Blocker and base
NAPI2B merged (nd_tex_bind_desc, nd_direct_bind, nd_direct_end). `sh 3ds/tools/wt_new.sh S1PREV`; PM_ROW=S1PREV PM_LANE=S1PREV.

## Readers and writers
- Producer: appendGfx_draw_prev_frame_buffer (src/screen_render_util.c:30-77), called by draw_prev_frame_buffer_at_screen_pos
  (:79), which is called by _render_transition_stencil OVERLAY_START_BATTLE (screen_overlays.c:350) and OVERLAY_BLUR (:357).
- Walker side today: the strips (6 rows, gDPLoadTextureTile of the previous cfb + gSPTextureRectangle with s = x * 32, t = y * 32,
  dsdx = dtdy = 1024) take ensure_tile_slow's FB1 arm (fb_prev_base): the first strip binds the whole previous buffer and the others
  are bind hits; tile_x0 = tile_y0 = 0 (absolute texels).

## Design
1. A record NdPrevRec { s32 x1, y1, x2, y2; u32 alpha; } recorded by nd_rec_napi under TARGET_3DS in place of the state lines and
   the strip loop (the state lines :50-61 become part of the record's pso: PM_CC_10 combine, G_RM_CLD_SURF render mode, 1-cycle,
   point filter, no perspective, TLUT none; prim 255 255 255 alpha).
2. Runner: nd_direct_bind(pso, prim, 0); nd_tex_bind_desc with the previous cfb (the same buffer choice as :41-45, done on core 1 and
   carried in the record as the address) as a 320 x 240 RGBA16 image, clamp, so the FB1 arm binds it; one pm_gpu_tex_rect over
   x1..x2, y1..y2 with s = x, t = y (texels, 1:1); nd_direct_end().
3. One rect gives the same texels as the strips: every pixel maps to texel (x, y) in both (dsdx 1.0, absolute origin). Expected
   byte-equal; if the 3DS rect rasterizes the strip seams differently, the rounding rule applies only with the cause named.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- A det scene that draws OVERLAY_BLUR or OVERLAY_START_BATTLE: pre-flight finds none in azt_scenarios.json. Add `prevfb_btl_det`:
  `warp=kmr_03:0 story=ch0_toad_town battle battlef=100 shots shotwin=100:12 det` (the battle start wipe draws the previous buffer
  in OVERLAY_START_BATTLE), until `shot \d+ frame 111\b`, cap 45. Base at the merge-base, then the row: every shot byte-equal.
- Census: rect count in the rect census line (`rect f... tex`) drops by (strips - 1) per overlay frame. Hand back both lines.
- Budget 6 Azahar starts.

## Stop
A byte difference without a named rounding cause: stop, commit, hand back the shots and counts. Delete this row file in the
completing commit.
