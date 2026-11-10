# Row SEAM2: seams on building geometry that come and go with the camera (user, 2026-11-09)

(Model: Haiku.)

## User facts
"I also still see seams on building geometry sometimes. Fix those." (towns, mac_*).

## State
Branch SEAM2 05792cdd7 (base ad669a0ab). T points of lists under their own matrix (cls 2) split in object space (nd_seam.c, fixture in
3ds/tools/nd_seam_test.c seam_fixture_matrix). Log word renamed: `seamfix map ... mtx N` (was skip_mtx). Not merged. Missing: the run-time static-matrix check.
Premise: a split on the receiver edge in object space stays on that edge under any affine matrix, so a moving matrix cannot open a crack.
It only leaves a useless point. The check keeps cls 2 T points off lists whose model matrix is not static at compile time (R1 bit 8, src/model.c:1540).

## Start state
`cd build3ds/wt/SEAM2` (or `sh 3ds/tools/wt_new.sh SEAM2` if gone), branch SEAM2. First `git merge main` (main changed nd_shape.c and
nd_mtx_set.c since ad669a0ab; resolve by keeping both sides). PM_ROW=PM_LANE=SEAM2.

## Touch only
3ds/src/nd_mtx_set.c (load_mtx), 3ds/src/nd_recs.c (rec_matrix, rec_apply), 3ds/src/nd_shape.c (dlc_build, its 3 callers),
3ds/src/pm_xfer.c (wq_xh, pm_xfer_hidden). Already on the branch: nd_seam.c, 3ds/tools/nd_seam_test.c.

## Exact edits
1. nd_mtx_set.c: above load_mtx add `int nd_mtx_static; /* SEAM2: the last model matrix load was a static matrix (R1 bit 8) */`.
   First line of load_mtx: `nd_mtx_static = (hdr & 0x100u) != 0;` (the walker G_MTX passes hdr 0: not static).
2. nd_recs.c: add `extern int nd_mtx_static;`. rec_matrix: `if (!(f & 256u)) load_mtx(...); else nd_mtx_static = 1;`.
   rec_apply: after `if (!(f & 256u) && !mdone) load_mtx(...)` add `else if (f & 256u) nd_mtx_static = 1;` (IGNORE_MATRIX: no model matrix).
3. nd_shape.c: `int dlc_build(DlcEnt* e, const Gfx* l, int stat)`. After `sl = ...`:
   const int use_tp = sl && sl->cnt && (sl->cls < 2 || stat);
   if (sl && sl->cnt && !use_tp) pm_log_evt("seamfix dyn map %s list %08x skip %u", pm_shape_name, (u32)(uintptr_t)l, (unsigned)sl->cnt);
   In env: `use_tp ? st->tp + sl->first : NULL, use_tp ? sl->cnt : 0u`. In the pool-full log, test use_tp instead of sl && sl->cnt.
   Callers: pm_dlc_native_ranges and dlc_try pass nd_mtx_static (add `extern int nd_mtx_static;`).
   `int pm_dlc_prebuild(const void* lv, int stat)` passes stat.
4. pm_xfer.c: `typedef struct { const void* dl; int stat; } XhQ;`. wq_xh reads XhQ and calls pm_dlc_prebuild(q->dl, q->stat) (fix the local prototype).
   In pm_xfer_hidden: `XhQ q = { m->modelNode->displayData->displayList, m->finalMtx == &m->savedMtx && !(m->flags & MODEL_FLAG_BILLBOARD) }; pm_wq_post(wq_xh, &q, sizeof q);`

## Done criteria
1. nd_host PASS (`bench.py run nd_host`; seam fixture PASS line).
2. Log of spot_mac01_det, spot_mac00_det: `seamfix map mac_01_shape ... mtx N`, N > 0. Count the `seamfix dyn` lines (expected 0 on a static town; report each one).
3. Shared draw code: `bench.py base spot_mac01_det` and `bench.py base spot_mac00_det`, then `bench.py cmp <bench> --row SEAM2`. They differ at
   building seams only: report the diff pixel count and bbox per bench (branch value before: spot_mac01_det 953 px).
4. Picture: `bench.py run seam_mac01_east`: a building edge with no background line. memcheck OK. Commit.
Affected scenarios: spot_mac01_det, spot_mac00_det, seam_mac01_east.

## Stop
6 Azahar starts. 2 tries per named cause, then hand back with the values. STOP RULE: 90 min or 150 tool calls.

## Rules
CLAUDE.md rules 1-7. PM_LANE=SEAM2. No git stash. Foreground waits.

## Commands
- Build: PM_ROW=SEAM2 PM_LANE=SEAM2 sh 3ds/build.sh
- Test: PM_ROW=SEAM2 PM_LANE=SEAM2 python 3ds/tools/bench.py run <bench> --row SEAM2 --lane SEAM2 --why "<text>"
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget SEAM2

## At the end
Delete this row file in the completing commit. Hand back "SEAM2 ready <hash>".
