# Row S5TIMG: draw_ci_image_with_clipping passes through the NdTRect record (S5 call sites, draw_img_util.c)

(Model: Haiku. Rewritten by ORACLE5 after the row spec fault of try 1; budget reset once. Step 0 is done (c4918c833, pad bit 0).
Worktree build3ds/wt/S5TIMG, merged with main 5c3865487.)

## Code read (src/draw_img_util.c on main 5c3865487)
- draw_ci_image_with_clipping (:8-41) states every value of the draw: cycle 1, filter POINT, persp NONE; opacity 255: TEX_EDGE
  and G_CC_DECALRGBA, no prim; opacity 1-254: XLU_SURF, PM_CC_02, prim (0,0,0,opacity); CI: G_TT_RGBA16 and LoadTLUT of
  `palette` (pal16 for 4b, pal256 else); not CI: G_TT_NONE. Then it calls draw_image_with_clipping. 16 callers (msg_draw.c 10,
  msg.c 1, Credits 3, kmr_20 documents 2, mgm_02 npc 1).
- draw_image_with_clipping (:50-171) loads and draws strips of 64 x 32 texels. Its own state is none. Direct caller besides
  draw_ci: msg.c:2272 (the choice pointer shadow; its state is set in msg.c). That caller keeps the old ops in this row.
- draw_tiled_image (:175) and draw_adjustable_tiled_image (:318) have no callers: not edited here (row_S9DEL4.md deletes them).

## Design
A file-static template pointer: draw_ci_image_with_clipping fills a template NdTRect with its state (pso words, prim or pad
bit 0, palette) and sets `dimg_tpl` around its call of draw_image_with_clipping. Inside the strip loop, when dimg_tpl is set and
bitDepth is 4b, 8b or 16b, one record replaces the load and the rectangle of the pass. dimg_tpl NULL (msg.c:2272) keeps the old
ops. Every state op of draw_ci_image_with_clipping stays: the S5TRECT fallback replays only the load and the rectangle.

## Touch only
src/draw_img_util.c, and 3ds/tools/azt_scenarios.json plus 3ds/tools/bench_defs.json (new entry msgbox_choice_det).

## Exact edits
1. Replace the empty `#ifdef TARGET_3DS` / `#endif` pair at :2-3 with
   ```c
   #ifdef TARGET_3DS
   #include <string.h>
   #include "nd_api.h"
   #define DIMG_CW_(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) \
       { ND_W0_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1), ND_W1_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) }
   #define DIMG_CW(a, b) DIMG_CW_(a, b)
   static const NdTRect* dimg_tpl; /* S5TIMG: the state of draw_ci_image_with_clipping for the strips of its draw (NULL: old ops) */
   #endif
   ```
2. draw_ci_image_with_clipping: replace `ret = draw_image_with_clipping(...);` with
   ```c
   #ifdef TARGET_3DS
       {
           static const u32 cw_decal[2] = DIMG_CW(G_CC_DECALRGBA, G_CC_DECALRGBA), cw_02[2] = DIMG_CW(PM_CC_02, PM_CC_02);
           NdTRect t;
           memset(&t, 0, sizeof t);
           if (opacity == 255) { t.cw0 = cw_decal[0]; t.cw1 = cw_decal[1]; t.oml = G_RM_TEX_EDGE | G_RM_TEX_EDGE2; t.pad = 1; }
           else { t.cw0 = cw_02[0]; t.cw1 = cw_02[1]; t.oml = G_RM_XLU_SURF | G_RM_XLU_SURF2; t.prim = opacity; }
           t.omh = (fmt == G_IM_FMT_CI ? G_TT_RGBA16 : G_TT_NONE) | G_TF_POINT;
           if (fmt == G_IM_FMT_CI) { t.pal = palette; t.pal_n = bitDepth == G_IM_SIZ_4b ? 16u : 256u; }
           dimg_tpl = &t;
           ret = draw_image_with_clipping(raster, width, height, fmt, bitDepth, posX, posY, clipULx, clipULy, clipLRx, clipRLy);
           dimg_tpl = NULL;
       }
   #else
       ret = draw_image_with_clipping(raster, width, height, fmt, bitDepth, posX, posY, clipULx, clipULy, clipLRx, clipRLy);
   #endif
   ```
3. draw_image_with_clipping: wrap the 3-arm load `if` and the gSPTextureRectangle (:142-156) as
   ```c
   #ifdef TARGET_3DS
               if (dimg_tpl && (bitDepth == G_IM_SIZ_4b || bitDepth == G_IM_SIZ_8b || bitDepth == G_IM_SIZ_16b)) {
                   NdTRect r = *dimg_tpl;
                   r.img = raster; r.fmt = fmt; r.siz = bitDepth; r.width = width; r.height = height;
                   r.uls = texRect.ulx; r.ult = texRect.uly; r.lrs = texRect.lrx; r.lrt = texRect.lry;
                   r.cms = G_TX_WRAP; r.cmt = G_TX_WRAP; r.masks = 6; r.maskt = 5;
                   r.w0 = ND_W0_TR(G_TEXRECT, drawRect.lrx * 4, drawRect.lry * 4);
                   r.w1 = ND_W1_TR(0, drawRect.ulx * 4, drawRect.uly * 4);
                   r.hs = ND_TR_H1(texOffsetX * 32, texOffsetY * 32);
                   r.hd = ND_TR_H2(1024, 1024);
                   nd_rec_napi(nd_trect_run, &r, sizeof r / 8u);
               } else
   #endif
               {
                   <the original 3-arm load if, unchanged>
                   <the original gSPTextureRectangle, unchanged>
               }
   ```
4. json: new scenario msgbox_choice_det after msgbox_choice_nowide: a copy of msgbox_choice_nowide with debug
   `preset=yard nowide uishow=choice shots shotwin=300:1 loglevel=debug det` (drawcensus dropped), expected_events only the
   warp and `debug uishow: choice opened f200` entries (both builds print them). Bench msgbox_choice_det: a copy of
   msgbox_talk_det with scenario msgbox_choice_det, until `shot \\d+ frame 300\\b`. Same commit as steps 1-3.

## Gate (expected values)
- Build, memcheck OK; `bench.py run nd_host --row S5TIMG --why "S5TIMG nd_host"`: PASS.
- `bench.py base msgbox_choice_det` (the choice pointer: msg.c:2273 draws through draw_ci_image_with_clipping) and
  `bench.py base msgbox_talk_det`; one list `bench.py run msgbox_choice_det,msgbox_talk_det --row S5TIMG --why "S5TIMG: draw_ci
  strips via NdTRect"` (2 starts). cmp: msgbox_choice_det 1/1, msgbox_talk_det 1/1 byte-equal.
- 0 `trect fallback` lines in both logs. Budget 8 (reset once).

## Stop
A byte difference: stop, commit, hand back the cmp lines (2 tries per named cause). Delete this row file in the completing commit.
