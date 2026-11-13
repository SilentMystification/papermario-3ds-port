# Row BASEEVID: one base run of spot_mac01_det and spot_mac01_fps60_det on main, to record which shots of a pair differ

(Model: Haiku. 40 lines or fewer. Read only this file, CLAUDE.md and what it names. No code edit in this row.)
## Facts (ORACLE8, main a3ffbbe8b)
- Two base pairs ended "two runs differ, not stored (0000_1_2.ppm differs)": spot_mac01_det at 86f530af6 and
  spot_mac01_fps60_det at 3c7d977ab. No producer was found by code read (row_BASEKEEP.md Facts).
- BASEKEEP (merged a3ffbbe8b): a differing pair now prints every differing shot with its pixel count and bounding box and keeps
  both shot sets at build3ds/basesnap_fail/<lane>/<time>/a and b. No PNG.
Start: `sh 3ds/tools/wt_new.sh BASEEVID` (branch from main a3ffbbe8b). PM_ROW=BASEEVID PM_LANE=BASEEVID on every call.
## Touch only (places)
No source file. Only the runs and the runs.jsonl lines they write.
## Steps (exact)
1. `PM_ROW=BASEEVID PM_LANE=BASEEVID python 3ds/tools/bench.py base spot_mac01_det --commit a3ffbbe8b --row BASEEVID
   > build3ds/out/base_det.txt 2>&1`
2. `PM_ROW=BASEEVID PM_LANE=BASEEVID python 3ds/tools/bench.py base spot_mac01_fps60_det --commit a3ffbbe8b --row BASEEVID
   > build3ds/out/base_fps60.txt 2>&1`
3. Do not open, view or describe any shot or kept file (CLAUDE.md 2). Do not run any other bench.
## Gate (expected values)
- Each command ends with one of: `base <bench> at a3ffbbe8b: two runs equal, stored ...` or `base <bench>: two runs differ, not
  stored, <k> shots: <name px <n> bbox x0..x1 y0..y1; ...>, kept at <path>`. Both are results: this row has no pass or fail.
- A different end (a build or run error, a tool fault): stop and hand back the exact output (CLAUDE.md 4).
- Budget: 3 Azahar starts per command is the tool's own limit (BASE_MAX_RUNS 4 runs at most: hand back the starts it used).
## Hand back (exactly, no picture words)
For each bench: the last line of its output file, the run ids, the build number, the kept path if any. Nothing else.
## Stop and end
After step 2, or at a tool fault. Commit only the runs.jsonl lines (`git add -f 3ds/logs/perf/runs.jsonl`). Delete this row file
in the completing commit.
