# Row BTLUI2: battle menu panel and reel (IA8 transformed HUD quads): reach the menu, record what is drawn, test the thread race

(Model: Haiku. Bug: user fact: in the main battle menu the option icons draw, the background panel and the lower-left reel do not.
Code only: no shot reading.)

## Code read (oracle, main after BTLUICEN; BTLUICEN census: no counted drop at f300/f400 in head and 9eeb74b1a, menu not proven open)
- btl_main_menu_draw (src/battle/btl_states_menus.c:852-990), state CHOOSING: hud_element_draw_complex_hud_first, then
  render_complex_hud_element for the spotlights (HES_ProjectorSpot), the beam HID_ProjectorBeam (panel) and the reels
  HID_ProjectorReelA/B (lower left): all hs_UseIA8 (src/battle_ui_gfx.c:522-546), so case 2 of render_complex_hud_element
  (src/hud_element.c, the quad in transform->quadBuffers[FrameQuadIndex], gDPLoadTextureBlock IA8, PM_CC_47, gSPVertex, 2 gSP1Triangle,
  lighting on from draw_complex_hud_first). The option icons are CI4 rects (hud_element_draw_clipped: NAPI2A records) and draw.
- No merge since 9eeb74b1a edits render_complex_hud_element, draw_complex_hud_first or btl_main_menu_draw (git diff: hud_element.c
  changes are inside hud_element_draw_rect only). The census counts no rect, triangle or batch drop. Remaining paths, none counted yet:
  a. the element is not drawn on core 1: flags DISABLED or INVISIBLE (the beam is created DISABLED, :686-690, and enabled by the menu),
     opacity 0 (opacity = BattleMenu_ReelAlpha * BattleMenuAlpha / 255, :887), a state that skips the block;
  b. drawn but invisible: prim alpha 0, transform scale 0, position off screen (pm_wide_left(40) - BattleMenu_ReelHidePercent);
  c. the thread race: core 1 writes transform->quadBuffers[FrameQuadIndex] and the HUD state of step N+1 while core 0 walks step N
     (FrameQuadIndex has 3 slots and advances in render_hud_elements_backUI, hud_element.c:1306); the BTLUICEN scene ran with det
     (serial: no race), the user plays threaded. The c2haz census (CORE2-2) names walker reads of memory the logic rewrote.

## Start
`sh 3ds/tools/wt_new.sh BTLUI2`. PM_ROW=BTLUI2 PM_LANE=BTLUI2.

## Exact edits
1. src/hud_element.c render_complex_hud_element: a debug switch `cplxhud` (static int read once with pm_debug_has, as mdl0_sw in
   nd_model.c:216): at the switch on `mode`, `pm_log_d("cplx f%u mode %d flags %08x op %u pos %d,%d scale %d tsc %d,%d img %p q %d",
   pm_frame_count(), mode, hudElement->flags, hudElement->opacity, hudElement->renderPosX, hudElement->renderPosY,
   (int)(hudElement->uniformScale * 100), (int)(transform->scale.x * 100), (int)(transform->scale.y * 100), hudElement->imageAddr,
   FrameQuadIndex)` for the first 120 calls after frame 150. In btl_main_menu_draw under the same switch one line per 30 frames:
   `pm_log_d("btlmenu f%u state %d reelalpha %d menualpha %d hide %d", ...)` (BattleMenuState, BattleMenu_ReelAlpha, BattleMenuAlpha,
   BattleMenu_ReelHidePercent).
2. Scenarios and benches btlui_menu_det (debug `warp=kmr_03:0 story=ch0_toad_town battle battlef=100 cplxhud census loglevel=debug det`)
   and btlui_menu_thr (the same without det and census, plus `c2haz`), until `btlmenu f\d+ state` on a line after frame 300, cap 40.
   Commit with the code.

## Gate (measurement)
Build, memcheck OK. One list, 2 Azahar starts: `bench.py run btlui_menu_det,btlui_menu_thr --row BTLUI2 --why "battle menu panel and
reel: drawn or not, values, thread race"`. Hand back the btlmenu lines, the cplx lines of mode 2, the tridrop and rect lines (det run)
and the c2haz lines (thr run), as printed. Stop after it.

## At the end
Delete this row file in the completing commit.
