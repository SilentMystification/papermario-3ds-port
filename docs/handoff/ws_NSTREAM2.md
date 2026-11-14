# ws_NSTREAM2: a run of native records is an NDC_NAPI record (runner nd_nr_run); NDC_STATE goes; record runs leave the list executor name (design_lookahead 8)
Model: Haiku. PM_ROW, PM_LANE = NSTREAM2. Budget: 2 Azahar runs. Start after `Merge GKEY2` and `Merge NSTREAM1` show in `git log --oneline main` (GKEY2 edits
nd_rec.c and nd_walk.c; NSTREAM1 adds nd_nr.h). Branch NSTREAM2. Files: 3ds/include/nd_cmd.h, nd_api.h, nd_nr.h, 3ds/src/nd_rec.c, nd_walk.c, nd_model.c, nd_scomp.c,
pm_phase.c, 3ds/tools/nd_st_test.c, nd_geo_test.c, nd_tx_test.c, nd_val_test.c, nd_xf_test.c, 3ds/tests/host.d/stunit.sh, and the comment lines of steps 2 and 11.
Counter: C8 falls (GUESS about 30; report listcen reader before and after). Part of it is the rename nd_rec_state -> nd_rec_nr: the commit text says so.
nd_nr_exec is nd_walk_state in this step, so every unit runs as before: equal by construction. Anchors are quoted text (GKEY2 moves the lines).
- Facts (main a37368582): st_put writes the run header (nd_rec.c:342-344), stream_run (:578) and nd_r11_walk (nd_model.c:285) read it. NAPI and st_put both allow 1-255 units (:270, :330).
  stream_run calls a runner with p + 2 (:579): a runner reads its header at rec[-2]. nd_rec_state callers: nd_rec.c, nd_scomp.c:149, nd_nr.h, nd_xf_test.c:26.
## Steps (one build, one gate, one commit `NSTREAM2: ...`)
1. nd_cmd.h: the two lines of `NDC_STATE = 9, ...` -> `NDC_NRUN = 9,     /* NSTREAM: a counter slot only (nd_rec_cnt_take): the runs of native records; a run is NDC_NAPI with runner nd_nr_run */`.
   The `void nd_rec_state(...)` line -> `void nd_rec_nr(uint32_t k, uint32_t v); /* NSTREAM: one native record (nd_nr.h) into the open run (a new run after any other command) */`.
2. Rename: `grep -rl --include=*.c --include=*.h nd_rec_state 3ds src include | xargs sed -i 's/\bnd_rec_state\b/nd_rec_nr/g'` (never a .py file: listcen.py keeps its regex).
3. nd_api.h after the `void nd_walk_state(` line add: `void nd_nr_exec(const uint32_t* u, unsigned n); /* nd_walk.c, core 0: n native records (nd_nr.h) */` and
   `void nd_nr_run(const uint32_t* rec); /* nd_walk.c, core 0: the NDC_NAPI runner of a run; n = the units field of its header at rec[-2] */`.
4. nd_rec.c: delete the line `void nd_walk_state(const uint32_t* q, unsigned n); /* nd_walk.c: a run of state ops */`. In st_put:
   Before `ndc_n[NDC_STATE]++;` / `p[0] = NDC_HDR(NDC_STATE, 1, 0);` / `p[1] = 0;`
   After  `ndc_n[NDC_NRUN]++;` / `p[0] = NDC_HDR(NDC_NAPI, 1, 0);` / `p[1] = (uint32_t)(uintptr_t)nd_nr_run;`. In stream_run delete the line `case NDC_STATE: nd_walk_state(...); break;`.
