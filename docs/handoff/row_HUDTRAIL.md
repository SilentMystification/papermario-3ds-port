# Row HUDTRAIL: wide status bar draws a trail of star power dots to the left and repeated coin digits to the right

## User facts
User (2026-11-09, O3DS XL, build 92e013100, wide): with max star power and max coins (cheats, values in range) the star power meter
has yellow dots trailing to the left, and 999 coins shows 3 extra 9s at the end. The user thinks it is a draw bug (confirmed).

## Start state
Worktree build3ds/wt/HUDTRAIL, branch HUDTRAIL from main. PM_ROW=HUDTRAIL, PM_LANE=HUDTRAIL. One Haiku agent.

## Cause (oracle, about 90%)
- 3ds/src/pm_gpu_rect.c:218-222 (tex_raw_rect, F07-7 edge stretch, 48b52addba): in wide, any texrect with x0 == 0 stretches to
  screen x 0 and any with x1 == 319 to screen x 400, and the texture coordinate stretches too, so the texture repeats across the
  40 px gutter. Meant for full-screen bars, fades and overlays.
- WIDE3 status_bar_item_x (src/inventory.c:566-568) put two HUD pieces on the edges:
  - star power dot 4 of bar 0: x = status_bar_item_x(32) = 0, offset 4 (partners.c:149), 8x8 art offsetX -4 -> ulx 0; the wrapped
    dot repeats 5 times into the left gutter.
  - coin digits: status_bar_draw_number (inventory.c:470) at 304, 312, 320; tens digit rect 304..320 -> ex 319 -> stretched to 400;
    the tens digit repeats at game x 320, 336, 352 (any count >= 10 shows the trail of its tens digit).
- 4:3 is not affected. yard_wide_hud ran with 0 coins and 0 star power, so WIDE3 missed it.

## Fix steps
1. pm_gpu_rect.c:218: add `&& x1 - x0 + 1 >= 160` to the F07-7 condition. Comment HUDTRAIL: only a screen-covering rectangle reaches
   the screen edge; a HUD piece on game x 0 or 319 repeated its texture across the gutter.
2. 3ds/src/pm_warp.c next to spmax (about line 846): debug switch `coins=<n>` (gPlayerData.coins = (s16)atoi(vs)).
3. Scenarios + benches in the same commit: yard_wide_hud_max (copy of yard_wide_hud, azt_scenarios.json about :9859, plus
   `spmax=7 cheats=sp coins=999`, same until-line and shotwin, cap 45) and its 4:3 copy yard_hud_max (nowide); benches *_pic.
4. If the trail stays, or a wide fade/overlay now leaves a gutter strip: revert step 1 and skip the stretch only when the rect draws
   one whole small texture (pm_tex_size tw <= 64 and x1 - x0 + 1 <= tw).

## Gates
- nd_host PASS.
- yard_wide_hud_max_pic: 7 full bars, no dots left of the bar, coins exactly "999". yard_hud_max_pic (4:3): same.
- Byte-equal to a build-at of the base: msgbox_talk_nowide_pic.
- Wide edge consumers (pictures): wide_fade_mac (fade reaches both screen edges), dark_kgr01 (darkness covers the gutters),
  shop_wide_pic and battle_wide_pic unchanged.
- memcheck OK. Console (user): XL in wide with max star power and 999 coins.
