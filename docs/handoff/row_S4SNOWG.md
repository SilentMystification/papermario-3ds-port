# Row S4SNOWG: census speed gate of the S4P branch (snowfall), fncov call counts on Azahar

(Model: Haiku. The S4P coder runs it in build3ds/wt/S4P on branch S4P (ready hash d447ad40c, byte gates passed). Tool edit only.)

## Why
Console runs are one per merged batch (orchestrator_rules.md). The speed gate of a row is the deterministic fncov call count per class
(Azahar counts are exact; time is not). Stored values: FXFNCOV (build3ds/out/fxfncov_classes.txt), snowfall (fxfncov_63) minus its base
scene, calls per frame: A +1257, C +495, D +648, F +7015, G +1208, I +1158. Expected by code read (row S4P): per part the old path ran
nd_geom_vtx, 8 gv_tri, 7 gappend, 1 gtrip, a gv_epoch_enter (ensure_tile, nd_tex_get) and nd_gv_params, and ent_gdl (ent_find); the
new path runs 1 gtrip and 7 gappend per part, one epoch enter per record and no ent_gdl.

## Next step (after try 1: F +5923, G +992, C +501, I +376, A +656, D +948; code e7b898e7b, tool f8d23b538)
Code read of e7b898e7b: snowfall records one particle run (nd_parts_add takes the units: the I and A drops show walk_call and
ent_gdl are gone). nd_part_draw (nd_geom.c:302-360) can still fall back per part (nd_walk.c:427, nd_ops_run: gv_epoch++ per part,
then nd_geom_vtx and gv_tri as before), and the class sums cannot tell a fallback from per-part work of the new path or from scene
activity between frames 396 and 420 (C and D hold model and submit calls of the whole frame). The census per function decides; the
two logs of try 1 hold it (build3ds/azt/logs/fxfncov_base.txt and fxfncov_63.txt): no new Azahar start.
A. hotset.py: a flag `--fncovfn` (with --fncov): after the class lines print one line per function of the census frame,
   `hotset fn <name> <calls>`, calls >= 20, largest first (fmap name; unmatched addresses as hex). Commit.
B. Run it on both logs (> build3ds/out/s4g_fn_base.txt, s4g_fn_63.txt); hand back, minus base, the calls of: nd_part_draw,
   nd_parts_run, nd_ops_run, nd_geom_vtx, gv_tri, draw_tri, gv_epoch_enter, gv_params_calc, nd_gv_params, ensure_tile,
   pm_gpu_gtrip, pm_gpu_gappend, gv_matrix_id, load_mtx, nd_tex_get, nd_raw_native, nd_native_submit, and the 15 largest deltas.
C. Stop and hand back (the oracle sets the fix or the pass numbers from these counts). No other edit.

## Steps (try 1)
0. 3ds/tools/hotset.py fncov_report (:298-317): next to `cb = Counter()` add `cc = Counter()`; in the class loop add `cc[c] += n`
   beside `cb[c] += b`; after the class print add `print("hotset M3 f%d calls %s" % (f, " ".join("%s %d" % (c, cc[c]) for c in CLASSES)))`.
   Commit it (a tool change: no game build needed for it).
1. `PM_FNCOV=1 PM_ROW=S4SNOWG PM_LANE=S4P sh 3ds/build.sh > build3ds/out/build_fncov.txt 2>&1` (census lane lane-S4P_fncov).
2. One list (2 Azahar starts): `python 3ds/tools/bench.py run fxfncov_base,fxfncov_63 --row S4SNOWG --lane S4P_fncov --why "S4 speed
   gate: class calls of snowfall on the native particle path"`.
3. `python 3ds/tools/hotset.py --fncov build3ds/azt/logs/fxfncov_base.txt --lane S4P > build3ds/out/s4g_base.txt` and the same for
   fxfncov_63 (> build3ds/out/s4g_63.txt). Per class, delta = the `hotset M3 f<n> calls` value of s4g_63 minus that of s4g_base.
   Tool check first: s4g_base calls must match the stored base (fxfncov_classes.txt: A 1371 B 1069 C 2320 D 5128 E 683 F 1286 G 1424
   H 326 I 1433) within 5 % per class (the base scene has no particles). Outside 5 %: a tool fault, stop and hand back both files.

## Gate (pass rule, all must hold)
F delta < 2806 (40 % of 7015); I delta < 290 (25 % of 1158); G delta < 302 (25 % of 1208); C delta < 198 (40 % of 495);
A delta < 754 (60 % of 1257). D delta: report only (the flushes stay: 8 matrices per batch). A failed rule: stop, commit, hand back
both files (no new try).

## After the merge (orchestrator, not this row)
One console run for the merged batch: `warp=mac_01:0 pos=0:0:200 fxtest=all` on the XL, fxcen_report.py, snowfall walk compared with
xl4a (build3ds/out/fxcen_xl4a.txt: 18099 us above base). No base rerun, no repeat.

## At the end
Commit (no row file change other than deleting this one in the completing commit). Hand back: the per-class deltas, pass or fail.
