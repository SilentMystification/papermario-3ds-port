# Row S4SNOWG: census speed gate of the S4P branch (snowfall), fncov call counts on Azahar

(Model: Haiku. The S4P coder runs it in build3ds/wt/S4P on branch S4P (ready hash d447ad40c, byte gates passed). No source edit.)

## Why
Console runs are one per merged batch (orchestrator_rules.md). The speed gate of a row is the deterministic fncov call count per class
(Azahar counts are exact; time is not). Stored values: FXFNCOV (build3ds/out/fxfncov_classes.txt), snowfall (fxfncov_63) minus its base
scene, calls per frame: A +1257, C +495, D +648, F +7015, G +1208, I +1158. Expected by code read (row S4P): per part the old path ran
nd_geom_vtx, 8 gv_tri, 7 gappend, 1 gtrip, a gv_epoch_enter (ensure_tile, nd_tex_get) and nd_gv_params, and ent_gdl (ent_find); the
new path runs 1 gtrip and 7 gappend per part, one epoch enter per record and no ent_gdl.

## Steps
1. `PM_FNCOV=1 PM_ROW=S4SNOWG PM_LANE=S4P sh 3ds/build.sh > build3ds/out/build_fncov.txt 2>&1` (census lane lane-S4P_fncov).
2. One list (2 Azahar starts): `python 3ds/tools/bench.py run fxfncov_base,fxfncov_63 --row S4SNOWG --lane S4P_fncov --why "S4 speed
   gate: class calls of snowfall on the native particle path"`.
3. `python 3ds/tools/hotset.py --fncov build3ds/azt/logs/fxfncov_base.txt --lane S4P > build3ds/out/s4g_base.txt` and the same for
   fxfncov_63 (> build3ds/out/s4g_63.txt). Per class, delta = calls of fxfncov_63 minus calls of fxfncov_base (the `hotset M3 ... calls`
   or per-class call line the tool prints; the same reading as fxfncov_classes.txt).

## Gate (pass rule, all must hold)
F delta < 2806 (40 % of 7015); I delta < 290 (25 % of 1158); G delta < 302 (25 % of 1208); C delta < 198 (40 % of 495);
A delta < 754 (60 % of 1257). D delta: report only (the flushes stay: 8 matrices per batch). A failed rule: stop, commit, hand back
both files (no new try).

## After the merge (orchestrator, not this row)
One console run for the merged batch: `warp=mac_01:0 pos=0:0:200 fxtest=all` on the XL, fxcen_report.py, snowfall walk compared with
xl4a (build3ds/out/fxcen_xl4a.txt: 18099 us above base). No base rerun, no repeat.

## At the end
Commit (no row file change other than deleting this one in the completing commit). Hand back: the per-class deltas, pass or fail.
