# ws_TST13 (Haiku): tst_13 floor shockwave list gets its captured entry (C1 build -1, C2 gfxpos -1)
Designer: oracle DESIGN3. Worktree build3ds/wt/TST13, branch TST13 from main AFTER the MDLTYPE and FXSLOT1 merges.
PM_ROW=TST13 PM_LANE=TST13. Edit only src/world/area_tst/tst_13/main.c; add 3ds/tools/azt.d/TST13.json,
3ds/tools/bench.d/TST13.json, 3ds/tools/ntab_ent.d/TST13.txt, 3ds/tools/fxrest.d/cap4.txt.
## Facts (code read, main 7ed772cad7)
- tst_13 main.c:306 `gSPDisplayList(gMainGfxPos++, NDN(N(shockwave_gfx)));` is the last game call on gMainGfxPos besides
  status_star_shimmer.c:189 (FXSLOT1, FXSLOT2). The warp switch ignores tst_* maps (pm_warp.c:747); FXSLOT1 adds the switch
  warptest (pm_warp.c:846). entcap logs only nd_rec_dl and nd_draw_list sites (nd_walk.c:644). entapply needs the plain line
  `nd_rec_dl(...);` (entapply.py:32) and names the desc pe_main_306 (entapply.py:15). NAMESPACE is tst_13 (tst_13.h:22).
- GUESS: tst_13 loads in Azahar and N(build_gfx_floor) runs by frame 200. The capture run decides.
## Steps (each output to build3ds/out/, read the last 20 lines)
1. main.c:306 -> `    nd_rec_dl(NDN(N(shockwave_gfx)));`. Add ntab_ent.d/TST13.txt: line 1 `# TST13: sites with a captured entry`,
   line 2 `src/world/area_tst/tst_13/main.c=tst_13` (the form of ntab_ent.d/ENTCAPR2.txt).
2. TST13.json (azt.d and bench.d, copies of the VCFIX.json forms), 2 scenarios and 2 benches, cap 25, deadline 25:
   tst13_cap: `warp=tst_13:0 warptest loglevel=debug det drec=200:1 entcap entcapt psochk`, until `drec f200 draws [1-9]\d* bytes \d+`.
   tst13_drec: `warp=tst_13:0 warptest loglevel=debug det drec=200:2 psochk`, until `drec f201 draws [1-9]\d* bytes \d+`.
   first_marker `warp: tst_13 entry 0`. Commit `TST13 1: tst_13 shockwave nd_rec_dl (capture form); tst13_cap, tst13_drec`.
3. `sh 3ds/build.sh > build3ds/out/tst13_build1.txt 2>&1` (bench.py run does not build; bench.py:1218-1225). Then
   `python 3ds/tools/bench.py run tst13_cap --row TST13 --why "TST13 capture"`. `python 3ds/tools/entcap.py <its run.log>
   > build3ds/out/tst13_entcap.txt`. `grep "tst_13/main.c:306" build3ds/out/tst13_entcap.txt > 3ds/tools/fxrest.d/cap4.txt`
   (only this site: entapply edits every site of its input). cap4.txt must hold one line.
4. `python 3ds/tools/entapply.py 3ds/tools/fxrest.d/cap4.txt > build3ds/out/tst13_apply.txt`. Expected: `entapply site
   src/world/area_tst/tst_13/main.c:306 pe_main_306` and the last line `entapply sites 1 ...`. Commit `TST13 2: captured entry`.
5. `python 3ds/tools/bench.py base tst13_drec --row TST13 --commit $(git merge-base HEAD main)` (base run, not charged).
6. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row TST13 --bench tst13_drec --why "TST13 gate" > build3ds/out/tst13_loop.txt 2>&1`.
## Tests that decide (2 builds, 3 Azahar starts)
- `LOOP PASS`: host PASS, build rc 0, tst13_drec byte-equal. psochk end line: bad equal to the base run, draws higher.
- listcen on HEAD vs `$(git merge-base HEAD main)`: build -1, gfxpos -1, no other change.
## Fix rights: only the 5 files. No `warp: tst_13 entry 0`, a crash, a missing cap4 line, a `multi` or `skip` line, a DIFF or a
  higher bad: stop, send the output path to DESIGN3 and wait.
## Stop: last commit `TST13 3: gate. tst13_drec byte-equal, psochk bad equal; build <n> gfxpos <m>` deletes this page.
One SubagentHandback: hashes, LOOP line, psochk lines, counters.
