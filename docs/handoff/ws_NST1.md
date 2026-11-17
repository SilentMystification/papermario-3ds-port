# ws_NST1 (Sonnet, user yes 2026-11-17): the core 1 draw state NdDs and nd_ds_apply (stage S3; no listcen counter)
Designer: oracle PHASEE2. Waits for nothing (not PSOFB). Worktree build3ds/wt/NST1, branch NST1. PM_ROW=NST1 PM_LANE=NST1.
Read first: docs/handoff/design_nstate.md sections 1, 3, 4. Edit only: new 3ds/include/nd_ds.h, new 3ds/src/nd_ds.c, 3ds/src/nd_rec.c
(the hook), new 3ds/tools/nd_ds_test.c, new 3ds/tests/host.d/nst1.sh. Core 0 code does not change.
## Facts (code read, main 7733aa14d8)
- Every setter unit passes nd_rec_nr (nd_rec.c:375), also the in-line state lists (rec_inline :412) and the non-table particle units (:311).
- Core 0 applies a unit in nd_op_apply (nd_set.c:159-183). The state writes sit in: set_geom :32, set_fogcolor :43, set_scissor :46
  (pm_wide sign rule), set_primdepth :64, set_fillcolor :69, set_blendcolor :71, nd_set_prim :74, set_om :83, gbi_setenv :99, set_mw :107,
  set_rare :148; op_setcombine nd_comb.c:488 (comb words and the derived comb_set, comb_alpha_shade, box_prod, dual_on, tst1 fields);
  nd_tex_set.c set_timg :48, set_tile :60, set_loadblock :90, set_loadtile :107, set_loadtlut :122, nd_set_tilesize :218, set_texture :222;
  nd_mtx_set.c gbi_movemem :180 (lights, look-at, viewport), nd_light_count :226.
- Globals that the writes touch (design F3): nd_comb.c:23-31, nd_set.c:25-26 and :41, nd_mtx_set.c:18-26. Initial values: nd_state.c:11.
## Edits
1. nd_ds.h: `typedef struct NdDs { NdWs w; <one field per F3 global, same name and type>; } NdDs;` `extern NdDs nd_ds;`
   `unsigned nd_ds_apply(NdDs* s, uint32_t k, uint32_t v);` (bit 0: a field value changed)
   nd_ds.c defines `NdDs nd_ds` with the core 0 initial values (nd_state.c:11 and the global initializers, e.g. comb_alpha_shade 1).
2. nd_ds.c: copy the state writes of each handler above into the switch of nd_ds_apply, on s-> fields. Leave out every pm_gpu_* call,
   pm_gpu_flush, pm_tex_set_filter, sync_combine, GP_SET, lit_freeze, logs, and the draw-time fields (design section 3 list).
   Keep the order of writes inside a handler. A handler that reads a draw-time field (box_folded, td[]) for a state write: use the s-> copy.
   Before (nd_set.c:99-104) `nd_ws.envc = w1; pm_gpu_set_env(nd_ws.envc); sync_combine();` After (nd_ds.c) `case NDO_ENV: ch = s->w.envc != v;
   s->w.envc = v; break;`.
3. nd_rec.c:375 nd_rec_nr: after the untyped check, before `rec_op_t(t);` add `nd_ds_apply(&nd_ds, w0, w1);`. No reset anywhere: core 0 never
   resets nd_ws (FACT: nd_state.c:11 static init only, git grep), so its state runs on across tasks and frames.
4. nd_ds_test.c + host.d/nst1.sh: feed 12 fixed units (OMH cycle 2, OML render mode, GEOM clear and set, PRIM, ENV, COMB 2-cycle, TIMG,
   TILE, LOADBLOCK, TILESIZE, NUMLIGHT, SEG) and compare every changed field with a table of expected values. Print `nst1 fields <n> bad <b>`.
   `--fault` changes one expected value: the script requires bad above 0 first, then bad 0 without it.
## Steps (each output to build3ds/out/, read the last 20 lines)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nst1_host.txt 2>&1`. Commit `NST1 1: core 1 draw state nd_ds (copy of the
   state writes), nd_rec_nr hook, nst1 host test`.
2. `python 3ds/tools/bench.py base spot_mac01_drec,wscmb_pause_drec --row NST1 --commit $(git merge-base HEAD main)` (not charged).
3. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row NST1 --bench spot_mac01_drec,wscmb_pause_drec --why "NST1 gate" > build3ds/out/nst1_loop.txt 2>&1`.
## Tests that decide (1 build, 2 Azahar starts)
- Host PASS with `nst1 fields <n> bad 0` and the seeded fault seen. LOOP PASS: both benches byte-equal. listcen totals equal to main.
- The proof that nd_ds equals core 0 is row NST3 (dschk). This row proves only the build, the host table and that core 0 output is equal.
## Fix rights: the 5 files. A DIFF means core 0 changed: undo the change outside the hook. Compile errors in your files: fix. Else stop and
  send the output path to oracle PHASEE2 (or its successor named in oracle_handoff.md). Wait.
## Stop: last commit `NST1 2: gate. 2 benches byte-equal; nst1 bad 0` deletes this page. One SubagentHandback. After the merge the
  orchestrator asks the user for one prof push (UD3: core 1 cost).
