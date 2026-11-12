# Row FSELFIX: ROTSCALE windows draw their contents image as one texture and one vertex batch, not height/6 strips (D1)

(Model: Haiku, long row. Every value is below; no design choice. Why not Sonnet: one loop in one function, with the vertex values
computed from the loop it replaces.)

## User fault and census (FSELCEN, merged; calls per frame)
The file select flip on a file pick is very laggy, and the pause flip uses the same path.
- fsel_fncov_pick f208 (the flip) against fsel_fncov_idle f400: flush_draw 663 against 425; gv_tri 216 against 0;
  ensure_tile_slow 181 against 162.
- pm_gpu_off_begin 86: that is not 86 passes. off_screen_check (nd_target.c:89-93) calls pm_gpu_off_begin for every draw into an
  off-screen image, and the call returns early while the pass is active (pm_gpu.c:2027). The real pass count is near off_end 30.
- Pause flip: cc_v 6144 and lcg_f 4800 come from decide_miss (nd_comb.c:381-421): 24 random samples per new combine key, about
  8 new keys. That is a first-use cost per key (a 128-entry table), not a cost per flip frame. S6T removes it. No action here.
Cause by code read (draw_box.c:801-823): per ROTSCALE window per frame, height/6 strips, each a LoadTextureTile of 7 rows of the
off-screen contents image, a SetTileSize, gSPVertex of vtx_drawbox1 (4 vertices), 2 triangles, a PipeSync and a matrix multiply.
That is 216 triangles in about 108 strips for 11 windows, each strip its own gv batch and flush. The 7-row load is an N64 TMEM limit
(320 x 6 x 2 bytes). On the 3DS the contents image is one off-screen slot texture.

## Start
`sh 3ds/tools/wt_new.sh FSELFIX`; PM_ROW=FSELFIX PM_LANE=FSELFIX. No running row edits draw_box.c (S2W waits for NAPI2B and edits
the non-ROTSCALE path, other lines).

## Touch only
src/draw_box.c: the strip loop :801-823 inside `if(mdl_address != 0)` only, plus one static vertex pool above draw_box.

## Exact edits (under `#ifdef TARGET_3DS`; `#else` keeps the old loop)
1. Above draw_box: `#define BOX_STRIP_VTX 1024` and `static Vtx sBoxStripVtx[2][BOX_STRIP_VTX]; static s32 sBoxStripN[2]; static
   u16 sBoxStripFrame[2];`.
   - Index the pool by gCurrentDisplayContextIndex. A display context is built again only after the walk of the frame that used
     it (nd_wq.c:7-8, depth 1), so the walker has read these vertices before core 1 writes them again.
   - Reset: `if (sBoxStripFrame[c] != (u16)gGameStatusPtr->frameCounter) { sBoxStripFrame[c] = (u16)gGameStatusPtr->frameCounter;
     sBoxStripN[c] = 0; }`, where c = gCurrentDisplayContextIndex.
2. Replace the strip loop (:804-823; the guTranslateF/gSPMatrix lines :799-803 before it stay) with:
   a. `s32 ns = height / 6;` If `ns <= 0` or `sBoxStripN[c] + ns * 4 > BOX_STRIP_VTX`: run the old loop unchanged (keep it in the
      TARGET_3DS branch as the fallback).
   b. One load of the whole image:
      `gDPLoadTextureTile(gMainGfxPos++, OS_K0_TO_PHYSICAL(mdl_address), G_IM_FMT_RGBA, G_IM_SIZ_16b, width, 0, 0, 0, width - 1,
      height - 1, 0, G_TX_CLAMP, G_TX_CLAMP, 9, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);`
      maskt is NOMASK, not 3: a mask of 3 would make the walker's window 8 rows (tex_win0 cuts h to the mask period).
   c. One tile size: `gDPSetTileSize(gMainGfxPos++, G_TX_RENDERTILE, (160 - width / 2) * 4, 0, ((160 - width / 2) + width - 1) * 4,
      (height - 1) * 4);`.
   d. Vertices, for idx 0..ns-1, 4 per strip in the order of vtx_drawbox1 (assets/us/vtx/drawbox1.vtx.inc.c):
      - ob x: -160, 160, -160, 160;
      - ob y: `6 * idx + {0, 0, 6, 6}`. The strip matrices of the old loop (translate 0, 6 per strip after the first, on top of
        the -height/2 translate that stays at :801-803) give exactly these offsets in integers.
      - tc s: 0, 10256, 0, 10256;
      - tc t: `idx * 6 * 32 + {0, 0, 208, 208}` (the strip's rows start at image row idx * 6);
      - colours 0 and flag 0 as vtx_drawbox1.
   e. Draw in chunks of at most 32 vertices (8 strips):
      `gSPVertex(gMainGfxPos++, &v[k * 4], n * 4, 0);` then per strip j of the chunk
      `gSP2Triangles(gMainGfxPos++, 4*j+0, 4*j+3, 4*j+1, 0, 4*j+0, 4*j+2, 4*j+3, 0);` (the old index order 0,3,1 / 0,2,3).
   f. One `gDPPipeSync(gMainGfxPos++)` after the last chunk. No gBoxMatrix multiply: the gSPPopMatrix at :830 restores the stack
      as before.
   g. `sBoxStripN[c] += ns * 4;`.
3. Pre-flight check: `grep -n "gCurrentDisplayContextIndex" src/main_loop.c` shows that it flips once per frame (:341).

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- `bench.py base pause_spin_grab_det_w300`; one list
  `bench.py run pause_spin_grab_det_w300,fsel_fncov_pick --row FSELFIX --why "D1: ROTSCALE contents as one batch"`. The census
  scene needs the PM_FNCOV lane: run it as a second call with `--lane FSELFIX_fncov` after
  `PM_FNCOV=1 PM_ROW=FSELFIX PM_LANE=FSELFIX sh 3ds/build.sh`. 2 starts in all.
- `bench.py cmp pause_spin_grab_det_w300`: every shot byte-equal. With a diff, give its size and box. The rounding rule passes it
  only if every channel differs by at most 1 step, inside the flipping window area; the cause would then be bilinear samples at
  the strip tops that now read the row above (the strip load clamped them). Larger diffs: stop.
- fsel_fncov_pick: gv_tri, flush_draw and ensure_tile_slow drop against FSELCEN (216, 663, 181). Expected gv_tri still 216 (same
  triangles), and flush_draw down by about 100 (one batch per window). Hand back the 10 named counts.
- Budget 6 Azahar starts. Console: one file select push (CONSOLE C4) after the merge. The GPU part (the 512 x 256 slot clear per
  window, pm_gpu.c:2089) is measured there, and its fix is a later row.

## Stop
A byte difference above the rounding rule, or a fallback taken in the pause scene: stop, commit, hand back. Delete this row file in
the completing commit.
