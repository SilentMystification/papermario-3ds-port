# Row HC0: hot code measurement tools (plan_hotcode.md stage HC0)

## User facts
User (2026-11-09): "Come up with a plan to evaluate the whole draw pipeline to reduce cache misses. We need to shrink the hot code by
a very significant amount." Console O3DS XL, mac_01: 40.8k I-miss and 22.4k D-miss per frame on core 0 (4.49M cycles).

## Start state
Worktree build3ds/wt/HC0, branch HC0 from main. PM_ROW=HC0, PM_LANE=HC0. One Haiku agent. Tools only: no game behaviour change.
Read docs/handoff/plan_hotcode.md sections 0, 2 (HC0) and 3 first.

## Steps
1. 3ds/tools/hotset.py + 3ds/tools/hotset.json (class lists A to I from plan_hotcode section 0). Inputs: the build map, plus nm and
   objdump of the ELF through the devkitarm docker image (see how build.sh runs tools). Prints: M1 bytes per class and the union; M2
   the bytes of the linker hot group in 3ds/ld/3dsx.ld.in and every name in it that is missing from the map (stale-name warning); per
   hot function, its calls into log, debug, timer, division and double helpers. hotspan.py is deleted (hotset.py replaces it).
2. build.sh prints one hotset line after the memcheck line. Bench hotset_size (tool kind, no Azahar) in bench_defs.json; it records
   M1 and M2 into runs.jsonl.
3. fncov debug tool: with PM_FNCOV=1 build.sh compiles the draw TUs (3ds/src/nd_*.c, pm_gpu*.c, pm_tex.c, pm_comb.c) with
   -finstrument-functions into a census lane; __cyg_profile_func_enter (no_instrument_function) counts calls per function address in
   a static table; switch fncov=<frame> prints the functions that ran in that frame with call counts, then "fncov f<n> done".
   hotset.py --fncov <log> joins it with the sizes into M3 per class.
4. Scenarios + benches (same commit): fncov_mac01_det (warp=mac_01:0 pos=0:0:200 det), fncov_kmr10_det, fncov_mac00_det; cap about
   15 s; until "fncov f<n> done".
5. 3ds/tools/nd_size_test.sh calls the missing nd_size_check.py: delete it (hotset.py replaces gate G5) and remove its callers.
6. Check that the console bench spot_mac01_prof prints the pmu line; if prof=30 does not turn on pmu, add the switch to the recipe.

## Gates
- The normal build (PM_FNCOV unset) is byte-identical in .text to main: compare the ELF .text with a build-at of the base.
- bench hotset_size prints M1 (expect about 124 KB union) and M2 (expect the stale names nd_walk_run, pm_gbi_run).
- fncov_mac01_det prints the fncov list and "fncov f<n> done"; hotset.py gives M3 per class. Report M1, M2, M3.
- memcheck OK. No console run in this row (the user runs the baseline).
