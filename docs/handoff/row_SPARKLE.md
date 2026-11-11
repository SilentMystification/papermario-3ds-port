# Row SPARKLE: star spirit sparkles split into one GPU draw per part (lag)

(Model: Sonnet. Reason: the per-frame vertex copies need the port's rule for vertex data that a recorded list reads later (the stream
replays after the producer returns): a choice between the display context and a frame pool, which Haiku cannot make. Why not Haiku: that
choice. Why not Opus: one file, no cross-thread design. The user decides.)

## User facts (bugs only)
User (2026-11-11, console): "When all the star spirits poof in the game, it lags. It's all the sparkle effects; something is
inefficient with them."

## Cause (oracle code read)
- Producer: hos_01 (Shooting Star Summit) npc.c:107-122 calls N(DrawAppearSparkles) 6 times in one step: fx_sparkles(0, x, y, z, 10).
  sparkles.c:56 type 0 has numParts 36 (+1 header): 216 parts on screen, render mode RENDER_MODE_CLOUD_NO_ZCMP (sparkles.c:240).
- Per part, sparkles_appendGfx (src/effects/sparkles.c:264-284) writes: gDPSetPrimColor (a new colour each part, colorIdx += 3 over
  12 colours), gSPMatrix push MUL, gSPDisplayList(D_E0022CD0[k]) (4 vertices, 2 triangles: assets/us/effects/gfx/D_09001210_3391D0
  .gfx.inc.c and the 7 others), gSPPopMatrix. The combine (D_09000F20_338EE0: 1, PRIM, TEXEL0, PRIM / PRIM, 0, TEXEL0, 0) reads PRIM.
- Port cost per part (ranked): 1. the PRIM change ends the GPU batch: the combine signature changes (sync_combine, nd_comb.c:566),
  the TEV constant changes, the ring batch closes (pm_gpu.c:2830 gtrip_params_same / flush) and nd_raw_native (pm_gpu.c:1308) runs
  nd_tev_build again (the tk memo misses on prim, pm_gpu.c:1350-1355) and submits one draw: 216 draws and 216 TEV builds per frame.
  2. gSPMatrix MUL per part: load_mtx, the combined product and gv_matrix_id (nd_mtx_set.c:54, a new table entry per part, 256 per task:
  past it the part's vertices get no ring slot and its triangles drop, nd_geom.c:324-325). 3. the G_DL per part: walk_call, dlc_try
  lookup and a 4-vertex ring copy (nd_shape.c:361-381). The texture is one atlas (176x22 IA8, loaded once): no load per part.
- Other running rows: HC6 (pm_gpu.c model key/hit/submit, nd_recs.c, nd_model.c, nd_emit.c, nd_rec.c nd_rec_model) does not touch
  sparkles.c. This row edits only src/effects/sparkles.c.

## Start
`sh 3ds/tools/wt_new.sh SPARKLE`; PM_ROW=SPARKLE PM_LANE=SPARKLE on every call.

## Touch only
src/effects/sparkles.c: sparkles_appendGfx (:245-288), a TARGET_3DS block; a table of the 8 vertex arrays above it. No 3ds/src file.

## Exact edits (TARGET_3DS block; the #else keeps the N64 code)
1. Above sparkles_appendGfx: `static Vtx* const sparkle_vtx[8]` = the vertex array of each D_E0022CD0[k] list, in the same order
   (read the first gsSPVertex of each of the 8 .gfx.inc.c files named in src/effects/gfx/sparkles.c:16-23).
2. In sparkles_appendGfx after gSPDisplayList(D_09000F20_338EE0): set the combine with SHADE in place of PRIMITIVE
   (`gDPSetCombineLERP(gMainGfxPos++, 1, SHADE, TEXEL0, SHADE, SHADE, 0, TEXEL0, 0, 1, SHADE, TEXEL0, SHADE, SHADE, 0, TEXEL0, 0)`).
3. Per part: no gDPSetPrimColor. Copy the 4 vertices of sparkle_vtx[unk_2C & 7] into per-frame vertex memory with cn = the part
   colour (D_E0022CF0[colorIdx..+2], alpha 255), then gSPMatrix push MUL, gSPVertex(copy, 4, 0), gSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
   gSPPopMatrix. The memory: decide by reading how another recorded producer keeps per-frame vertices alive until the replay (grep
   `gDisplayContext->` vertex arrays and DIRECT D7 nd_rec_list in 3ds/src/nd_rec.c); name the choice in the commit.
4. Same colour order and the same part order as the N64 code (no sort).

## Gate (CLAUDE.md 2)
- build: memcheck OK (static growth written); `bench.py run nd_host --row SPARKLE --why "sparkles"`: PASS.
- Byte compare: `bench.py base starfx_hos01_pic`, then run it with the row lane, then `bench.py cmp starfx_hos01_pic --row SPARKLE`.
  Expected equal or a difference only in sparkle pixels: write the diff counts (the vertex colour may round differently from the
  TEV constant; the user accepts or rejects that on the console). spot_mac01_det byte-equal (no sparkles: unchanged).
- Draw count: the starfx_hos01 prof line before and after (the bench records it): write draws per frame.

## Stop
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the completing commit. Hand back: hash, gates, memcheck line, draws per frame before and after.
