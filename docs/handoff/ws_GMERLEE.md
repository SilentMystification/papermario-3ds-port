# ws_GMERLEE (Haiku): the 3 Merlee card lists of dro_02 npc_merlee.c move to the twin (C3 data, C4 data_arrays)
Designer: the oracle (SendMessage; the orchestrator gives the id). Worktree build3ds/wt/GMERLEE, branch GMERLEE. PM_ROW=GMERLEE PM_LANE=GMERLEE.
Start after Merge AXNS (shared 3ds/tools/gen_gfx_syms.py and 3ds/tests/host.d/gconvw.sh). Parallel with CGFX, DLSITE1, MODELRM.
Cause (FACT, code read): src/world/area_dro/dro_02/npc.c:53 has `#include "npc_merlee.c"`. npc_merlee.c alone does not compile:
it uses N(RitualStateTime) of npc.c:11. The host twin must include npc.c. gen_gfx_syms.py:91 (main) takes the scanned source from the
twin's first line, so it scans npc.c and misses the card symbols. The patch scans the twin path (device file) instead.
FACT (oracle run on main): the new gen_gfx_syms.py gives the same output for all 241 current sources (4323 symbols); every twin's
first line is its own path today (214 twins). The patch adds the twin 3ds/tools/gfxin/src/world/area_dro/dro_02/npc_merlee.c
with the one line `#include "src/world/area_dro/dro_02/npc.c"`. gset.py keeps a twin's first line (gset.py:243 `old[:1]`).
Objective (FACT, oracle dry run on a copy of main 93a5a7bd50 with the 3 names as ok): `gset moved 3 files 1 keep 0`.
listcen after vs before (FACT, code read): data -44, data_arrays -3, gfxtype 0, build 0, gfxpos 0. Bench difference: none (0 differing bits).
GUESS: the converter marks the 3 lists ok: dro_02_card_setup_gfx, dro_02_card_1_gfx, dro_02_card_2_gfx.
Files: ws_GMERLEE.patch (gen_gfx_syms.py, host.d/gconvw.sh; new azt.d/GMERLEE.json, bench.d/GMERLEE.json, gconv_src.d/GMERLEE.txt,
gset.d/GMERLEE.txt, the twin), and the files gset.py writes (npc_merlee.c, the twin). No other file.
## Edit 1: `git apply docs/handoff/ws_GMERLEE.patch` (the oracle checked it on main 93a5a7bd50 plus ws_AXNS.patch)
## What gset.py writes (dry run FACT); use it to check the result
npc_merlee.c:2 new line `#include "nd_gtab.h"`. :85 `#include "world/area_dro/dro_02/card_setup.gfx.inc.c"` becomes `NDN_DECL(N(card_setup_gfx));`
(same for card_1 :86 and card_2 :87). :234 `    gSPDisplayList(gMainGfxPos++, N(card_setup_gfx));` becomes
`    gSPDisplayList(gMainGfxPos++, NDN(N(card_setup_gfx)));` (same for :247 card_1 and :258 card_2).
The twin gets the 3 include lines after its first line. The 2 vtx includes (:83, :84) stay in npc_merlee.c.
## Loop (1 build, 2 Azahar starts; base runs not charged)
1. Edit 1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/gmerlee_host1.txt 2>&1`. Pass: `ND HOST TESTS PASS`, `gconvw ok`, conv2c `bad 0`,
   `grep -cE "^dro_02_card_(setup|1|2)_gfx$" build3ds/out/gset_ok.txt` prints 3. Commit `GMERLEE 1: the twin scan reads the device file; gmerlee bench`.
   A compile error of the twin or a count below 3: send build3ds/out/gmerlee_host1.txt to the designer. Stop.
2. `python 3ds/tools/bench.py base gmerlee_drec` and `python 3ds/tools/bench.py base spot_mac01_drec`.
   Pass: the gmerlee base log has `merlee: shuffle ret` and `drec f835 draws`. Another result: send the log path to the designer. Stop.
3. `python 3ds/tools/listcen.py > build3ds/out/gmerlee_lc0.txt`. Then
   `python 3ds/tools/gset.py 3ds/tools/gset.d/GMERLEE.txt build3ds/out/gset_ok.txt > build3ds/out/gmerlee_gset.txt 2>&1`. Pass: `gset moved 3 files 1 keep 0`, `gset ok`.
4. Host test again (> gmerlee_host2.txt). Pass: `ND HOST TESTS PASS`, `lkey census ok`, `gconv gset ok` and no `not moved` line, conv2c `bad 0`.
   Commit `GMERLEE 2: gset moves the card lists`. One build: rc 0, 0 compile errors, memcheck margin above 0.
   `python 3ds/tools/listcen.py > build3ds/out/gmerlee_lc1.txt`. Pass vs gmerlee_lc0.txt: data -44, data_arrays -3, gfxtype 0, build 0, gfxpos 0.
5. Gate, for b in gmerlee_drec spot_mac01_drec: `python 3ds/tools/bench.py run $b --row GMERLEE --why "GMERLEE gate"`, then
   `python 3ds/tools/bench.py cmp $b --row GMERLEE --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each.
Fix scope: the lines that gset.py writes. A compile error on a gset line: compare it with the dry run lines above.
A byte diff or another failure after 2 tries: send the output path to the designer.
Stop: both gates pass. The final commit `GMERLEE 3: gate` deletes ws_GMERLEE.md and ws_GMERLEE.patch.
Hand-back (one SubagentHandback message): commits, gset_ok count, gset moved line, listcen data data_arrays gfxtype, memcheck margin, the 2 cmp lines.
