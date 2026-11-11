# Row HCM3: current hot-code census (M1-hot per class, M3 functions run per frame with call counts) for the next HC cuts

(Model: Haiku. Base: main. No source edit: tool and census runs only. Disjoint from every row. Reads this file and CLAUDE.md.)

## Why (console PMU, XL, main after PMUPHFIX)
Core 0: CPI 4.69, istall 41.4 % of cycles, 47,308 I-misses per clean frame; the gbi phase (the walk and draw core) holds 83.2 % of the
I-misses, dl (the list build) 9.7 %. plan_hotcode.md M1-hot is 70.1 KB (HANDOFF) against a 16 KB I-cache. The next HC cuts need the
functions that run per frame now, by class and call count (M3), on the current main (HCCUT1-5, HC6B, HC9B/C, HC10 changed them).

## Start
`sh 3ds/tools/wt_new.sh HCM3`. PM_ROW=HCM3 PM_LANE=HCM3 on every build and bench call.

## Steps
1. `PM_ROW=HCM3 PM_LANE=HCM3 sh 3ds/build.sh > build3ds/out/build.txt 2>&1` and the census lane `PM_FNCOV=1 PM_ROW=HCM3 PM_LANE=HCM3 sh 3ds/build.sh > build3ds/out/build_fncov.txt 2>&1`
   (hotset.py header: the census lane is <L>_fncov).
2. `python 3ds/tools/bench.py run hotset_size --row HCM3 --why "M1-hot per class on main"`: PASS; copy its per-class lines.
3. One list `bench.py run fncov_mac01_det,fncov_kmr10_det --row HCM3 --why "M3 per class and calls per frame" --lane HCM3_fncov`
   (2 starts).
4. `python 3ds/tools/hotset.py --fncov <each LOG: copy> > build3ds/out/hcm3_<scene>.txt`: per class the bytes that ran and, per function,
   bytes and calls per frame.
5. Commit only the runs.jsonl lines.

## Gate
hotset_size PASS; both fncov runs rc 0 with their `fncov f<n> done` line; both hcm3 files written. A tool fault: stop and hand back.

## End
Delete this row file in the completing commit. Hand back: the hash, the hotset per-class lines, and from each hcm3 file the per-class
totals and the 15 largest functions by bytes x (calls > 0), copied as they are.
