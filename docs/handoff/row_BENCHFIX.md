# Row BENCHFIX: bench and prof tool faults found on 2026-11-09

## User facts
User (2026-11-09) asked how cache misses are now; the pmu and core2 prof lines are not in the run history, so no build can be
compared with an older one. Agents found bench faults during the 2026-11-09 rows.

## Start state
Worktree from `sh 3ds/tools/wt_new.sh BENCHFIX`, branch BENCHFIX. PM_ROW=BENCHFIX, PM_LANE=BENCHFIX. One Haiku agent.

## Faults (each found in a run; fix each at its cause)
1. Shot cut: Azahar stops 4 to 13 s after the until-line while the last shot is still being written; azt prints "SHOT COPY FAILED"
   but the bench reports PASS (5 runs in HUDTRAIL, also nok11_det in WATER). Fix: azt.ps1 waits for the shot it counts last (file
   size stable) before it stops Azahar, and a SHOT COPY FAILED on a shot the bench gates makes the run FAIL.
2. `bench.py run` without --lane inside a worktree uses the main build3ds/pm_3ds.3dsx and writes a bad record (exit 5). Fix: in a
   worktree, the lane defaults to PM_LANE or the worktree branch name; a run with no lane build stops before Azahar with one line.
3. The console prof lines `pmu m <I> <D> cyc <c>` and `core2 lgms .. ic1 .. dc1 .. cyc1 ..` are not ingested. Fix: the ingest
   (bench.py / perf DB) stores pmu_i, pmu_d, pmu_cyc, ic1, dc1, cyc1, lgms per window with the scene, so `bench.py compare` and
   `trend` show them per build.
4. The prof `nd ... gb <ms>` value is wrong in some windows (8864753.66 and 39953391.90 in kmr_10, XL log of 92e013100). Find
   where gb is measured in 3ds/src/pm_phase.c (GPU time), name why it wraps or reads without a start, fix it.

## Gates
- A det bench with a shot at the last frame: no SHOT COPY FAILED in 3 runs; a forced cut (kill Azahar early once, by hand) makes
  the run FAIL.
- `bench.py run nd_host` from the worktree with no --lane uses the BENCHFIX lane.
- Ingest build3ds/hw_live_log_131.txt (the 92e013100 XL log): runs.jsonl gets pmu and core2 fields; `bench.py trend` prints them.
- gb fix: a code trace names file, line and value; nd_host PASS; memcheck OK. No console run in this row.
