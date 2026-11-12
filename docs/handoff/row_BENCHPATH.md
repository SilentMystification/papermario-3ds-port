# Row BENCHPATH: bench.py run-id scan must survive a worktree that is removed during the run

(Model: Haiku. Tool row: one function in bench.py plus a host test.)

## Fact (S4PDIR, 2026-11-12)
The fxfncov_63 run printed `BENCH: run not recorded`: `FileNotFoundError ... build3ds\wt\S1FILL\3ds\logs\perf\runs.jsonl` in
append_runs (bench.py:309), reached through ingest-last. S1FILL was merged and its worktree removed (wt_gc) while the run went on.

## Cause (code read)
append_runs (bench.py:302-312) takes the next run id from every tree's runs.jsonl: `for f in all_tree_runs_files(): ...
f.read_text(...)`. all_tree_runs_files (:1025-1040) lists the trees with `git worktree list --porcelain` and keeps the files that
exist at listing time (`f.exists()`). A worktree that wt_gc deletes after the listing, or a registered worktree whose folder is
gone, makes read_text raise. Nothing caches another row's path: the path comes from git's worktree list.

## Start
`sh 3ds/tools/wt_new.sh BENCHPATH`; PM_ROW=BENCHPATH. No running row edits bench.py.

## Exact edits (3ds/tools/bench.py)
1. append_runs: replace the read in the loop with
   `try: txt = f.read_text(encoding="utf-8")` / `except OSError: continue  # BENCHPATH: a tree removed during the run (wt_gc)` /
   then `n = max([n] + [json.loads(x).get("run_id", 0) for x in txt.splitlines() if x.strip()])`.
2. grep `all_tree_runs_files()` for its other callers. Give each read the same try/except OSError skip.

## Test (new) 3ds/tests/benchpath/run.sh and test_benchpath.py
- `sys.path.insert(0, "3ds/tools"); import bench`.
- Point bench.RUNS and bench.PERF at a temp folder (both are module globals) and monkeypatch
  `bench.all_tree_runs_files = lambda: [Path(tmp) / "gone" / "runs.jsonl"]` (a file that does not exist).
- Call bench.append_runs with one minimal record `{"bench": "t", "run_id": 0}`. Use the record fields append_runs needs: read
  its body and give the fields it reads.
- Assert no exception and one line in the temp runs file. Print `benchpath: PASS`.
- run.sh: `cd "$(dirname "$0")/../../.." && python 3ds/tests/benchpath/test_benchpath.py`.

## Gate
- `sh 3ds/tests/benchpath/run.sh`: `benchpath: PASS`.
- `python 3ds/tools/bench.py run nd_host --row BENCHPATH --why "BENCHPATH"`: PASS, and recorded (the normal path still works).
- No Azahar start.

## Stop
A failed gate: stop, commit, hand back. Delete this row file in the completing commit.
