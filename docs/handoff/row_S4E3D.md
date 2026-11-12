# Row S4E3D: effect_3D on the native particle path (NATIVEAPI S4)

(Model: Haiku: the S4SHIM/S4EMB pattern, one loop, no design choice.)

## State (try 1, code 0d77cb9b8 on branch S4E3D)
Code done, memcheck OK, nd_host PASS, fxpic_55 6/6 byte-equal against the merge-base. Only the census (speed gate) is open.
Spec fault (oracle 3): "effect minus base" in try 1 meant the scene fxfncov_base, not `bench.py base`. `bench.py base fxfncov_base
--commit d048e654d` builds that commit WITHOUT PM_FNCOV (the base lane is the committed tree, no census instrumentation), so the
game never prints `fncov f<n> done` and azt.ps1 stamps the scenario "broken" (azt.ps1:756-758). That is not a tool fault and not a
fault of the scenario: main ran fxfncov_base 7 times in census lanes.

## Fix the worktree first (no new commit for this)
1. In build3ds/wt/S4E3D: `git status --short`. The only uncommitted change must be 3ds/tools/azt_scenarios.json (the
   AZT11-5 stamp: a new "reason" text "broken: no until in 45 s ..." on fxfncov_base). Revert it:
   `git checkout -- 3ds/tools/azt_scenarios.json`. Your fxpic_55 scenario is in commit 0d77cb9b8 and stays.
   Any other uncommitted file: stop and hand back `git status --short`.
2. Never run `bench.py base` or `bench.py cmp` on an fxfncov_* bench. They have no stored base and need the census build.
3. Base runs do not count against the budget (run_azahar.ps1:79): the three failed base runs cost nothing. Budget left: 8 minus
   your fxpic_55 row runs. The census needs 2 starts.

## Census (the speed gate): both scenes on YOUR census build, one list, one slot
- Build: `PM_FNCOV=1 PM_ROW=S4E3D PM_LANE=S4E3D sh 3ds/build.sh > build3ds/out/build_fncov.txt 2>&1`. With PM_FNCOV=1 the build
  writes the lane `S4E3D_fncov` (build.sh:20-23), not lane S4E3D. Check that build3ds/lane-S4E3D_fncov/pm_3ds.3dsx has a new time.
- Run: `PM_ROW=S4E3D PM_LANE=S4E3D_fncov python 3ds/tools/bench.py run fxfncov_base,fxfncov_55 --row S4E3D --lane S4E3D_fncov
  --why "S4 speed gate: draw function calls of effect_3D minus the no-effect frame, census lane"`. This is 2 starts.
  bench.py adds -Try itself (fxfncov_* have status unverified on main; bench.py:993).
- Expected: each run prints `fncov f396 done` (base) and `fncov f420 done` (effect 55) within 45 s. A run without it: tool fault
  (CLAUDE.md 4), stop and hand back the LIST lines and the last 20 lines of each LOG.
- Count: `python 3ds/tools/hotset.py --fncov build3ds/azt/logs/fxfncov_base.txt --fncovfn --lane S4E3D_fncov > build3ds/out/fn_base.txt`
  and the same for fxfncov_55.txt into build3ds/out/fn_55.txt. Delta = calls in fn_55 minus calls in fn_base, per function.
- Pass: nd_part_draw > 0; gv_tri, draw_tri, nd_geom_vtx, nd_ops_run and ent_gdl each < 20 (effect minus base).
- After the run: `git status --short` again. A new azt_scenarios.json stamp means a run hit the cap: revert it as in step 1, stop,
  hand back.

## Stop
A failed gate or a tool fault: no new commit (the code is in 0d77cb9b8), keep this row file, hand back the output. Note that
--fncovfn lists only functions with 20 or more calls: a function absent from a file counts as below 20 there. At a passed gate:
delete this row file in one commit (`git rm docs/handoff/row_S4E3D.md`), hand back that hash, the 15 largest deltas, the memcheck
line of the normal build (try 1) and the fxpic_55 result (6/6).
