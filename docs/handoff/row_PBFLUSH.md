# Row PBFLUSH: measure which flush closes each pink_sparkles batch (existing PG0 flush tracer, no code edit)

(Model: Haiku. Base: main. No source edit: one scenario and its bench. Reads this file and CLAUDE.md.)

## Why
PBATCH2 (two tries, CLAUDE.md 5) left pink_sparkles at 44 draws per frame: tri = gvtri = 94.4 (the triangles take the ring path),
gvrej 0, gvfb 0, prg/shr 0. The code read does not show which flush closes each part's batch. pm_gpu.c already records the caller
of every flush: the flush() macro stores __func__ and __LINE__ (pm_gpu.c:606, fl_fn/fl_line :361-362), fl_count tallies them per
cause (:369-372) and pm_gpu_gbiprof_report prints the six most frequent causes per 300 frames as `PG0 flush per frame ...` and
`PG0 <function>:<line> <n> per frame, empty <e>` (:374-385), called from nd_cen.c:528. It runs in phase mode only (gp_on, nd_int.h:64:
pm_ph_cen, set by the switch phase=1, pm_phase.c:850).

## Start
`sh 3ds/tools/wt_new.sh PBFLUSH`. PM_ROW=PBFLUSH PM_LANE=PBFLUSH on every build and bench call. Build from the PBATCH2 branch head
4ab13ab4e (merge it into this worktree: `git merge 4ab13ab4e`), so the measured build has both PBATCH2 steps.

## Steps
1. Scenario fxflush_pink (form of kzn04_shadestat, status unverified, needs_picture false): debug
   `warp=mac_01:0 pos=0:0:200 fxtest=124,124,124,124,124,124,124,124 phase=1 loglevel=debug`, until `fxtest: done`, cap 45, deadline 40,
   first_marker `warp: mac_01 entry 0`, expects warp. Bench fxflush_pink (azahar_stream, normal, default false, tags azahar fxcen). `bench.py rebuild`.
   Commit the scenario and the bench together.
2. `PM_ROW=PBFLUSH PM_LANE=PBFLUSH sh 3ds/build.sh > build3ds/out/build.txt 2>&1`; `bench.py run fxflush_pink --row PBFLUSH --why "flush causes
   of the pink_sparkles batches"` (1 start).
3. In the LOG: copy, take the PG0 block whose window covers frames 400-752 (the eight pink_sparkles slots: the block printed after the
   first `fxtest: spawn 124` line), and the same block of the window before it (frames without the effect).

## Gate
rc 0; both PG0 blocks present. A missing PG0 block: stop and hand back (tool fault).

## End
Delete this row file in the completing commit. Hand back the hash and both PG0 blocks copied as they are (no interpretation).
