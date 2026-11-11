# Row BATUI: battle menu wide layout (the description boxes use the full widescreen width)

(Model: Haiku. Base: main 436abd053. Runs together with KEYMISS and KEYFIX1: no shared file. The agent reads only this file and CLAUDE.md.)

## User facts
- 2026-11-09 (BATWIDE): with Widescreen on, the menu boxes are missing and the menu does not reach the left screen edge.
- 2026-11-11 (decision): BATUI uses the wide layout: the battle UI uses the full widescreen width.

## Design (oracle code read, main 436abd053; include/pm_wide.h:5-6: pm_wide_left(x) = x - 40, pm_wide_right(x) = x + 40 in wide)
- The wheel is already in the wide layout (WIDE3/WIDE4): base pm_wide_left(54) (btl_states_menus.c:611), radius 101 (:597), the reels
  at pm_wide_left(40) (:660, :670, a 64 px quad centred on its position: 8 px from the screen edge, as 4:3 is from the picture edge),
  the moves and strategies lists at pm_wide_left(POS_X) (:1258, :1418) and pm_wide_left(20 or 32) (:2154-2156). No change there.
- The two description boxes are the battle UI parts that span the 4:3 picture (x 20 to 300, POS_X/WIDTH_3 :1215-1238, X_VAR7/W_VAR3
  :2131-2140). In wide the moves box is only moved left (:1420: x pm_wide_left(POS_X), width WIDTH_3: screen 20 to 300) and the
  strategies box is not moved at all (:2195-2197: x X_VAR7: screen 60 to 340). Wide layout: both span pm_wide_left(x) to
  pm_wide_right(x + w), screen 20 to 380. Their text starts at the box x + 8 (:1974-1975, :2473): it follows the box.
- The 4:3 path (pm_wide 0) is unchanged by construction (the added width is 0, pm_wide_left(x) is x).

## Start
`sh 3ds/tools/wt_new.sh BATUI`. PM_ROW=BATUI PM_LANE=BATUI on every build call.

## Touch only
src/battle/btl_states_menus.c: the WIN_BTL_DESC_BOX lines in the moves menu init (:1420) and the strategies menu init (:2195-2197).

## Exact edits (src/battle/btl_states_menus.c; pm_wide.h is included at :3)
1. :1420 `set_window_properties(WIN_BTL_DESC_BOX, moveX, 186, WIDTH_3, 32, ...` -> width `WIDTH_3 + (pm_wide ? 80 : 0)`. Comment:
   `/* BATUI: wide layout, the box spans pm_wide_left(POS_X) to pm_wide_right(POS_X + WIDTH_3) */`.
2. :2195 `x = X_VAR7;` -> `x = pm_wide_left(X_VAR7); /* BATUI: wide layout, as the moves box */`.
3. :2197 `set_window_properties(WIN_BTL_DESC_BOX, x, y, W_VAR3, 32, ...` -> width `W_VAR3 + (pm_wide ? 80 : 0)`.
No other line. No new variable.

## Gate (CLAUDE.md 2)
- `PM_ROW=BATUI PM_LANE=BATUI sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: build OK, memcheck OK.
- `git diff main --stat`: only src/battle/btl_states_menus.c, 3 lines changed.
- No Azahar run (budget 0): the change is game layout code, not shared draw code; no det scene opens these menus. The look is the
  user's console check (Wide on: battle, Mario's Jump list and the Strategies list: the description box from 20 px to 380 px).

## Stop
A failed gate or an edit that does not fit the code: commit, hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the completing commit. Hand back: the hash, the memcheck line, and the console check text above for the user.
