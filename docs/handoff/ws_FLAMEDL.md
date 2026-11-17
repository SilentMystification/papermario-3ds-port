# ws_FLAMEDL (Haiku): the last 2 list calls of src/effects/flame.c (C1 build -2, C2 gfxpos -2, C10 recdl 0 net)
Designer: oracle PHASEE2. Worktree build3ds/wt/FLAMEDL, branch FLAMEDL from main. PM_ROW=FLAMEDL PM_LANE=FLAMEDL.
Edit only src/effects/flame.c. Add 3ds/tools/azt.d/FLAMEDL.json, 3ds/tools/bench.d/FLAMEDL.json, 3ds/tools/fxrest.d/cap6.txt.
## Facts (code read, main 51332ca38e)
- flame.c:188 `gSPDisplayList(gMainGfxPos++, NDN(D_09000918_3544C8));` (FACT). The list has only state ops, no draw
  (FACT, assets/us/effects/gfx/D_09000918_3544C8.gfx.inc.c:3-16). GUESS: entcap logs it as `so` (nd_walk.c:636).
- flame.c:211 `gSPDisplayList(gMainGfxPos++, NDN(D_090008F8_3544A8));` (FACT). The list is pipesync, 4 vertices, 2 triangles
  (FACT, D_090008F8_3544A8.gfx.inc.c:3-8). It sets no combine. The preset list D_09000800 (flame.c:197) sets the full state
  before each draw. So the MISCP first/later split does not apply. GUESS: 1 tuple.
- flame is fxtest 27 (FACT, pm_fxtest.c:62, first entry :35). Slot 0 starts f400, 44 frames (FACT, fxops drec=412:5:44).
- flame.c has `#include "nd_gtab.h"` at :2 (FACT). gen_gfx_tab.sh:28 scans src/effects: no ntab_ent.d file.
## Edits
1. flame.c:188 and :211: change `gSPDisplayList(gMainGfxPos++, NDN(X));` to `nd_rec_dl(NDN(X));`, no comment (entapply.py:32).
   Before: `    gSPDisplayList(gMainGfxPos++, NDN(D_090008F8_3544A8));`  After: `    nd_rec_dl(NDN(D_090008F8_3544A8));`
2. azt.d/FLAMEDL.json and bench.d/FLAMEDL.json: copy the 2 entries of azt.d/FXSLOT.json and bench.d/FXSLOT.json. Change:
   flamedl_cap: debug `warp=mac_01:0 pos=0:0:200 det fxtest=27 loglevel=debug entcap entcapt psochk`, cap 25, deadline 20.
   flamedl_drec: debug `warp=mac_01:0 pos=0:0:200 fxtest=27 loglevel=debug det drec=412:2:20 psochk`, cap 25, deadline 20,
   drec event regex `drec f432 draws [1-9]\d* bytes \d+`. Keep the until `fxtest: done`. Write FLAMEDL in each text field.
## Steps (each output to build3ds/out/, read the last 20 lines)
1. Edits 1-2. Commit `FLAMEDL 1: flame.c:188 :211 nd_rec_dl capture form; flamedl_cap, flamedl_drec`.
2. `sh 3ds/build.sh > build3ds/out/flamedl_build1.txt 2>&1` (bench.py run does not build).
3. `python 3ds/tools/bench.py run flamedl_cap --row FLAMEDL --why "FLAMEDL capture"`. `python 3ds/tools/entcap.py <its run.log>
   > build3ds/out/flamedl_entcap.txt`. `grep -E "src/effects/flame.c:(188|211)\b" build3ds/out/flamedl_entcap.txt
   > 3ds/tools/fxrest.d/cap6.txt`.
4. `python 3ds/tools/entapply.py 3ds/tools/fxrest.d/cap6.txt > build3ds/out/flamedl_apply.txt`. Pass: last line
   `entapply sites 2 keyed 0 so 1 multi 0 kmiss 0 mixed 0 skip 0 other 0 files 1` (:188 ND_SELF, :211 pe_flame_211), or
   `... sites 2 keyed 0 so 0 ...` (2 descs). Commit `FLAMEDL 2: captured entries of flame.c:188 :211 (cap6.txt)`.
5. `python 3ds/tools/bench.py base fxops_drec,flamedl_drec --row FLAMEDL --commit $(git merge-base HEAD main)` (not charged).
6. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row FLAMEDL --bench fxops_drec,flamedl_drec --why "FLAMEDL gate"
   > build3ds/out/flamedl_loop.txt 2>&1`.
7. `grep -h "psochk end" ../../basecache/$(git merge-base HEAD main)/flamedl_drec/run.log <the flamedl_drec log of step 6>
   > build3ds/out/flamedl_psochk.txt`. Line 1 is the base. Line 2 is the gate.
## Tests that decide (2 builds, 3 Azahar starts; bases not charged)
- `LOOP PASS`: host PASS, build rc 0, fxops_drec and flamedl_drec byte-equal.
- psochk end: gate bad equal to base bad, gate draws higher than base draws, lkbad 0.
- `python 3ds/tools/listcen.py --group src/effects/flame.c` prints no `listcen file` line (build, gfxpos, recdl 0).
## Fix rights: only the 4 files. A multi, mixed, skip, kmiss or other line, no cap6 line for :211, a compile error, a DIFF,
  a higher bad or no psochk end line: stop. Send the output path to oracle PHASEE2 with SendMessage. Wait for the reply.
## Stop: last commit `FLAMEDL 3: gate. fxops_drec, flamedl_drec byte-equal; psochk bad <b> equal base, draws <base> -> <gate>`
  deletes this page. One SubagentHandback: hashes, LOOP line, psochk lines, listcen line.
