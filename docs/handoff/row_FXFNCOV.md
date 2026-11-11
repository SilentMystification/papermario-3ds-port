# Row FXFNCOV: which draw functions each walk-bound effect runs per frame (measurement, no code edit)

(Model: Haiku. Data only: the scenarios and benches in one commit. Call counts are valid on Azahar (deterministic counts, not time).)

## Why (oracle, XL run xl4a: build3ds/out/fxcen_xl4a.txt)
Core 0 walk above base per frame: snowfall 18.1 ms (117 parts), water_splash 12.9 (53 draws, 52 binds), energy_shockwave 11.2,
pink_sparkles 11.1 (47 parts), water_block 10.8, embers 9.2, shimmer_burst 8.7, effect_3D 8.4; GPU stays under 0.8 ms for most.
That is 150-700 us of core 0 per part, far above the code-read estimate (5-20 us), and even a 4-triangle effect (ring_blast) costs
2 ms. A code read cannot say which class (A op loop, B combine sync and plan, E batch close and TEV, F vertex and matrix, D submit) runs
per part. PBFLUSH (run 504363) showed pink_sparkles' flushes come from pm_gpu_set_plan (53.8 per frame, base 11.4) and
pm_gpu_set_prim (30.2): a plan per part. The fncov census lane (plan_hotcode HC0, 3ds/src/pm_fncov.c) gives the calls of each draw
function in one walker frame; hotset.py --fncov gives M3 bytes and calls per class.

## Start
`sh 3ds/tools/wt_new.sh FXFNCOV`. PM_ROW=FXFNCOV PM_LANE=FXFNCOV on every build and bench call.

## Touch only
3ds/tools/azt_scenarios.json (8 new scenarios), 3ds/tools/bench_defs.json (8 new benches). No source file.

## Exact edits
1. Scenarios, each a copy of fncov_mac01_det with these fields changed (name, subject, proves, debug, reason):
   fxfncov_base: debug `warp=mac_01:0 pos=0:0:200 fxtest=63 fncov=396 loglevel=debug det` (frame 396: before the spawn at 402).
   fxfncov_<i> for i = 63 snowfall, 89 water_splash, 51 energy_shockwave, 124 pink_sparkles, 105 embers, 50 shimmer_burst,
   55 effect_3D: debug `warp=mac_01:0 pos=0:0:200 fxtest=<i> fncov=420 loglevel=debug det` (frame 420: the effect is live, slot
   400-443, pm_fxtest.c:253-275). Cap 45, deadline 40, until `fncov f\d+ done`, first_marker `warp: mac_01 entry 0`.
2. Benches fxfncov_base and fxfncov_<i>: copies of the fncov_mac01_det bench entry (azahar_stream, normal, default false, tags
   azahar hotcode fncov), scenario of the same name, cap 45. Then `python 3ds/tools/bench.py rebuild`. Commit both files together.

## Gate (measurement; CLAUDE.md 3)
- `PM_FNCOV=1 PM_ROW=FXFNCOV PM_LANE=FXFNCOV sh 3ds/build.sh > build3ds/out/build.txt 2>&1` (the census lane lane-FXFNCOV_fncov).
- One list, 8 Azahar starts (the budget): `python 3ds/tools/bench.py run fxfncov_base,fxfncov_63,fxfncov_89,fxfncov_51,
  fxfncov_124,fxfncov_105,fxfncov_50,fxfncov_55 --row FXFNCOV --lane FXFNCOV_fncov --why "per-effect draw function calls in one
  walker frame: which class costs per part"`.
- For each log: `python 3ds/tools/hotset.py --fncov build3ds/azt/logs/<scenario>.txt --lane FXFNCOV > build3ds/out/<scenario>.txt`.
  Pass: all 8 files have the M3 lines (no "did not finish" line). Hand back the 8 file paths and the M3 class lines of each, as printed.

## Stop
A missing done line, a run that fails, or two lines that disagree with the bench: stop and hand back the exact output (CLAUDE.md 4).

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash and the 8 file paths.
