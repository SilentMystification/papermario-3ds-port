#!/usr/bin/env python3
"""BASEREUSE self-test of bench.py base_alias, base_adopt and cmppass_put. Case L: a commit whose diff from a stored commit
touches NONINPUT paths only gets a copy of that base; a 3ds/src diff does not. Case A: an adoption at a merge gives the merge
commit a copy with adopted_from. Cases R1..R7: one refusal each, with its reason. Case W: cmppass_put writes an ok record and
removes it on a failed cmp. Seeded faults: NONINPUT ("docs/", "3ds/") makes case L copy the 3ds/src diff; BASE_ADOPT_MAX 99
makes case R1 adopt. Each seed must be seen. Prints "baseadopt ok"."""
import argparse
import contextlib
import io
import json
import shutil
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
import bench as B  # noqa: E402

BENCH = "spot_test"
ROW = "X"
ORIG_NONINPUT = B.NONINPUT
ORIG_MAX = B.BASE_ADOPT_MAX


def fail(msg):
    print("baseadopt FAIL " + msg)
    sys.exit(1)


def entry(root, commit, drec="draw 1\n"):
    d = root / commit / BENCH
    d.mkdir(parents=True)
    (d / "drec.txt").write_text(drec, encoding="utf-8")
    meta = {"key": "K", "scenario": "s", "row": ROW, "tools": "T", "out_sha1": {"drec.txt": B.bb.sha1_file(d / "drec.txt")}}
    (d / "meta.json").write_text(json.dumps(meta), encoding="utf-8")
    return d


def fresh(changed, nonin=ORIG_NONINPUT, adopt_max=ORIG_MAX):
    """A new store and the mocks: key K, tools T, commits are their own ids, changed_paths from the dict changed (default no change)."""
    root = Path(tempfile.mkdtemp())
    B.store_root = lambda: root
    B.shots_key = lambda c, b: "K"
    B.tools_hash = lambda: "T"
    B.base_commit = lambda c=None: c
    B.changed_paths = lambda a, b: changed.get((a, b), [])
    B.NONINPUT, B.BASE_ADOPT_MAX = nonin, adopt_max
    return root


def rec_set(root, key, val):
    f = root / "cmppass" / ROW / (BENCH + ".json")
    rec = json.loads(f.read_text(encoding="utf-8"))
    rec[key] = val
    f.write_text(json.dumps(rec), encoding="utf-8")


def mut_drec(root, changed):
    """The base at P is valid (its hashes match) but its shots differ from the row cmp."""
    d = root / "P" / BENCH
    (d / "drec.txt").write_text("draw 2\n", encoding="utf-8")
    m = json.loads((d / "meta.json").read_text(encoding="utf-8"))
    m["out_sha1"] = {"drec.txt": B.bb.sha1_file(d / "drec.txt")}
    (d / "meta.json").write_text(json.dumps(m), encoding="utf-8")


def mut_p_tools(root, changed):
    m_path = root / "P" / BENCH / "meta.json"
    m = json.loads(m_path.read_text(encoding="utf-8"))
    m["tools"] = "OLD"
    m_path.write_text(json.dumps(m), encoding="utf-8")


def adopt_case(mut=None, changed=None, nonin=ORIG_NONINPUT, adopt_max=ORIG_MAX):
    """Stores P, a cmp pass record of ROW at P (run commit R, clean), then mut(root, changed), then base_adopt to D.
    Returns (stored_why of D, output text, D meta or None, cmppass folder exists)."""
    changed = dict(changed or {})
    root = fresh(changed, nonin, adopt_max)
    entry(root, "P")
    B.cmppass_put(ROW, BENCH, "P", {"commit": "R", "dirty": False}, True)
    if mut:
        mut(root, changed)
    B.save_index({"commits": {"P": {"short": "P", "benches": [BENCH]}}})
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        B.base_adopt(argparse.Namespace(adopt=ROW, commit="D", parent="P"))
    dm = root / "D" / BENCH / "meta.json"
    meta = json.loads(dm.read_text(encoding="utf-8")) if dm.exists() else None
    res = (B.stored_why("D", BENCH), buf.getvalue(), meta, (root / "cmppass" / ROW).exists())
    shutil.rmtree(root, ignore_errors=True)
    return res


