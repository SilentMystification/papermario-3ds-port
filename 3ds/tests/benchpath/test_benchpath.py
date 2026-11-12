# BENCHPATH host test: append_runs skips a runs.jsonl that vanished (a worktree removed by wt_gc). Run from the repo root (run.sh does that).
import sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent.parent
sys.path.insert(0, str(ROOT / "3ds" / "tools"))
import bench  # bench.py guards main, so the import runs nothing

with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    bench.PERF = tmp  # the temp copy: the real runs.jsonl and bench.db are not touched
    bench.RUNS = tmp / "runs.jsonl"
    bench.DB = tmp / "bench.db"
    bench.all_tree_runs_files = lambda: [tmp / "gone" / "runs.jsonl"]  # listed, then removed: does not exist
    ids, _info = bench.append_runs([{"bench": "t", "run_id": 0}])
    lines = [x for x in bench.RUNS.read_text(encoding="utf-8").splitlines() if x.strip()]
    ok = ids == [1] and len(lines) == 1
    print("benchpath: %s" % ("PASS" if ok else "FAIL ids=%r lines=%d" % (ids, len(lines))))
    sys.exit(0 if ok else 1)
