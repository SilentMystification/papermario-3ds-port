# ws_NC0T1 (Haiku): the bind outputs leave nd_c0 (tile_x0, tile_y0, tile_fx, tile_fy, cur_tile) into the bind module
Designer: oracle NATIVE17 (plan_perf.md section 7, Q4 default: bind outputs are a cache of the bind module). WAIT NC0V: branch from main after the
NC0V merge (shared lines nd_comb.c:195, :325-329, nd_geom.c:115-121). Parallel with NC0M (other lines). NC0F waits for this merge.
Worktree build3ds/wt/NC0T1, branch NC0T1. PM_ROW=NC0T1 PM_LANE=NC0T1. Lines: main 8bcf0252ee; NC0V moves some lines: grep the quoted text.
Why Haiku: a pure move with one sed line. Every read and write of the 5 fields moves to one new object (start values 0, as nd_c0),
so the result is byte-equal by construction. No census.
Objective: delete the 5 fields from NdC0. Counter: nd_c0 -20 B (GUESS, 5 words). c0gate 0 / 0 / 1.
## Facts (FACT grep `nd_c0\.(tile_x0|tile_y0|tile_fx|tile_fy|cur_tile)`, main 8bcf0252ee)
- Writers: the bind (nd_tex_set.c:121-155, :296-462), nd_direct.c:96-99, :132, nd_rect.c:158, nd_comb.c:249. They are outputs of nd_tile_bind and its arms.
- Readers: uv origin math nd_comb.c:195-196 (gv key), :275-276, :327-328, nd_direct.c:117-118, :176-177, nd_geom.c:117, nd_rect.c:190-194, :260-261;
  cur_tile tests in nd_tex_set.c:425, :448.
- Host: 3ds/tools/nd_direct_test.c includes nd_direct.c and nd_int.h (:8); it writes the fields at :195 and reads cur_tile at :201.
## Edits
1. nd_int.h after line 214 `extern PmPlan cur_plan; extern float dual_f[4];` add:
   `typedef struct NdBo { int x0, y0; float fx, fy; int cur; } NdBo; /* NC0T1: the bind outputs: uv origin and fraction of the bound tile, the bound tile (bind cache) */`
   `extern NdBo nd_bo;`
2. nd_tex_set.c after line 43 (`struct TszLast tsz_last; ...`): `ND_HOTD NdBo nd_bo; /* NC0T1: owner nd_tile_bind */`.
3. One sed (CRLF safe), from the repo root:
   `sed -b -i -e 's/nd_c0\.tile_x0/nd_bo.x0/g' -e 's/nd_c0\.tile_y0/nd_bo.y0/g' -e 's/nd_c0\.tile_fx/nd_bo.fx/g' -e 's/nd_c0\.tile_fy/nd_bo.fy/g' -e 's/nd_c0\.cur_tile/nd_bo.cur/g' 3ds/src/nd_comb.c 3ds/src/nd_direct.c 3ds/src/nd_geom.c 3ds/src/nd_rect.c 3ds/src/nd_tex_set.c 3ds/tools/nd_direct_test.c`
   Example, nd_geom.c:117 before `in.tile_x0 = nd_c0.tile_x0; in.tile_y0 = nd_c0.tile_y0; ...` after `in.tile_x0 = nd_bo.x0; in.tile_y0 = nd_bo.y0; ...`.
4. nd_direct_test.c after line 38 (the pm_log_evt stub): `NdBo nd_bo; /* NC0T1 */`.
5. nd.h NdC0: delete `int tile_x0, tile_y0;`, `float tile_fx, tile_fy;`, `int cur_tile;`. Keep NdWs (its own tile fields) unchanged.
6. Check: `grep -rnE "nd_c0\.(tile_x0|tile_y0|tile_fx|tile_fy|cur_tile)" 3ds/src 3ds/tools 3ds/tests` prints nothing.
## Checklist (hw_guidelines.md 2): nd_bo is 20 B in .data.pmhot (ND_HOTD), next to the other bind state. PICA: no change.
## Tests (build3ds/out/, last 20 lines). Gate: 1 build, 2 Azahar runs. Base: `bench.py base <b> --commit $(git merge-base HEAD main)`.
1. Host `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=NC0T1 sh 3ds/build.sh`; c0gate 0 / 0 / 1; note sizeof(NdC0) base and tip.
2. `bench.py run spot_mac01_drec,hudfb_drec --row NC0T1 --why "NC0T1 gate"`; `bench.py cmp <b> --row NC0T1` byte-equal for both.
Fix rights: compile errors (a file without nd_int.h gets the include), a site that step 6 shows. A DIFF or a crash: stop, send the path to oracle NATIVE17.
Stop: last commit `NC0T1 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hash, host, c0gate, nd_c0 size base and tip, the 2 cmp lines.