def alias_case(paths, nonin=ORIG_NONINPUT):
    """Stores S. base_alias(C): returns (True/False, stored_why of C, output text)."""
    changed = {("S", "C"): paths}
    root = fresh(changed, nonin)
    entry(root, "S")
    B.save_index({"commits": {"S": {"short": "S", "benches": [BENCH]}}})
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        got = B.base_alias("C", BENCH)
    why = B.stored_why("C", BENCH)
    shutil.rmtree(root, ignore_errors=True)
    return got, why, buf.getvalue()


def main():
    # L: docs-only diff copies the base; a 3ds/src diff does not
    got, why, out = alias_case(["docs/a.md"])
    if not (got and why == "" and "copied. No build, no run." in out):
        fail("L docs: got %s why %r out %r" % (got, why, out))
    got, why, out = alias_case(["3ds/src/a.c"])
    if got or why != "missing":
        fail("L src: got %s why %r, want False and missing" % (got, why))

    # A: the adoption gives D a copy of the base of P, marked adopted_from, the count 1, the record removed
    why, out, meta, cp = adopt_case()
    if why != "" or "adoption 1 of 4" not in out or cp or not meta or "adopted_from" not in meta:
        fail("A: why %r out %r meta %s cmppass %s" % (why, out, meta is not None, cp))

    # R1..R7: each one refusal, with its reason, nothing stored at D, the record removed
    def ledger4(root, changed):
        B.ledger({BENCH: 4})

    def run_row_inputs(root, changed):
        changed[("R", "refs/heads/X")] = ["3ds/src/a.c"]

    def no_p(root, changed):
        shutil.rmtree(root / "P")

    refusals = [
        ("R1 ledger", ledger4, "full base due"),
        ("R2 tools", lambda r, c: rec_set(r, "tools", "OLD"), "the measure tools changed since the cmp"),
        ("R3 dirty", lambda r, c: rec_set(r, "dirty", True), "the row run had uncommitted changes (dirty True)"),
        ("R4 inputs", run_row_inputs, "the row run is not at the inputs of the branch tip"),
        ("R5 no P", no_p, "main before the merge (P) has no stored base"),
        ("R6 P shots", mut_drec, "the base at P differs from the base of the row cmp"),
        ("R7 P tools", mut_p_tools, "the base at P was made by other measure tools"),
    ]
    for name, mut, reason in refusals:
        why, out, meta, cp = adopt_case(mut)
        if why != "missing" or meta is not None or ("refused: %s" % reason) not in out or cp:
            fail("%s: stored %r, out %r, cmppass %s, want missing and refused: %s" % (name, why, out, cp, reason))

    # W: cmppass_put writes an ok record and removes it on a failed cmp
    root = fresh({})
    entry(root, "P")
    B.cmppass_put(ROW, BENCH, "P", {"commit": "R", "dirty": False}, True)
    f = root / "cmppass" / ROW / (BENCH + ".json")
    rec = json.loads(f.read_text(encoding="utf-8")) if f.exists() else {}
    if not (rec.get("base") == "P" and rec.get("run_commit") == "R" and rec.get("tools") == "T" and rec.get("dirty") is False):
        fail("W write: record %r" % rec)
    B.cmppass_put(ROW, BENCH, "P", {"commit": "R", "dirty": False}, False)
    if f.exists():
        fail("W remove: record kept after a failed cmp")
    shutil.rmtree(root, ignore_errors=True)

    # seeds: each must change its case
    got, why, out = alias_case(["3ds/src/a.c"], nonin=("docs/", "3ds/"))
    if not got:
        fail("seeded fault NONINPUT ('docs/', '3ds/') not seen (3ds/src diff not copied)")
    why, out, meta, cp = adopt_case(ledger4, adopt_max=99)
    if why != "":
        fail("seeded fault BASE_ADOPT_MAX 99 not seen (R1 still refused: %r)" % out)
    print("baseadopt ok")


if __name__ == "__main__":
    main()
