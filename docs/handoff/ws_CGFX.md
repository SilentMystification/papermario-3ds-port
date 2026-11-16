# ws_CGFX (Haiku): 7 SetCustomGfx lists move to the gfxin twins; nd_rec_list reads a moved key through nd_lkey_end (C5 C6 C7)
Designer to ask: oracle DL2 (SendMessage; the orchestrator gives the id). Worktree build3ds/wt/CGFX, branch CGFX. PM_ROW=CGFX PM_LANE=CGFX.
Start after Merge GSETR. Parallel with ws_DLSITE1 (no shared file). ws_MODELRM starts after Merge CGFX (both edit 3ds/src/nd_rec.c).
Objective: the 7 lists leave the device sources. FACT (build3ds/out/gset_ok.txt on main): all 7 are ok. GUESS listcen: data_arrays -7, gfxtype -7.
Lists: N(setup_gfx_candle_lights) in src/world/area_kkj/kkj_03, kkj_13, kkj_14, kkj_15, kkj_18, kkj_27 /main.c; N(HazePreGfx) in src/world/area_isk/isk_01/haze.c.
Why by hand: gset.py holds every SetCustomGfx line (gset.py:160). Do not edit gset.py.
## Edit 1: 3ds/src/nd_rec.c, nd_rec_list (nd_rec.c:528). Add `#include "nd_lkey.h"` if the file has no such line.
Before: `void nd_rec_list(const void* l) {` / `    const Gfx* g = (const Gfx*)l;`
After: `void nd_rec_list(const void* l) {` / `    uint32_t kb[2u * 64u];` /
`    const NdNDev* ke = nd_ntab_find(l);` / `    const Gfx* g = (const Gfx*)(ke == (const NdNDev*)l ? (const void*)nd_lkey_end(l, ke, kb, 64u) : l); /* CGFX: a moved key (NDN) */`
Only a moved key takes the copy. Any other list keeps its words: the base bytes do not change.
Edit 3ds/tests/lkey/allow.txt line `3ds/src/nd_rec.c`: append ` ; nd_rec_list reads a moved key through nd_lkey_end (CGFX)` to the reason. Keep the count.
## Edit 2: the 7 sources (one shape, 6 kkj files and haze.c)
Before (src/world/area_isk/isk_01/haze.c:3-6): `Gfx N(HazePreGfx)[] = {` / `    gsDPSetAlphaDither(G_AD_NOISE),` / `    gsSPEndDisplayList(),` / `};`
After: one line `NDN_DECL(N(HazePreGfx));`. Move the block, unchanged, to the new twin 3ds/tools/gfxin/src/world/area_isk/isk_01/haze.c:
line 1 `#include "src/world/area_isk/isk_01/haze.c"`, then the block. Example twin: 3ds/tools/gfxin/src/world/area_dgb/dgb_01/gfx.c.
Before (haze.c:12): `Call(SetCustomGfx, 0, Ref(N(HazePreGfx)), nullptr)`. After: `Call(SetCustomGfx, 0, Ref(NDN(N(HazePreGfx))), nullptr)`.
Ref lines (9, FACT grep): kkj_03/main.c:63, kkj_13/main.c:86, kkj_14/main.c:228, kkj_15/main.c:130 and :140, kkj_18/main.c:123,
kkj_27/main.c:59 and :69, isk_01/haze.c:12. If a twin path exists already, append the block at its end.
## Edit 3: new bench files 3ds/tools/azt.d/CGFX.json and 3ds/tools/bench.d/CGFX.json (the CGFX coder builds them)
Copy the shape of azt.d/MESH1.json and bench.d/MESH1.json. Two scenarios: kkj14_cgfx_drec (`warp=kkj_14:0 loglevel=debug det drec=200:2`,
first_marker `warp: kkj_14 entry 0`) and isk01_cgfx_drec (`warp=isk_01:0 ...`, `warp: isk_01 entry 0`). until `drec f201 draws [1-9]\d* bytes \d+`, cap 40, deadline 35.
## Loop (1 build, at most 6 Azahar starts; base runs not charged)
1. Edit 3. Commit `CGFX 1: kkj14 and isk01 drec benches`. Run `bench.py base` for both (base = `git merge-base HEAD main`).
2. Edit 1 and Edit 2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/cgfx_host.txt 2>&1`. Pass: `ND HOST TESTS PASS`, `lkey census ok`, `gconv gset ok`
   with no `moved ... not moved` line, conv2c `bad 0`. Commit `CGFX 2: 7 SetCustomGfx lists move`.
3. One build: rc 0, 0 compile errors, memcheck margin above 0. `python 3ds/tools/listcen.py > build3ds/out/cgfx_lc.txt`. Write the totals in the commit.
4. Gate: kkj14_cgfx_drec, isk01_cgfx_drec, spot_mac01_drec: `bench.py run <b> --row CGFX --why "CGFX gate"`, then
   `bench.py cmp <b> --row CGFX --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each.
   Bench check: each base log has the until line and no `ndc: static list op` line (nd_rec.c:525). GUESS: the candle model draws in frames 200-201.
   OPEN (the user decides): a seeded fault of the read path costs 1 more build. Ask the orchestrator before you run it.
## Fix scope
Edits 1-3 only. A compile error in a source: fix the NDN or N() form in that line. A `not moved` line: the named caller moves the same way.
## Stop and hand-back
Stop: 3 gates pass. Final commit `CGFX 3: gate` deletes this page. Hand-back: build3ds/out/handback_CGFX.txt (commit, gate values, listcen totals, memcheck).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to oracle DL2 (SendMessage). Then stop.
