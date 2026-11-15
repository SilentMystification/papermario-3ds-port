"""MESHST: totals of a drec.txt pair (base and row) for a bench. Prints `drecsum <bench> draws A B idx A B tex A B`, then `drecsum equal`
(exit 0) or `drecsum DIFF` (exit 1). Totals per file: draws = count of `draw` lines; idx = sum of word 6 (n) of every `draw` line;
tex = the set of `t` lines without their first two words. Equal: draws, idx and tex equal. --seed: the row is the base with n of the
first `draw` line plus 3 (the self-test: it must print DIFF)."""
import os
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench  # noqa: E402


def load(path):
    return Path(path).read_text(encoding="utf-8").split("\n")


def totals(lines):
    draws = [x for x in lines if x.startswith("draw ")]
    idx = sum(int(x.split(" ")[5]) for x in draws)
    tex = set(" ".join(x.split(" ")[2:]) for x in lines if x.startswith("t "))
    return len(draws), idx, tex


def main(argv):
    if not argv or argv[0].startswith("-"):
        print("usage: drecsum.py <bench> --row <id> [--seed]")
        return 2
    name = argv[0]
    row = argv[argv.index("--row") + 1] if "--row" in argv else None
    seed = "--seed" in argv
    commit = bench.base_commit(None)
    if commit is None:
        print("drecsum: no merge-base with main")
        return 2
    base_path = bench.store_root() / commit / name / "drec.txt"
    if not base_path.is_file():
        print("drecsum: no stored base drec %s (run bench.py base %s)" % (base_path, name))
        return 2
    base_lines = load(base_path)
    if seed:
        row_lines = list(base_lines)
        k = next(i for i, x in enumerate(row_lines) if x.startswith("draw "))
        w = row_lines[k].split(" ")
        w[5] = str(int(w[5]) + 3)
        row_lines[k] = " ".join(w)
    else:
        rr = bench.row_shots_run(name, row) if row else None
        if rr is None:
            print("drecsum: no run of %s under row %s with shots" % (name, row))
            return 2
        row_path = Path(rr["shot_path"]) / "drec.txt"
        if not row_path.is_file():
            print("drecsum: no drec.txt in %s" % row_path)
            return 2
        row_lines = load(row_path)
    da, ia, ta = totals(base_lines)
    db, ib, tb = totals(row_lines)
    print("drecsum %s draws %d %d idx %d %d tex %d %d" % (name, da, db, ia, ib, len(ta), len(tb)))
    if da == db and ia == ib and ta == tb:
        print("drecsum equal")
        return 0
    print("drecsum DIFF")
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
