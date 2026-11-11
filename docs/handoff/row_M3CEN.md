# Row M3CEN: M3 census of main (bytes run per frame and class) for the next class cuts

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names. A measurement row: no source edit.)
## Facts
- M1-hot (hotset_size, main after the night merges) per class: A 9.1, B 7.1, C 10.0, D 11.1, E 23.4, F 10.8, G 14.8, H 12.7, I 10.5 KB;
  plan_hotcode.md section 2 targets: one mac_01 frame <= 16 KB, a class <= 8 KB. M1 counts every function the call graph reaches;
  the cuts so far (HCCUT1, HCCUT2, PERF2) took the rare arms already: the next cut must come from the functions that RUN per frame
  (M3: hotset.py --fncov on a census log, plan_hotcode section 3). The last census (HC8, run 253145/253967) predates HC4e4, HC6, KZN04.
- Census benches: fncov_mac01_det, fncov_kmr10_det, fncov_mac00_det (azahar_stream, lane <L>_fncov, build.sh PM_FNCOV=1). A census
  bench has no base and is never a byte gate (do not run `bench.py base fncov_*`).
## Start
`sh 3ds/tools/wt_new.sh M3CEN` from main; PM_ROW=M3CEN PM_LANE=M3CEN on every call. No file of a running row is touched.
## Steps
1. `PM_ROW=M3CEN PM_LANE=M3CEN PM_FNCOV=1 sh 3ds/build.sh > build3ds/out/build_fncov.txt 2>&1`: rc 0.
2. One list: `PM_ROW=M3CEN python 3ds/tools/bench.py run fncov_mac01_det,fncov_kmr10_det,fncov_mac00_det --row M3CEN --lane M3CEN_fncov --why "M3 per class after HC6/KZN04/HCCUT2 for the next class cuts"`.
   If azt_scenarios.json gets an "unverified" mark from the run: `git checkout 3ds/tools/azt_scenarios.json`.
3. Copy the three LOG files the runs print to build3ds/out/, then for each:
   `PM_LANE=M3CEN python 3ds/tools/hotset.py --fncov build3ds/out/<log> > build3ds/out/m3_<scene>.txt`.
4. Commit only runs.jsonl (`git add -f 3ds/logs/perf/runs.jsonl`). The commit message lists per scene: M3 per class (bytes) and the 5
   largest run functions of classes E, G, H and D with bytes and calls per frame (copied from the m3 files).
## Gate
Step 1 rc 0; each run reaches its until-line (bench.py check: PASS); hotset.py --fncov rc 0 for the three logs.
## Stop and end
A tool fault (a run that ends rc 2, a missing until-line, a stale build): commit and hand back the exact output (CLAUDE.md 4).
Delete this row file in the completing commit. Hand back: the commit hash and the per-class M3 table of the three scenes.