5. nd_walk.c: after the closing `}` of `void nd_walk_state(const uint32_t* q, unsigned n) {` add
   `void nd_nr_exec(const uint32_t* u, unsigned n) { nd_walk_state(u, n); } /* NSTREAM2: the record runs; stage 1D gives it the arms and deletes nd_walk_state */`
   `void nd_nr_run(const uint32_t* rec) { nd_nr_exec(rec, (rec[-2] >> 16) & 0xffu); }`
   Then `nd_walk_state(` -> `nd_nr_exec(` in: nd_parts_run (2 lines), nd_parts_run_v (2 lines, the `dbg` branch), gt_ops (the NDG_DEF, NDG_UNIT, NDG_VTX and NDG_TRI lines),
   nd_ent_pieces (2 lines), nd_ntab_run, and the state-only branch (`nd_walk_state(lst, 1u);`). Keep nd_walk_state in: its forward declaration and definition, walk_call
   (`ND_LIST_MAX`), nd_ops_run (`nd_walk_state((const uint32_t*)l, n);`), gt_call (`c->list, ND_LIST_MAX`) and the debug call (the `nd_walk_state(u, 1u);` after `ND_OP_CALL_W0`).
6. nd_scomp.c: `nd_walk_state(` -> `nd_nr_exec(` in sc_exec (2 lines) and s6_run (1 line).
7. nd_model.c: after `#include "nd_cmd.h"` add `#include "nd_api.h"`. In nd_r11_walk Before `case NDC_STATE:` After
   `case NDC_NAPI:` and the next line `if (p[1] != (uint32_t)(uintptr_t)nd_nr_run) return 0; /* NSTREAM2: only a run of native records */` (the loop below stays).
8. pm_phase.c: `C1(n10[NDC_STATE])` -> `C1(n10[NDC_NRUN])`. nd_nr.h: the nd_nr line now names nd_rec_nr (step 2 did it).
9. Host doubles: in nd_st_test.c, nd_geo_test.c, nd_tx_test.c, nd_val_test.c Before `void nd_walk_state(const uint32_t* q, unsigned n) {`
   After `void nd_nr_run(const uint32_t* q) { const unsigned n = (q[-2] >> 16) & 0xffu;` (body unchanged). nd_xf_test.c: step 2 renamed its double.
10. Seeded check. nd_st_test.c main, before the `printf("stunit ok` line add:
   `run_one(n_cyc, &b); if (b.nw < 4u || b.w[0] != NDC_HDR(NDC_NAPI, 1, 0) || b.w[1] != (uint32_t)(uintptr_t)nd_nr_run || b.nu != 2u`
   `    || b.u[0] != NDO_W0(NDO_OMH, G_MDSFT_CYCLETYPE | 2u << 8) || b.u[1] != G_CYC_1CYCLE) { printf("stunit FAIL run\n"); return 1; }`.
   stunit.sh after the cyc seed: `sed 's/p\[0\] = NDC_HDR(NDC_NAPI, 1, 0);/p[0] = NDC_HDR(NDC_NRUN, 1, 0);/' 3ds/src/nd_rec.c > /tmp/nd_rec_seed.c`, link the test with
   /tmp/nd_rec_seed.c in place of 3ds/src/nd_rec.c, and require the output `stunit FAIL run` (same form as the cyc seed lines).
11. The code lines are done. `grep -rlw NDC_STATE 3ds/src 3ds/include` now lists comment lines only: run `sed -i 's/NDC_STATE run/native record run/g; s/\bNDC_STATE\b/native record run/g'`
   on those files. Check: `grep -rnE "NDC_STATE|nd_rec_state" 3ds/src 3ds/include src include` prints nothing.
## Gate (1 build, 2 Azahar runs)
nd_host PASS with both stunit seeds seen. Build rc 0, memcheck margin 310 KB or more. `bench.py run` then `cmp --commit <merge-base>` byte-equal: gold_12_drec, fxpic_91_drec.
Commit `NSTREAM2: ...` with the listcen reader line before and after. The commit deletes this page.
Fix rights: the files above. A FAIL or a drec difference: 2 tries per named cause, then no commit, write build3ds/out/nstream2.txt, send the path to the oracle. Hand back: 8 lines (hash, seed lines, drec results, reader count, margin; details build3ds/out/handback_NSTREAM2.txt).
