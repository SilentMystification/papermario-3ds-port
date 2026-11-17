#!/usr/bin/env python3
"""BASEKEEP2 self-test of bench.py base_check_shots. Case 1: an entry stored for other inputs (a bench of a row branch) is not a
candidate and stays. Case 2: a valid entry whose check rerun stores nothing goes back with its index line. Seeded faults:
CHECK_PURE False removes the case 1 entry, RESTORE_KEPT False loses the case 2 entry. Prints "basecheck ok"."""
import argparse
import json
import random
import shutil
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
import bench as B  # noqa: E402


def fail(msg):
    print("basecheck FAIL " + msg)
    sys.exit(1)


def entry(root, commit, bench, key):
    d = root / commit / bench
    d.mkdir(parents=True)
    (d / "drec.txt").write_text("draw 1\n", encoding="utf-8")
    meta = {"key": key, "out_sha1": {"drec.txt": B.bb.sha1_file(d / "drec.txt")}}
    (d / "meta.json").write_text(json.dumps(meta), encoding="utf-8")
    return d


def run(pure, restore):
    """(other entry exists, valid entry exists, valid entry listed) after one base_check_shots on a fresh store."""
    root = Path(tempfile.mkdtemp())
    B.store_root = lambda: root
    B.shots_key = lambda c, b: "K"
    B.bench_def = lambda n: {"name": n}
    B.base_build_run = lambda bds, commit, row: 1  # the rerun stores nothing (a tool fault)
    B.CHECK_PURE, B.RESTORE_KEPT = pure, restore
    other = entry(root, "c1", "row_bench", "OTHER")
    valid = entry(root, "c1", "main_bench", "K")
    B.save_index({"commits": {"c1": {"short": "c1", "benches": ["row_bench", "main_bench"]}}})
    B.base_check_shots(argparse.Namespace(row="BASEKEEP2"), ["c1"], random.Random(1))
    listed = "main_bench" in B.load_index()["commits"].get("c1", {}).get("benches", [])
    out = (other.exists(), valid.exists() and (valid / "drec.txt").exists(), listed)
    shutil.rmtree(root, ignore_errors=True)
    return out


def main():
    if run(True, True) != (True, True, True):
        fail("real: %s, want (True, True, True)" % (run(True, True),))
    if run(False, True)[0]:
        fail("seeded fault CHECK_PURE False not seen (the other-input entry stayed)")
    if run(True, False)[1]:
        fail("seeded fault RESTORE_KEPT False not seen (the checked entry stayed)")
    print("basecheck ok")


if __name__ == "__main__":
    main()
