# ws_AXNS (Haiku): the A(x) namespace lists of kzn2 lava_piranha.c and sam2 dlist.c move to the twin (C3 data, C4 data_arrays, gfxtype)
Designer: the oracle (SendMessage; the orchestrator gives the id). Worktree build3ds/wt/AXNS, branch AXNS. PM_ROW=AXNS PM_LANE=AXNS.
Start now. Parallel with CGFX, DLSITE1, MODELRM (no shared file). GMERLEE starts after Merge AXNS (shared gen_gfx_syms.py, gconvw.sh).
Cause (FACT, code read): lava_piranha.c:15 and sam2/dlist.c:3 have `#define NAMESPACE A(x)`. NS_DEF `(\w+)` reads only `A`
(gen_gfx_syms.py:21, gset.py:34). The C name is AREA_x_name (include/macros.h:44, :200; `#define AREA b_area_kzn2`, kzn2/area.h:6).
Objective (FACT, oracle dry run on a copy of main 93a5a7bd50 with the 6 names as ok): `gset moved 6 files 2 keep 0`.
listcen after vs before (FACT, code read): data -53, data_arrays -6, gfxtype -6, build 0, gfxpos 0. Bench difference: none (0 differing bits).
GUESS: the converter marks all 6 lists ok: b_area_kzn2_lava_piranha_DList1..4, D_8021CF40_5A3620, b_area_sam2_monstar_dlist.
Files: ws_AXNS.patch (3ds/tools/gen_gfx_syms.py, 3ds/tools/gset.py, 3ds/tests/host.d/gset.sh, 3ds/tests/host.d/gconvw.sh; new 3ds/tools/azt.d/AXNS.json,
bench.d/AXNS.json, gconv_src.d/AXNS.txt, gset.d/AXNS.txt), src/battle/area/sam2/actor/monstar.c, and the files gset.py writes
(lava_piranha.c, sam2/dlist.c, 2 twins in 3ds/tools/gfxin). No other file.
## Edit 1: `git apply docs/handoff/ws_AXNS.patch` (the oracle checked it on main 93a5a7bd50; the new host tests pass on a copy)
## Edit 2: src/battle/area/sam2/actor/monstar.c, 3 lines (gset.py does not rewrite this file: it is in actor/, not next to dlist.c)
After :5 `#include "include_asset.h"` add the line `#include "nd_gtab.h"`.
:9 Before `extern Gfx N(dlist)[];`. After `NDN_DECL(N(dlist));`.
:157 Before `    .displayList = N(dlist),`. After `    .displayList = NDN(N(dlist)),` (same form as src/world/area_sam/sam_05/npc.c:60).
## What gset.py writes (dry run FACT); use it to check the result
lava_piranha.c:40-42 `Gfx N(DList1)[] = {` .. `};` becomes `NDN_DECL(N(DList1));`. :46 `ems_Draw(N(DList1), 60)` becomes `ems_Draw(NDN(N(DList1)), 60)`.
lava_piranha.c:111 `#include "battle/area/kzn2/lava_piranha_vine.gfx.inc.c"` becomes `NDN_DECL(D_8021CF40_5A3620);`.
lava_piranha.c:447 becomes `    gSPDisplayList(gMainGfxPos++, NDN(D_8021CF40_5A3620));`. sam2/dlist.c keeps `NDN_DECL(N(dlist));` only.
## Loop (1 build, 2 Azahar starts; base runs not charged)
1. Edit 1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/axns_host1.txt 2>&1`. Pass: `ND HOST TESTS PASS`, `gset test ok`, `gconvw ok`, conv2c `bad 0`,
   `grep -cE "^(b_area_kzn2_lava_piranha_DList[1-4]|D_8021CF40_5A3620|b_area_sam2_monstar_dlist)$" build3ds/out/gset_ok.txt` prints 6.
   Commit `AXNS 1: gen_gfx_syms and gset read NAMESPACE A(x); axns benches`. A count below 6: put the ntab_unsup.txt lines of the
   missing names in build3ds/out/axns_unsup.txt. Send the path to the designer. Stop.
2. `python 3ds/tools/bench.py base axns_kzn2_drec` and `python 3ds/tools/bench.py base axns_sam2_drec`. Pass: each base log has `drec f440 draws`.
3. `python 3ds/tools/listcen.py > build3ds/out/axns_lc0.txt`. Then Edit 2. Then
   `python 3ds/tools/gset.py 3ds/tools/gset.d/AXNS.txt build3ds/out/gset_ok.txt > build3ds/out/axns_gset.txt 2>&1`. Pass: `gset moved 6 files 2 keep 0`, `gset ok`.
4. Host test again (> axns_host2.txt). Pass: `ND HOST TESTS PASS`, `lkey census ok`, `gconv gset ok` and no `not moved` line, conv2c `bad 0`.
   Commit `AXNS 2: gset moves the A(x) lists`. One build: rc 0, 0 compile errors, memcheck margin above 0.
   `python 3ds/tools/listcen.py > build3ds/out/axns_lc1.txt`. Pass vs axns_lc0.txt: data -53, data_arrays -6, gfxtype -6, build 0, gfxpos 0.
5. Gate, for b in axns_kzn2_drec axns_sam2_drec: `python 3ds/tools/bench.py run $b --row AXNS --why "AXNS gate"`, then
   `python 3ds/tools/bench.py cmp $b --row AXNS --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each.
Fix scope: Edit 2 and the lines that gset.py writes. A compile error in monstar.c: compare the 3 lines with Edit 2.
A compile error on a gset line: compare it with the dry run lines above. A byte diff or another failure after 2 tries: send the output path to the designer.
Stop: both gates pass. The final commit `AXNS 3: gate` deletes ws_AXNS.md and ws_AXNS.patch.
Hand-back (one SubagentHandback message): commits, gset_ok count, gset moved line, listcen data data_arrays gfxtype, memcheck margin, the 2 cmp lines.
