# ws_FXCULL (Haiku): green_impact, red_impact, shockwave part draws get their captured entry (C10 recdl -3)
Designer: oracle DESIGN3. Worktree build3ds/wt/FXCULL, branch FXCULL from main, PM_ROW=FXCULL PM_LANE=FXCULL.
Edit only src/effects/green_impact.c, red_impact.c, shockwave.c; add 3ds/tools/azt.d/FXCULL.json, 3ds/tools/bench.d/FXCULL.json.
## Objective and counters
- FACT (nd_walk.c:537, nd_psochk.c:12-16): entcap zeroes geom and plod, psochk does not compare them. So the 2 cull passes of
  the NdSub replay (classes.txt:3,5,6, from the old cap.txt) are one entry. Keep the NdSub code as it is.
- FACT (fxrest.d/cap2.txt, the FXREST2 capture): green_impact.c:202 one tuple (:4), red_impact.c:209 one tuple (:9).
  shockwave.c:341 two tuples (:3, :31) that differ only in conv_k4 (0 and 0x7f). FXDIG found the walker value 0 in a
  one-effect run (commit 3059a64b15). GUESS: 0 here too; psochk decides.
- FACT (shockwave.c:31-43): shockwave_main maps every type to unk_00 2..5, so D_E004AE60[0..1] never run. One setup list.
- Move (FACT, listcen): recdl -3. No other counter moves.
## Edits (line numbers main 7757625eb3; match the text)
1. green_impact.c: after the line `#include "nd_st.h"` add
   `ND_PSO_DESC(pe_green_impact_200, 0xfc3133ff, 0x5ffefe38, 0x1e182cf0, 0x0c184b50, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0);`
   :200 before `            nd_rec_dl(dlist2);` after `            nd_draw_list(dlist2, pe_green_impact_200);`
2. red_impact.c: the same desc words, name pe_red_impact_207. :207 `nd_rec_dl(dlist2);` -> `nd_draw_list(dlist2, pe_red_impact_207);`.
3. shockwave.c: desc pe_shockwave_339 = 0xfc60b2a0, 0x23fffef8, 0x1e182ca0, 0x0c184b50, four 0x00000000, then
   `1, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 0`. :339 `nd_rec_dl(dlist);` -> `nd_draw_list(dlist, pe_shockwave_339);`.
   Desc rule (entapply.py:20): w0..w7 as hex, then the 12 bytes of w8, w9, w10 little endian as decimals.
4. azt.d/FXCULL.json and bench.d/FXCULL.json: copies of azt.d/FXDIG.json and bench.d/FXDIG.json, name fxcull_drec, debug
   `warp=mac_01:0 pos=0:0:200 fxtest=32,44,98,143 loglevel=debug det drec=412:4:44 psochk` (fxtest number = FXLIST line - 33,
   pm_fxtest.c), until `fxtest: done`, cap 45, deadline 40, drec event `drec f544 draws [1-9]\d* bytes \d+`. Text names FXCULL.
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edits 1-4. `grep -c "nd_rec_dl(" src/effects/green_impact.c src/effects/red_impact.c src/effects/shockwave.c` prints 0 for each.
   Commit `FXCULL 1: green_impact, red_impact, shockwave part entries; fxcull_drec`.
2. `python 3ds/tools/bench.py base fxcull_drec --row FXCULL --commit $(git merge-base HEAD main)` (base run, not charged).
3. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row FXCULL --bench fxcull_drec --why "FXCULL gate" > build3ds/out/fxcull_loop.txt 2>&1`.
## Tests that decide (1 build, 2 Azahar starts)
- `LOOP PASS`: host PASS, build rc 0, fxcull_drec byte-equal.
- `grep "psochk end"` in the base run.log (build3ds/basecache/<merge-base>/fxcull_drec/) and the row run.log: bad equal, draws higher.
- listcen total recdl 3 lower than on `$(git merge-base HEAD main)`.
## Fix rights: only the 5 files. A compile error, a DIFF, a higher bad (send the `psochk bad` lines) or a timeout: stop,
  send the output path to DESIGN3 and wait.
## Stop and hand-back
Last commit `FXCULL 2: gate. fxcull_drec byte-equal, psochk bad equal; recdl <n>` deletes this page. One SubagentHandback.
