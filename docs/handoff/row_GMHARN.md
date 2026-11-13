# Row GMHARN: gold.py runs the coverage of a GOLD row and gives the 90 % verdict (plan GMHARN and GMCOV in one tool)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main cff324713)
- COVMAP gives covsites.py `<fncov log> --lane <L> <src file>...` with lines `cov <file> sites <h>/<t> funcs ...` and
  `cov miss <file> <func> sites <k>`. COVMAP2 gives the frame window.
- 3ds/logs/perf/runs.jsonl has one JSON line per run with keys bench, row, ts and log_path. log_path is the slot log
  (build3ds/lane-<row>/slot<n>/last_log.txt), and the next run on that slot overwrites it.
- A bench may name an existing scenario (field "scenario"). bench.d/<row>.json adds benches without editing a registry.
Blocked by COVMAP2. Start: `sh 3ds/tools/wt_new.sh GMHARN`. PM_ROW=GMHARN PM_LANE=GMHARN on every call.
## Touch only (places)
New 3ds/tools/gold.py; 3ds/tools/covsites.py (argument parsing only: several logs); new cov.d/GMHARN.txt, bench.d/GMHARN.json.
## Exact edits
1. covsites.py: accept one or more logs before `--lane`. The calls per address are the sum over all logs.
2. gold.py `<row> [--no-build]`: it reads cov.d/<row>.txt (the files) and bench.d/<row>.json. The coverage benches have the tag
   fncov. Without --no-build it runs `PM_FNCOV=1 PM_LANE=<row> sh 3ds/build.sh` (exit not 0: print and exit 1). For each
   coverage bench it runs `python 3ds/tools/bench.py run <b> --row <row> --lane <row>_fncov --why "gold coverage"`. It takes the
   last runs.jsonl line with that bench and row, and copies its log_path to build3ds/out/<row>_<b>.log. It runs covsites.py on
   all copies, sums h and t over the files, and prints `gold <row> sites <H>/<T> pass` when `10 * H >= 9 * T`, else
   `gold <row> sites <H>/<T> fail` and the cov miss lines. Exit 0 on pass, 1 on fail. It does not run bench.py base.
3. cov.d/GMHARN.txt: `src/effects/water_splash.c`. bench.d/GMHARN.json: gm_fncov_89, scenario fxfncov_89, kind azahar_stream,
   tags azahar det fncov (no new scenario).
4. Commit: `GMHARN: gold.py coverage harness with the 90 percent verdict, covsites.py takes several logs`.
## Gate (expected values)
1. `python 3ds/tools/gold.py GMHARN`: `gold GMHARN sites 11/11 pass`, exit 0.
2. Seed first (a value change, then undo): `10 * H >= 11 * T`. `gold.py GMHARN --no-build` prints `fail`, exit 1.
3. Budget 8 Azahar starts. Counter: none moves (phase G harness; the GOLD rows use it).
## Stop and end
A verdict other than the gate, or a tool fault: stop, commit, hand back the exact output. Delete this row file at the end.
