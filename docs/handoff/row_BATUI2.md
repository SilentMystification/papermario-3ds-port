# Row BATUI2: wide layout of the item popup menu and the level-up description box

(Model: Haiku. Base: main 5d26c84a2. Disjoint from MAPROMFS and BTLDBG. Reads only this file and CLAUDE.md.)

## User decision (2026-11-11)
In Wide mode the battle item popup menu (shared with the field menus) and the level-up description box use the full width, as BATUI
did for the moves and strategies description boxes.

## Design (oracle code read, main 5d26c84a2; include/pm_wide.h: pm_wide_left(x) = x - 40, pm_wide_right(x) = x + 40 in wide)
- The popup menu (src/8a860_len_3f30.c) is one code path for battle items and every field popup (use item, switch partner, shops, key
  items, chests, letters, diary, post office): this change moves them all in Wide mode (the user accepts the field effect).
  Its description box: x PopupDescX (20, JP 32; :370, :374), width WINDOW_KEY_WIDTH (280, JP 242; :21, :33), set at :796. Its list
  window: x posX = PopupWinX (:486; PopupWinX 20 at :372, or the field start PopupWorldStartX + 20 at :411), the title windows are its
  children (relative x). Text starts at the box x + 8 (:1954): it follows the box.
- Level-up description box (src/battle/level_up.c:1163 JP, :1167 others): x 32 or 20, width 242 or 280, literals in the call.
- Wide layout, as BATUI: a description box spans pm_wide_left(x) to pm_wide_right(x + w) (width + 80); the list window is anchored to
  the left screen edge (pm_wide_left), as the battle moves list. The 4:3 path is unchanged by construction.

## Start
`sh 3ds/tools/wt_new.sh BATUI2`. PM_ROW=BATUI2 PM_LANE=BATUI2 on every build call.

## Touch only
src/8a860_len_3f30.c: the include lines, :486, :796. src/battle/level_up.c: the include lines, :1163, :1167.

## Exact edits
1. src/8a860_len_3f30.c: add `#include "pm_wide.h"` after `#include "message_ids.h"`.
   :486 `posX = PopupWinX;` -> `posX = pm_wide_left(PopupWinX); /* BATUI2: wide layout, the list at the left screen edge */`.
   :796 `set_window_properties(WIN_POPUP_DESC, posX, posY, WINDOW_KEY_WIDTH, 32, ...` -> x `pm_wide_left(posX)`, width
   `WINDOW_KEY_WIDTH + (pm_wide ? 80 : 0)`; comment `/* BATUI2: the box spans the wide screen */`.
2. src/battle/level_up.c: add `#include "pm_wide.h"` after the last #include line.
   :1163 `set_window_properties(WIN_BTL_DESC_BOX, 32, 186, 242, 32, ...` -> `pm_wide_left(32), 186, 242 + (pm_wide ? 80 : 0), 32`.
   :1167 `set_window_properties(WIN_BTL_DESC_BOX, 20, 186, 280, 32, ...` -> `pm_wide_left(20), 186, 280 + (pm_wide ? 80 : 0), 32`;
   comment `/* BATUI2: wide layout */` on both.
No other line.

## Gate (CLAUDE.md 2)
- `PM_ROW=BATUI2 PM_LANE=BATUI2 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: build OK, memcheck OK.
- `git diff main --stat`: only the two files, 7 lines changed.
- No Azahar run (budget 0): game layout code, not shared draw code; no det scene opens these menus. The look is the user's console
  check (Wide on: a battle Items list, a field item menu, a level-up screen: the description box from 20 px to 380 px, the list at the left).

## Stop
A failed gate or an edit that does not fit the code: commit, hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the completing commit. Hand back: the hash, the memcheck line, and the console check text above for the user.
