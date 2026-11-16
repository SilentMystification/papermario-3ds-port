# ws_EVAR (Haiku): 11 entity variable list calls become nd_draw_list(.., ND_SELF) (C1 build -11, C2 gfxpos -11)
Designer: the oracle (SendMessage). Worktree build3ds/wt/EVAR, branch EVAR. PM_ROW=EVAR PM_LANE=EVAR. Start now.
No shared file with MODELRM, GMERLEE, FXWRAP, WSCMB, PBUF, NMDL0, MAPCONV.
## Objective and counters
- FACT (code read): each site draws a variable whose value set is a fixed list set. Every list of the set is a self-contained v2 table
  (build3ds/out/ntab_tables.txt). That is the codemod ND_SELF rule (codemod.py self_ok) that ROOTCM applied byte-equal (merge 59cd904491).
  Sites and sets: Chest.c:107 (Entity_Chest_RenderLid :106), HiddenPanel.c:33 (Gfx_HiddenPanel_RenderHole), SaveBlock.c:54
  (Entity_SaveBlock_RenderBlock :45), ShatteringBlock_common.c:195 (4 arrays, ShatteringBlock.c:134-149), WoodenCrate.c:215, default/BoardedFloor.c:202,
  default/BombableRock.c:227 (one array each: :66, :53, :78), default/Padlock.c:49 (4 lists :201-228), jan_iwa/ArrowSign.c:25 (:24),
  jan_iwa/Plants1.c:52 (D_0A000D18_E9D618 :51), sbk_omo/StarBoxLauncher.c:48. 3ds/tests/evar/evarset.py checks all 173 lists.
- Not in this page (inheriting tables, ntab_inherit.txt, they need an entry): BlueWarpPipe.c:160, HeartBlock.c:54 :69, SaveBlock.c:42,
  SuperBlock.c:150 :165 :180, Plants1.c:177, Tweester.c:43 :61. Do not edit them.
- FACT (oracle listcen run, main eaf3865490 with the patch): build 518 -> 507 (-11), gfxpos 153 -> 142 (-11). data, data_arrays, gfxtype,
  recdl, reader, model: 0. The 11 files pass gcc -fsyntax-only. evarset on main: `evarset lists 173 self 173 bad 0`, --seed exits 1.
## Files (all in docs/handoff/ws_EVAR.patch). Edit only these 14.
The 11 src/entity files above, new: 3ds/tests/evar/evarset.py, 3ds/tools/azt.d/EVAR.json, 3ds/tools/bench.d/EVAR.json.
## Edit shape (the patch does it; use it to check a reject)
Before (src/entity/default/Padlock.c:49): `    gSPDisplayList(gMainGfxPos++, data->shackleGfx);`
After: `    nd_draw_list(data->shackleGfx, ND_SELF);`. ShatteringBlock_common.c also gets `#include "nd_gtab.h"` after nd_xf.h.
## Loop (1 build, 2 Azahar starts; base runs not charged)
1. `python 3ds/tools/listcen.py > build3ds/out/evar_lc0.txt`.
2. `tr -d '\r' < docs/handoff/ws_EVAR.patch | git apply -`. A reject: stop, send the output to the oracle.
3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/evar_host.txt 2>&1`. Read the last 20 lines. Pass: `ND HOST TESTS PASS`.
4. `python 3ds/tools/listcen.py > build3ds/out/evar_lc1.txt`. Pass vs evar_lc0.txt: total build -11, gfxpos -11, no other total line changes.
   Commit the 14 files: `EVAR 1: 11 entity variable list calls take nd_draw_list ND_SELF; evarset check; evar_iwa01_drec bench`.
5. One build. Pass: rc 0, 0 compile errors, memcheck margin above 0.
6. `python 3ds/tests/evar/evarset.py build3ds/out/ntab_tables.txt` (the build writes the file: gen_gfx_tab.sh:28). Pass: exit 0,
   `evarset lists 173 self 173 bad 0`. Then the same command with `--seed`. Pass: exit 1.
7. `python 3ds/tools/bench.py base spot_mac01_drec` and `python 3ds/tools/bench.py base evar_iwa01_drec`. Pass: each base log has its drec line.
8. Gate, for b in spot_mac01_drec evar_iwa01_drec: `python 3ds/tools/bench.py run $b --row EVAR --why "EVAR gate"`, then
   `python 3ds/tools/bench.py cmp $b --row EVAR --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each.
   Coverage: FACT mac_01/entity.c:14 save block, iwa_01/entity.c:5-7 save block and 2 arrow signs. GUESS: they draw in frames 200-201.
## Fail
- A failing test or a compile error: you may edit only the 14 files. Compare each with the patch. 2 tries for one cause.
  Then write the output to build3ds/out/ and send the path to the oracle.
- evarset bad above 0, a base run that fails or hangs, or a byte diff in a gate: stop. Send the output path to the oracle.
## Stop
Both gates pass. The last commit `EVAR 2: gate` deletes ws_EVAR.md and ws_EVAR.patch.
## Hand-back (one SubagentHandback message)
Commits. evarset line. listcen build and gfxpos before and after. memcheck margin. The 2 cmp lines.
