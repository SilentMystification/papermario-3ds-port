# Row CONV2D2: nd_ntab_run (core 0) runs the NDN_UNIT copy of a v2 table in place of the v1 table ops; nd_ntab_find

Blocked by CONV2D (part 1: NDN_UNIT) merging. Re-check the line numbers below against that merge.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. The design is fixed in row_CONV2D.md: units
through nd_walk_state, no pso apply, no walker handover.)
## Facts (ORACLE8, main 0250d5221)
- nd_gtab_run (3ds/src/nd_walk.c:568-606) is core 0. Its last arm runs the list start (gv_epoch++, dlc_on, gp_prev_op,
  pm_walk_side, ex_depth) and then `gt_ops(t);` (:601). gt_ops (:516) runs NDG_UNIT and DEF units with nd_walk_state.
- nd_gtab_find (3ds/src/nd_gtab.c:17-27) is a binary search with `>> 1` and no division, and a lazy insertion sort (gt_sort
  :8-16). nd_ntab_find has the same form. Only core 0 calls it (nd_gtab_run, ENTRYCAP), so the lazy sort has one caller core.
- The census gtr_cen (nd_gtab.c:43-48, core 1) reads the core 0 counters nd_gtr_ent/dlc/ops and logs `gtabrun f%u ...`.
- XF form: the converter keeps the low byte of the list's G_MTX w0 (already XORed by gsSPMatrix). nd_xf.c:14 XORs because its
  input is the raw gSPMatrix argument. The runner does not read NDN_XF (the unit copy holds the original MTX unit).
- Hot loop (hw_guidelines.md): the runner is one call `nd_walk_state(p, b)` per table (sequential 8-byte units, no division).
Start: `sh 3ds/tools/wt_new.sh CONV2D2`. PM_ROW=CONV2D2 PM_LANE=CONV2D2 on every call.
## Touch only (places)
3ds/include/nd_gtab.h (NdNIdx, 2 prototypes), 3ds/tools/nd_gfx_conv.c (v2_convert_all: the device index after the tables),
3ds/src/nd_gtab.c (nd_ntab_find and nd_ntab_run after nd_gtab_find; gtr_cen one line), 3ds/src/nd_walk.c (nd_gtab_run :601
only), new 3ds/tools/azt.d/CONV2D2.json and 3ds/tools/bench.d/CONV2D2.json.
## Exact edits
1. Commit 1 holds only the scene. azt.d/CONV2D2.json fxpic_89_drec is the fxpic_89 scenario of azt_scenarios.json. Its `debug` is
   `warp=mac_01:0 pos=0:0:200 fxtest=89 loglevel=debug det drec=412:2`. Its until is `drec f413 draws [1-9]\d* bytes \d+`, and
   expected_events are the warp and that until. It has cap 40, deadline 35 and needs_picture false. bench.d/CONV2D2.json:
   fxpic_89_drec, kind azahar_pic, tags azahar det drec, as spot_mac01_drec in bench.d/HOSTCORE2.json.
2. nd_gtab.h: `typedef struct NdNIdx { const void* list; const NdNTab* tab; } NdNIdx;` `extern NdNIdx ndn_dev_idx[];`
   `extern const unsigned ndn_dev_n;` `const NdNTab* nd_ntab_find(const void* list);` `void nd_ntab_run(const NdNTab* t);`
   (comments: core 0).
3. nd_gfx_conv.c v2_convert_all, after the loop that prints the tables, prints to stdout:
   `NdNIdx ndn_dev_idx[<n or 1>] = { { (const void*)NAME, &ndn_NAME }, ... };` and `const unsigned ndn_dev_n = <n>u;`.
4. nd_gtab.c after nd_gtab_find: nd_ntab_find, a copy of gt_sort and nd_gtab_find over ndn_dev_idx and ndn_dev_n (own sorted
   flag). Then `void nd_ntab_run(const NdNTab* t) { const NdNOp* o = t->ops; if (o->op == NDN_UNIT) nd_walk_state((const uint32_t*)o->p, o->b); }`.
5. nd_walk.c :601: `{ const NdNTab* nt = nd_ntab_find(t->list); if (nt) { nd_gtr_ntab++; nd_ntab_run(nt); } else gt_ops(t); }`.
   Add `nd_gtr_ntab` to the `unsigned nd_gtr_ent, ...` line (:500). gtr_cen: one more line,
   `pm_log_evt("ntabrun f%u runs %u", f, nd_gtr_ntab - <its previous value>)`, under the same condition as the gtabrun line.
6. Commit: `CONV2D2: nd_ntab_run runs the unit copy of the v2 table in nd_gtab_run, nd_ntab_find`.
## Gate (expected values)
1. Build, memcheck OK. nd_host PASS (tables 922, bad 0).
2. At commit 1: `bench.py base fxpic_89_drec` and `bench.py base spot_mac01_drec`. After edit 6:
   `bench.py run fxpic_89_drec,spot_mac01_drec --row CONV2D2 --why "CONV2D2 drec"`, then `bench.py cmp` of each: drec.txt
   byte-equal. The fxpic_89_drec log has `ntabrun f\d+ runs [1-9]`.
3. Seeded fault first (a value change, then undo): `o->b - 1u` in nd_ntab_run. cmp of fxpic_89_drec must differ.
4. Budget 8 Azahar starts. Counter: none moves (runner row, blocker of CONV2F and ENTRYCAP).
## Stop and end
No `ntabrun ... runs [1-9]` line (the scene's lists take ent_gdl or dlc_try: hand back the gtabrun lines), a drec difference with
the real edit, a seed with no difference, a base pair that differs, or a tool fault: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
