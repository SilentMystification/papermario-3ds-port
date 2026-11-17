# ws_ENTCAPR2 (Haiku): curtains and Merlee card sites get their captured entries (C1 build -3, C2 gfxpos -3)
Designer: the oracle (SendMessage). Worktree build3ds/wt/ENTCAPR2, branch ENTCAPR2 from main, PM_ROW=ENTCAPR2 PM_LANE=ENTCAPR2.
Main at release: 52f9b15c42. Start: now. Do not use the old branch ENTCAPR. Edit only the 4 files of docs/handoff/ws_ENTCAPR2.patch:
src/curtains.c, src/world/area_dro/dro_02/npc_merlee.c, new 3ds/tools/ntab_ent.d/ENTCAPR2.txt, 3ds/tools/entcap.py (ENTCAPR fix).
## Objective
- FACT (ENTCAPR capture, build3ds/wt/ENTCAPR/build3ds/out/entcapr_cap.txt): curtains.c:151 logged 3 word sets. They differ only in
  cw0, cw1, oml and comb_alpha_shade. TheaterGfx calls TheaterFloorGfx first (gfxin/src/curtains.c:107), and it sets the render mode
  and the combine (:65-67) before its first draw. nd_comb.c:526 sets comb_alpha_shade at each combine. So the page uses set 1.
- FACT (same file): npc_merlee.c:259 logged 2 sets. Set cw1 0xffff73b9, leg_prev 0 follows card_1_gfx (unk_00 1, card_1 ends with
  its own combine). Set cw1 0xfffcf279, leg_prev 2 equals the :248 entry (unk_00 5, only card_setup_gfx before). The patch splits :259.
- filemenu_common.c:996 :1018 are not in this row (D_8024B708 uses vertex slots 0-1 of D_8024B6F0 under another matrix: a design).
- Counters (FACT, oracle listcen on main + patch): build 341 -> 338, gfxpos 44 -> 41, recdl 11.
## Edit shapes (the patch holds every edit; FACT `git apply --check` OK on 52f9b15c42)
- curtains.c:151. Before: `        gSPDisplayList(gMainGfxPos++, NDN(TheaterGfx));`
  After: `        nd_draw_list(NDN(TheaterGfx), pe_curtains_151);` and one `ND_PSO_DESC(pe_curtains_151, ...)` line after `#include "nd_gtab.h"`.
- npc_merlee.c:259. Before: `            gSPDisplayList(gMainGfxPos++, NDN(N(card_2_gfx)));`
  After: `if (card->unk_00 == 1) { nd_draw_list(NDN(N(card_2_gfx)), pe_npc_merlee_259a); } else { ... pe_npc_merlee_259b); }`
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_ENTCAPR2.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. Commit the 4 files: `ENTCAPR2 1: captured entries of curtains.c:151 and npc_merlee.c:248 :259 (split by card state)`.
3. `sh 3ds/tools/rowloop.sh --row ENTCAPR2 --bench curt_title_drec,gmerlee_drec --why "ENTCAPR2 gate" > build3ds/out/entcapr2_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, both cmp lines byte-equal.
- build3ds/out/ntab_entries.txt has 4 lines for these sites. FACT (oracle host run): `TheaterGfx pe_curtains_151`,
  `dro_02_card_1_gfx pe_npc_merlee_248`, `dro_02_card_2_gfx pe_npc_merlee_259a`, `dro_02_card_2_gfx pe_npc_merlee_259b`.
- `python 3ds/tools/listcen.py | grep "^listcen total"`: build 338, gfxpos 41, recdl 11.
- GUESS: gmerlee_drec (frames 660-835) draws both card states. FACT: the ENTCAPR capture with its words logged both before f835.
## What you may edit to fix a fail
A DIFF in curt_title_drec: change pe_curtains_151 to the set-2 words (`0x2dffffff, 0xfffdf6fb, 0x1e182c00, 0x00504340`, last 12
bytes `0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0`), 1 try. A DIFF in gmerlee_drec: swap 259a and 259b in the if, 1 try. Then stop and
send build3ds/out/loop_ENTCAPR2_cmp_<bench>.txt to the oracle. Budget: 3 builds, 6 Azahar starts.
## Stop and hand-back
Last commit `ENTCAPR2 2: gate. curt_title_drec, gmerlee_drec byte-equal; build 338 gfxpos 41` deletes ws_ENTCAPR2.md, ws_ENTCAPR2.patch,
ws_ENTCAPR.md and ws_ENTCAPR.patch. One SubagentHandback: commit hashes, LOOP lines, listcen totals, the tries used.
