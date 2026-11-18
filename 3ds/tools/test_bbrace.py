"""BBRACE: bench.base_check_cold treats a basebin entry removed by a parallel run as skipped, not as a cache fault.

Run: python 3ds/tools/test_bbrace.py    (stdlib only). Prints "bbrace cases 3 bad <b>"; exit 1 when b > 0.
Each case monkeypatches the entry get, the lane paths, the lock, the scratch build and the lane drop.
Case 1: the entry file is removed during the scratch build (after bb.get returned it). Must print the skip line, return 0.
Case 2: the entry matches the scratch build. Must return 0 with the ok line.
Case 3: the entry differs from the scratch build. Must return 1 with the FAIL line.
"""
import contextlib
import io
import os
import sys
import tempfile
import types
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import bench  # noqa: E402


def run_case(tmp, build_fn):
    """Runs base_check_cold on one fake entry. build_fn(ent, src3dsx) plays the scratch build. Returns (rc, stdout)."""
    ent = tmp / "ent"
    ent.mkdir(parents=True, exist_ok=True)
    (ent / "pm_3ds.3dsx").write_bytes(b"entry bytes\n")
    src3dsx = tmp / "src.3dsx"
    src3dsx.write_bytes(b"entry bytes\n")
    lane_dir = tmp / "lane"
    lane_dir.mkdir(exist_ok=True)
    scratch = tmp / "scratch"
    scratch.mkdir(exist_ok=True)

    head = os.popen("git -C \"%s\" rev-parse HEAD" % bench.REPO).read().strip()
    saved = (bench.bb.get, bench.base_lane_paths, bench.base_lock_take, bench.scratch_build, bench.drop_base_lane,
             bench.basebin_key)
    lock = tmp / "x.lock"
    lock.write_text("lock")

    def fake_scratch(commit, gitdir, sc, lane, row):
        build_fn(ent, src3dsx)
        return 0, None

    bench.bb.get = lambda root, key: ent
    bench.base_lane_paths = lambda commit: ("base_x", scratch, lane_dir, tmp, src3dsx)
    bench.base_lock_take = lambda commit: lock
    bench.scratch_build = fake_scratch
    bench.drop_base_lane = lambda lane, bds: None
    bench.basebin_key = lambda commit: "key"
    a = types.SimpleNamespace(commit=None, row="TEST")
    rnd = types.SimpleNamespace(choice=lambda seq: seq[0])
    buf = io.StringIO()
    try:
        with contextlib.redirect_stdout(buf):
            rc = bench.base_check_cold(a, tmp, [head], rnd)
    finally:
        (bench.bb.get, bench.base_lane_paths, bench.base_lock_take, bench.scratch_build, bench.drop_base_lane,
         bench.basebin_key) = saved
    return rc, buf.getvalue()


def main():
    bad = 0
    cases = 0
    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)

        # Case 1: the entry vanishes during the scratch build.
        cases += 1
        sub = tmp / "c1"
        sub.mkdir()

        def remove_entry(ent, src3dsx):
            (ent / "pm_3ds.3dsx").unlink()

        rc, out = run_case(sub, remove_entry)
        if rc != 0 or "entry removed by a parallel run, skipped" not in out:
            bad += 1
            print("case 1 bad: rc %r out %r" % (rc, out))

        # Case 2: the entry matches the scratch build.
        cases += 1
        sub = tmp / "c2"
        sub.mkdir()
        rc, out = run_case(sub, lambda ent, src3dsx: None)
        if rc != 0 or "has the same 3dsx bytes" not in out:
            bad += 1
            print("case 2 bad: rc %r out %r" % (rc, out))

        # Case 3: the entry differs from the scratch build: a real fault, rc 1.
        cases += 1
        sub = tmp / "c3"
        sub.mkdir()

        def differ(ent, src3dsx):
            src3dsx.write_bytes(b"other bytes\n")

        rc, out = run_case(sub, differ)
        if rc != 1 or "CACHECHECK FAIL basebin" not in out:
            bad += 1
            print("case 3 bad: rc %r out %r" % (rc, out))
    print("bbrace cases %d bad %d" % (cases, bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
