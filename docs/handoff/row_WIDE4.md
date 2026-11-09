# Row WIDE4: battle command wheel and menus clipped and misplaced in wide mode (WIDE3 follow-up)

## User facts
User (2026-11-09, wide on): (1) "the battle hud is not rendering properly. it was moved to the corner but its clipped. The only visible
part is in the native resolution play field. the rest that is outside of that is not rendered." (2) "The right side selection part of
the battle ui should also be brought up and to the right a bit, its too close to Mario." (3) "When I scroll the ui to the bottom, the
stuff that is on the left side of the menu gets visually weird. The circle texture clips the same way as the thing in the bottom left."
(4) "when there are 5 or more items and you go all the way to the right, the leftmost element ends up in a weird spot."

## Start state
Worktree build3ds/wt/WIDE4, branch WIDE4 from main (hash in the first commit). PM_ROW=WIDE4, PM_LANE=WIDE4.

## Code facts
- WIDE3 (git show 4a918140e, merge 770cc1c0f): src/battle/btl_states_menus.c BattleMenu_BasePosX = pm_wide_left(54) (:605), reel/
  projector/swap icons pm_wide_left (:654-711, :866, :952-967), MovesMenuPosX/moveX (:1234, :1394), StratsMenuPosX (:2130-2132).
- Scissors: btl_states_menus.c:1717 and :2367 gDPSetScissor with 4:3 coordinates; the wheel/circle textures and HUD elements may also be
  clipped by a scissor or a viewport set in 320 space elsewhere (hud_element draw, the wheel's draw_box/window, the native rect path
  nd_rect.c clip to the 4:3 field, pm_wide x mapping for HUD elements: find which clip is 320 wide). Elements at x < 40 (left of the 4:3
  field) are not drawn = a clip at the 4:3 field edge.
- The wheel option layout (BattleMenu_* theta, WheelOptionCount, the scroll of 5+ options) computes positions around the base and may
  wrap or clamp in 320 space (item 4).

## Plan
1. Trace by code each clip that applies to the battle menu draws (scissor, viewport, the rect clip, the HUD element clip) and make it
   cover the wide screen when wide is on (pm_wide: 0..400 in game x after the -40 shift, i.e. -40..360 in 4:3 game x). Same for the
   circle texture.
2. Item 2: move the right-side part (the option title / selected-move panel next to the wheel; find which element the user means by
   its draw near Mario) up and right with pm_wide_right or an anchor, keeping its size. State the chosen offset in the commit.
3. Item 3: the scrolled list's left-side elements (reel icons) follow the same anchor in every scroll state.
4. Item 4: with 5+ options at the right end, the leftmost element's x uses the same wide anchor (find the wrap/clamp in 320 space).
5. Each fix only under pm_wide; 4:3 unchanged.

## Done criteria
nd_host PASS; memcheck OK; pictures with wide on (battle with 5+ options, e.g. a save with items/moves; padscript): the whole wheel and
circle drawn in the left corner with no clip, the right panel moved, the list scrolled to the bottom and to the right end with every
element in place; 4:3 battle shots byte-equal to base with nowide (battle_start_f182_cold with nowide, and a 4:3 wheel shot).

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no toggles, no stubs. Never commit 3ds/src/map_restore_table.c. Padscripts in the azslots sdmc.

## At the end
Delete this row file in the completing commit. SendMessage main "WIDE4 ready <hash>".
