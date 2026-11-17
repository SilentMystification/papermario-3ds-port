# ws_MISCP (Haiku): misc_particles part draw gets one captured entry per variation (C10 recdl -1)
Designer: oracle DESIGN3. Worktree build3ds/wt/MISCP, branch MISCP from main, PM_ROW=MISCP PM_LANE=MISCP.
Edit only src/effects/misc_particles.c; add 3ds/tools/azt.d/MISCP.json, 3ds/tools/bench.d/MISCP.json.
## Objective and counters
- FACT (nd_walk.c:537): entcap zeroes plod. The "30 tuples" of the old cap.txt differ only in plod (80 - animTime), so no value
  must move. FACT (fxrest.d/cap2.txt:15,16,40-45, keys = variation): v0 :40, v1 :15, v2 :41, v3 :16 (= v2), v4 :42, v5 :44 (= v4).
  v4 and v5 also show a second tuple (:43, :45: cw0 fcffcdff). FACT (assets D_09001818_3E0678.gfx.inc.c:15): the setup list of
  v4/v5 sets the combine of :42, not of :43. GUESS: :43 comes from another state; psochk decides.
- conv_k4: cap2 says 0x7f. FXDIG found 0 in a one-effect run on the same spot (3059a64b15). GUESS: 0. psochk decides.
- Move (FACT, listcen): recdl -1. No other counter moves.
## Edits (line numbers main 4e838cc7a1; the keyed form of entapply.py:23-26, as cold_breath.c:4-6 and :237-241)
1. After `#include "nd_gtab.h"` add 6 lines `ND_PSO_DESC(pe_misc_particles_330_k<k>, <cw0>, <cw1>, 0x1e182cf0, 0x0c184b50,
   0x00000000, 0x00000000, 0x00000000, 0x00000000, 1, 0, 1, 0, 0, 0, 1, 0, <K>, 0, 0, 0);` with
   k0: 0xfc3117ff, 0x5ffefe38, K 1. k1: 0xfc3116c2, 0x50fefe38, K 1. k2 and k3: 0xfcfffe60, 0xf50cf37f, K 2.
   k4 and k5: 0xfc1617ff, 0x5ffdfe38, K 2.
2. :330 before `            nd_rec_dl(D_E00E4D90[variation]);` after (6 cases, as entapply writes them):
```c
            switch (variation) {
                case 0: nd_draw_list(D_E00E4D90[variation], pe_misc_particles_330_k0); break;
                case 1: nd_draw_list(D_E00E4D90[variation], pe_misc_particles_330_k1); break;
                case 2: nd_draw_list(D_E00E4D90[variation], pe_misc_particles_330_k2); break;
                case 3: nd_draw_list(D_E00E4D90[variation], pe_misc_particles_330_k3); break;
                case 4: nd_draw_list(D_E00E4D90[variation], pe_misc_particles_330_k4); break;
                default: nd_draw_list(D_E00E4D90[variation], pe_misc_particles_330_k5); break;
            }
```
3. azt.d/MISCP.json and bench.d/MISCP.json: copies of azt.d/FXDIG.json and bench.d/FXDIG.json, name miscp_drec, debug
   `warp=mac_01:0 pos=0:0:200 fxtest=107,148-152 loglevel=debug det drec=412:6:44 psochk` (fxtest number = FXLIST line - 33),
   until `fxtest: done`, cap 40, deadline 35, drec event `drec f632 draws [1-9]\d* bytes \d+`. Text names MISCP.
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edits 1-3. `grep -c "nd_rec_dl(" src/effects/misc_particles.c` prints 0. Commit `MISCP 1: misc_particles entry per variation; miscp_drec`.
2. `python 3ds/tools/bench.py base miscp_drec --row MISCP --commit $(git merge-base HEAD main)` (base run, not charged).
3. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row MISCP --bench miscp_drec --why "MISCP gate" > build3ds/out/miscp_loop.txt 2>&1`.
## Tests that decide (1 build, 2 Azahar starts)
- `LOOP PASS`: host PASS, build rc 0, miscp_drec byte-equal.
- `grep "psochk end"` in the base run.log (build3ds/basecache/<merge-base>/miscp_drec/) and the row run.log: bad equal, draws higher.
- listcen total recdl 1 lower than on `$(git merge-base HEAD main)`.
## Fix rights: only the 3 files. A compile error, a DIFF, a higher bad (send the `psochk bad` lines) or a timeout: stop,
  send the output path to DESIGN3 and wait.
## Stop and hand-back
Last commit `MISCP 2: gate. miscp_drec byte-equal, psochk bad equal; recdl <n>` deletes this page. One SubagentHandback.
