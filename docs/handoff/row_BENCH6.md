# Row BENCH6: stale build in Azahar runs, and 3 ndmcheck benches

## Goal
1. bench.py records a wrong build and commit for Azahar runs. Every spot_mac01_pic run records "build 2146 commit 43135332b", also for fresh builds where pm_build.h says 2163. 2146 is the last console push. So ingest likely takes the pushed_builds.txt fallback (bench.py lines 14-15, PUSHED, PUSHED_LINE) because the Azahar log has no line that matches BUILD_LINE (bench.py:86, "build N git H").
2. Add 3 scenarios and 3 benches for the peer session (native draw epic S3N, row S3N-3).

## Start state
Branch nds1. You work in the main folder. A peer session (papermario-3ds-port-ae) runs a code agent on nds1 in this folder at the same time.

## Collision rules (hard)
- Edit only: 3ds/tools/bench.py, 3ds/tools/bench_defs.json, 3ds/tools/azt_scenarios.json, this row file, docs/handoff/agent_bench1.md.
- Commit with pathspec only: git commit -m "..." -- <paths>. Never a plain git commit, never git add -A, never stash, reset or checkout.
- No build. No Azahar start. No push. Do not touch build3ds/ outputs.
- Do not append test lines to 3ds/logs/perf/runs.jsonl (append only history).

## Row BENCH6-1: build and commit of an Azahar run
- Find the real source: read the boot lines of a recent Azahar log (grep "build" in the log azt.ps1 reads, see LOG paths in bench.py and 3ds/tools/azt.ps1). Name the line the build prints and its format.
- Fix: for host azahar, take build and commit from the log boot line of that run (fix BUILD_LINE to match the real format), else from the 3dsx that ran (build3ds/pm_build.h or the lane folder for -Build lanes). Never from pushed_builds.txt for azahar. pushed_builds.txt stays the fallback for console hosts only.
- Done: python -c test that calls the ingest parse function on the log of the last spot_mac01_pic run and prints build and commit. Expected: the build number of that log, not 2146. Write the output to 3ds/logs/perf/status_BENCH6.txt. Do not change old runs.jsonl lines.

## Row BENCH6-2: spot_*_ndmcheck scenarios and benches
- Add scenarios spot_mac01_ndmcheck, spot_mac00_ndmcheck, spot_sbk56_ndmcheck to azt_scenarios.json. Each one = a copy of the matching spot_*_pic scenario plus the debug switch ndmcheck in its debug string. until: "ndmcheck f\\d+ recs". cap 25. status "pending" (the build prints the line only after row S3N-3 lands). Gate: an expected_event with the regex of the line and field diff 0 (read docs/handoff/plan_s3n_native_models.md row S3N-3 for the line format, and the max gate form in the azt_scenarios.json note).
- Add 3 benches with the same names to bench_defs.json (copy the spot_*_pic bench, kind azahar_stream or the kind that fits a log gate, cap 25, the until above, gates: diff 0). Scenario and bench in the same commit.
- Done: python 3ds/tools/bench.py list shows the 3 benches. python 3ds/tools/bench.py show spot_mac01_ndmcheck prints the recipe, cap 25 and the until line.

## At the end
Overwrite the STATE block of docs/handoff/agent_bench1.md (10 lines or fewer). Delete this row file in the last commit.
