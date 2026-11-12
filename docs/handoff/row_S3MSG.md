# Row S3MSG: draw_number's digits through the NdTRect record (NATIVEAPI S3, part 1 of msg.c)

(Model: Haiku. One function pair in src/msg.c plus a 1-line accessor in src/hud_element.c. The S5TIMG pattern.)

## Blocker and base
S5TRECT and S5TIMG merged (S5TIMG step 0: NdTRect `pad` bit 0 = keep the walker's prim). `sh 3ds/tools/wt_new.sh S3MSG`;
PM_ROW=S3MSG PM_LANE=S3MSG on every build and bench call.

## Code read (oracle 4, main 2fdcf9a8b; re-grep, lines may move)
- draw_number (src/msg.c:2298-2381): gSPDisplayList(gMsgDlistInitDrawNumber) (:2331; the list :167-179 sets cycle 1, filter
  POINT, persp off, TLUT RGBA16, texture on); the drop-shadow loop (:2354-2361: render mode XLU_SURF pair, PM_CC_07, prim 40 40
  40 72, draw_digit); then opacity == 255: render mode TEX_EDGE pair and G_CC_DECALRGBA, no prim; else XLU_SURF pair, PM_CC_01,
  prim 255 255 255 opacity (:2364-2371); gDPLoadTLUT_pal16(0, D_802F4560[palette]) (:2373); the main loop of draw_digit (:2374-2379).
- draw_digit (:2276-2296): gDPLoadTextureTile_4b(img, CI, texWidth, texHeight, 0, 0, texWidth - 1, texHeight - 1, 0, NOMIRROR |
  WRAP twice, NOMASK twice), then the rect unless pm_anchor_rect took it (an open anchor: hud_element.c:2553, sAnchorOpen).
- The shadow digits run before the LoadTLUT: their palette is the TLUT the previous draw left, unknown on core 1. They keep the ops.

## Touch only
src/msg.c: draw_digit, draw_number, one static above draw_digit. src/hud_element.c: one new function right after pm_anchor_rect
(:2553-2584). include/pm_interp_fi.h: its declaration after pm_anchor_rect's (:14). Other rows edit other functions there.

## Exact edits (under `#ifdef TARGET_3DS`; `#else` keeps the old lines)
1. hud_element.c after pm_anchor_rect: `int pm_anchor_on(void) { return sAnchorOpen; }`; pm_interp_fi.h: `extern int pm_anchor_on(void);`.
2. msg.c above draw_digit: `static const NdTRect* msg_digit_st; /* S3MSG: the state of the main digits, NULL for the shadow digits */`
   and `#include "nd_api.h"` in the includes (under TARGET_3DS).
3. draw_number: right before the main loop (after the LoadTLUT) fill `NdTRect st; memset(&st, 0, sizeof st);` with pal =
   D_802F4560[palette], pal_n 16, fmt G_IM_FMT_CI, siz G_IM_SIZ_4b, cms = cmt = G_TX_NOMIRROR | G_TX_WRAP, masks = maskt =
   G_TX_NOMASK, omh = G_TT_RGBA16 | G_TF_POINT; opacity == 255: cw0/cw1 of G_CC_DECALRGBA (twice), oml = G_RM_TEX_EDGE |
   G_RM_TEX_EDGE2, pad = 1; else cw0/cw1 of PM_CC_01 (twice), oml = G_RM_XLU_SURF | G_RM_XLU_SURF2, prim = 0xFFFFFF00u | (u8)opacity.
   The combine words as S5TIMG builds them (copy hud_element.c's HUD_CW macros under other names). Set `msg_digit_st = &st;`
   before the loop and `msg_digit_st = NULL;` after it.
4. draw_digit: at its top, `if (msg_digit_st && !pm_anchor_on()) { NdTRect r = *msg_digit_st; r.img = img; r.width = num->texWidth;
   r.height = num->texHeight; r.uls = 0; r.ult = 0; r.lrs = num->texWidth - 1; r.lrt = num->texHeight - 1; r.w0 = ND_W0_TR(G_TEXRECT,
   4 * (posX + num->texWidth), 4 * (posY + num->texHeight)); r.w1 = ND_W1_TR(G_TX_RENDERTILE, 4 * posX, 4 * posY); r.hs = ND_TR_H1(0, 0);
   r.hd = ND_TR_H2(1 << 10, 1 << 10); nd_rec_napi(nd_trect_run, &r, sizeof r / 8u); return; }`. The rest of draw_digit stays.

## Merge rule
Right before the gate runs: `git merge main`, rebuild, nd_host; then take the bases at the new `git merge-base HEAD main`.

## Gate (expected values)
- Build, memcheck OK; `bench.py run nd_host --row S3MSG --why "S3MSG digits"`: PASS.
- `PM_ROW=S3MSG PM_LANE=S3MSG python 3ds/tools/bench.py base pause_tab_det_pic` and the same for spot_mac01_det. One list:
  `bench.py run pause_tab_det_pic,spot_mac01_det --row S3MSG --why "S3MSG: draw_number digits as NdTRect"` (2 starts).
  `bench.py cmp` both: every shot byte-equal (pause_stats.c and the HUD call draw_number). No `trect fallback` line in either
  log after frame 200. Budget 5.

## Stop and end
A byte difference, a fallback line, an edit that does not fit, or a test-tool fault: commit, hand back the exact output. Delete
this row file in the completing commit. Later msg.c parts (not this row): the rewind arrow (a 2-texture quad: after S5CHUD and
S2W), the choice pointer (its images go through draw_img_util.c: S5TIMG), the render_messages prologue (S6S2).
