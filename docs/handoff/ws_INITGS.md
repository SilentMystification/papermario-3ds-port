# ws_INITGS (Haiku): 3 root state lists move to device keys, 5 sites to nd_draw_list ND_SELF (C1 -5, C2 -5, C5 -28, C6 -3)
Designer: the oracle (SendMessage). Worktree build3ds/wt/INITGS, branch INITGS from main, PM_ROW=INITGS PM_LANE=INITGS.
Main at release: 52f9b15c42. Start: now. Edit only the 6 files of docs/handoff/ws_INITGS.patch: src/hud_element.c,
src/state_title_screen.c, new 3ds/tools/gfxin/src/hud_element.c, new 3ds/tools/gfxin/src/state_title_screen.c,
new 3ds/tools/gconv_src.d/INITGS.txt, new 3ds/tools/gset.d/INITGS.txt.
## Objective
- Lists: HudElemBackInitGfx, HudElemFrontInitGfx (hud_element.c:60, :67), TitleSetupGfx (state_title_screen.c:96). Each is state only
  (scissor, other modes, no draw). FACT (oracle host run, main + patch): the converter marks all 3 ok in gset_ok.txt,
  `gconv lists 1939 ... bad 0 moved 1934`, ND HOST TESTS PASS.
- The patch is the output of `python 3ds/tools/gset.py 3ds/tools/gset.d/INITGS.txt build3ds/out/gset_ok.txt` plus 2 hand edits:
  state_title_screen.c:711 `&TitleSetupGfx` became `TitleSetupGfx` (same address; gset.py holds a list named with `&`), and the 5 sites.
- FACT (code): ND_SELF is the null entry (nd_gtab.h:88). A state-only key with ND_SELF is the form of npc_merlee.c:235 (card_setup_gfx).
- Counters (FACT, oracle listcen on main + patch): build 341 -> 336, gfxpos 44 -> 39, data 482 -> 454, data_arrays 15 -> 12,
  gfxtype 428 -> 425, recdl 11.
## Edit shape (hud_element.c:1448). Before: `    gSPDisplayList(gMainGfxPos++, VIRTUAL_TO_PHYSICAL(HudElemBackInitGfx));`
After: `    nd_draw_list(NDN(HudElemBackInitGfx), ND_SELF);` (VIRTUAL_TO_PHYSICAL is a u32 cast, include/macros.h:68).
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_INITGS.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. Commit the 6 files: `INITGS 1: HudElemBackInitGfx, HudElemFrontInitGfx, TitleSetupGfx to device keys (gset.d INITGS), 5 sites ND_SELF`.
3. `sh 3ds/tools/rowloop.sh --row INITGS --bench curt_title_drec,spot_mac01_drec --why "INITGS gate" > build3ds/out/initgs_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, both cmp lines byte-equal.
- `grep -cE "^(HudElemBackInitGfx|HudElemFrontInitGfx|TitleSetupGfx)$" build3ds/out/gset_ok.txt` prints 3.
- `python 3ds/tools/listcen.py | grep "^listcen total"`: build 336, gfxpos 39, data 454, data_arrays 12, gfxtype 425.
- GUESS: curt_title_drec (title frames 360-540) runs title_screen_draw_logo and _copyright. spot_mac01_drec runs
  render_hud_elements_backUI and _frontUI (hud_element.c:1448, :1568). The byte compare decides.
## What you may edit to fix a fail
Only the 6 files. A build error in hud_element.c or state_title_screen.c: add `#include "nd_gtab.h"` if it is missing (the patch has it),
1 try. A DIFF: stop. Send build3ds/out/loop_INITGS_cmp_<bench>.txt to the oracle. Budget: 2 builds, 4 Azahar starts.
## Stop and hand-back
Last commit `INITGS 2: gate. curt_title_drec, spot_mac01_drec byte-equal; build 336 gfxpos 39 data 454` deletes ws_INITGS.md and
ws_INITGS.patch. One SubagentHandback: commit hashes, LOOP lines, listcen totals.
