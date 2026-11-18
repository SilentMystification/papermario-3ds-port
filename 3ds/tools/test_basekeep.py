"""BASELOG: bench.py base keeps the run log of each run of a differing pair beside its shot copy.

Run: python 3ds/tools/test_basekeep.py    (stdlib only). Prints "basekeep cases 3 bad <b>"; exit 1 when b > 0.
Fault: BASEKEEP_FAULT=1 makes case 1 call snap_shots without the log, so case 1 fails.
"""
import os
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import bench  # noqa: E402


def main():
    fault = os.environ.get("BASEKEEP_FAULT") == "1"
    bad = 0
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        os.environ["PM_BASE_SNAP"] = str(tmp / "snap")

        shots = tmp / "shots"
        shots.mkdir()
        (shots / "drec.txt").write_bytes(b"drec one\n")
        run = tmp / "run.txt"
        run.write_bytes(b"run log one\n")

        # Case 1: the run log lands beside the shot copy with the same bytes.
        d = bench.snap_shots(str(shots), "sc", None if fault else str(run))
        lg = Path(str(d) + ".log")
        ok1 = d is not None and lg.is_file() and lg.read_bytes() == run.read_bytes()
        if not ok1:
            bad += 1
            print("case 1 bad: log beside shot copy %s" % lg)

        # Case 2: a second run gives a second copy; keep_logs names both logs a.log and b.log.
        run2 = tmp / "run2.txt"
        run2.write_bytes(b"run log two\n")
        da = bench.snap_shots(str(shots), "sc", str(run))
        db = bench.snap_shots(str(shots), "sc", str(run2))
        keep = tmp / "keep"
        keep.mkdir()
        got = bench.keep_logs(str(keep), {"shot_path": da}, {"shot_path": db})
        ok2 = got == ["a.log", "b.log"] and (keep / "a.log").is_file() and (keep / "b.log").is_file()
        if not ok2:
            bad += 1
            print("case 2 bad: keep_logs returned %s" % got)

        # Case 3: the .log file is not a compared shot name.
        ok3 = not any(n.endswith(".log") for n in bench.shot_names(d))
        if not ok3:
            bad += 1
            print("case 3 bad: shot_names %s" % bench.shot_names(d))

        os.environ.pop("PM_BASE_SNAP", None)
    print("basekeep cases 3 bad %d" % bad)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
