# Row BATWIDE: battle menu boxes missing and the wheel light off the item in widescreen (user, 2026-11-09)

> STALE (oracle 2026-11-11): picture steps in this row are void (CLAUDE.md 2: no visual checks; gates are nd_host and the byte compare). The oracle rewrites the gates before launch.

## User facts
User: in every battle with Widescreen on, in every build, on Azahar and the console: the menu boxes (the boxes and backgrounds behind
the wheel and sub-menus) are missing, only the jump/item/strategies icons and labels draw; the menu does not reach the left screen
edge; the wheel light is not on the selected item, for any item. An earlier agent's Azahar shots did not show it: do not use "not
reproduced" as an answer; the user's report is the fault. Ask through the orchestrator if a setup detail is needed.

## Cause candidates (shared oracle, code read)
1. Backing quads: btl_draw_prim_quad(l-12, t-12, 24, 24) (btl_states_menus.c:997) and the name box (:1009) take u16 arguments
   (190B20.c:3039). In wide BasePosX = pm_wide_left(54) = 14 (:609), WHEEL_RADIUS 101: every option with 14 + 101 cos t < 12 gets a
   negative left that wraps to about 65000, so its box draws off screen. Check: log l and the u16 value for each option once.
2. Light: spotlights and HighlightSpot (:921-954) use the icon x, but draw through the CAM_HUD ortho; wide_edge / wide_on_edge
   (pm_gpu.c:2608-2625, TRI-2) moves ortho vertices at game x <= 0 or >= 320 to the screen edge, so the spot quads move or stretch and
   the icons do not. Check: log the spot vertex x before and after wide_on_edge for the selected item.
3. Beam: fixed pm_wide_left(79)+5 = 44, y 173, scale 2.1, -43 deg (:968-971), tuned by eye, not from the selected item.

## Start state
`sh 3ds/tools/wt_new.sh BATWIDE`, branch BATWIDE. PM_ROW=BATWIDE, PM_LANE=BATWIDE. One coder agent.
Scenarios: battle_wheel_wide_left, battle_wheel5_wide (exist; battle_wheel_nowide_det for 4:3). Options must have Widescreen on.

## Fix
- 190B20.c: btl_draw_prim_quad / btl_appendGfx_prim_quad take signed coordinates (TARGET_3DS); a negative left stays negative; the
  fill path maps x0 <= 0 to screen 0.
- pm_gpu.c: CAM_HUD complex elements (already in wide coordinates) are exempt from the wide_on_edge vertex move.
- btl_states_menus.c: the beam pivot, length and angle come from the selected item position (BasePosX + R cos, BasePosY - R sin).

## Done criteria
nd_host PASS, memcheck OK. Wide battle menu shots at the left-most item, a middle item and the right-most item: boxes behind every
option, the menu reaching screen x 0, the light on the selected icon. One 4:3 shot (battle_wheel_nowide_det) unchanged apart from
the boxes. Delete this row file in the completing commit. Budget: 6 Azahar starts.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops. One check per candidate; a false check moves to the next.
