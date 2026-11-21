#!/usr/bin/env python3
"""fprof_excl.py <log> [--lane PROF4B] [--max 60] [--budget 480000]  |  fprof_excl.py --census <file> [--max ..]: PROF4B.
Picks the exclude list of the fprof lane and the depth D from a census (lane <L>_fprof, PM_FPROF=1, game switch fprofn=<arm>:<end>).

From a log: names are resolved once with the census build's map, and build3ds/out/fprof_census.txt is written (per name: n and dsum
per core, the steps per core). Later rounds pass --census <file> and do not need the map.
Per core, per step: n of each name (census calls / steps). Heavy: n per step above --max on either core.
Excluded = previous list (3ds/tools/fprof_exclude.txt, not visible in the census) + heavy + collateral (a kept name that
contains an excluded name as a substring: GCC -finstrument-functions-exclude-function-list matches substrings).
Prediction per core: calls = sum of kept n per step; pairs(D) = sum of kept n per step with mean depth < D.
ticks = calls x 53 + pairs x 838 (PROF4 hook calibration). D = largest 1..6 with both cores at most --budget.
No D fits: --max lowered by 20 and repeated (down to 20). Writes the list (names only, sorted) and prints the lines.
A log with no core 1 steps (det: every step inline on core 0): the core 0 block is used for both cores (combined 1).

usage (repo root): python 3ds/tools/fprof_excl.py <log> [--lane PROF4B] [--max 60] [--budget 480000]
Exit 0 written, 1 no D fits at any --max, 2 tool fault.
"""
import argparse
import re
import sys
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import hotset  # noqa: E402  (lane_dir, parse_map, REPO)

CALL_TICKS = 53
PAIR_TICKS = 838
LIST = HERE / "fprof_exclude.txt"
CENSUS_OUT = hotset.REPO / "build3ds" / "out" / "fprof_census.txt"
ENT = re.compile(r"fprofn f(\d+) c([01]) ([0-9a-f]{8}) n (\d+) dsum (\d+)")
STEPS = re.compile(r"fprofn f(\d+) c([01]) steps (\d+) calls (\d+) d ((?:\d+ ?){9})")


def read_log(log):
    """-> per core: n and dsum per address, steps and calls totals, depth histogram. Only the first counts window of the log
    counts (the core 0 steps line sets its frame): later window blocks are not the census."""
    n = [defaultdict(int), defaultdict(int)]
    dsum = [defaultdict(int), defaultdict(int)]
    steps, hist = [0, 0], [[0] * 9, [0] * 9]
    with open(log, encoding="utf-8", errors="replace") as fh:
        lines = fh.readlines()
    first = None
    for line in lines:
        m = STEPS.search(line)
        if m and m.group(2) == "0":
            first = m.group(1)
            break
    for line in lines:
        m = ENT.search(line)
        if m:
            if m.group(1) != first:
                continue
            c, a = int(m.group(2)), int(m.group(3), 16)
            n[c][a] += int(m.group(4))
            dsum[c][a] += int(m.group(5))
            continue
        m = STEPS.search(line)
        if m and m.group(1) == first:
            c = int(m.group(2))
            steps[c] += int(m.group(3))
            for i, v in enumerate(m.group(5).split()):
                hist[c][i] += int(v)
    return n, dsum, steps, hist


def table_from_log(log, lane):
    mapf = hotset.lane_dir(lane, fprof=True) / "pm_3ds.map"
    if not mapf.exists():
        raise SystemExit("fprof_excl TOOL FAULT: no map %s (build the lane first)" % mapf)
    fmap = hotset.parse_map(mapf)
    n, dsum, steps, hist = read_log(log)
    if not steps[0]:
        raise SystemExit("fprof_excl TOOL FAULT: no fprofn steps line for core 0 in %s" % log)
    combined = 0
    if not steps[1]:  # det: every step runs inline on core 0
        combined = 1
        n[1], dsum[1], steps[1] = n[0], dsum[0], steps[0]
    table = {}
    for c in (0, 1):
        for a in n[c]:
            nm = fmap[a]["name"] if a in fmap else "0x%08x" % a
            row = table.setdefault(nm, [0, 0, 0, 0])
            row[2 * c] += n[c][a]
            row[2 * c + 1] += dsum[c][a]
    return table, steps, combined, hist


def write_census(path, table, steps, combined):
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("# steps %d %d combined %d\n" % (steps[0], steps[1], combined))
        for nm in sorted(table):
            r = table[nm]
            fh.write("%s %d %d %d %d\n" % (nm, r[0], r[1], r[2], r[3]))


def read_census(path):
    p = Path(path)
    if not p.exists():
        raise SystemExit("fprof_excl TOOL FAULT: no census file %s" % path)
    table, steps, combined = {}, [0, 0], 0
    for line in p.read_text(encoding="utf-8").splitlines():
        if line.startswith("# steps"):
            parts = line.split()
            steps = [int(parts[2]), int(parts[3])]
            combined = int(parts[5])
            continue
        if not line.strip() or line.startswith("#"):
            continue
        nm, a, b, c, d = line.split()
        table[nm] = [int(a), int(b), int(c), int(d)]
    return table, steps, combined


def previous_names():
    if not LIST.exists():
        return set()
    return {l.strip() for l in LIST.read_text(encoding="utf-8").splitlines() if re.match(r"^[A-Za-z_]", l)}


def pick(max_rate, table, steps, prev, budget):
    """-> (excluded names, collateral names, heavy names, chosen (D, per-core ticks/calls/pairs) or None)."""
    rate = [{nm: r[0] / steps[0] for nm, r in table.items()}, {nm: r[2] / steps[1] for nm, r in table.items()}]
    heavy = {nm for nm in table if not nm.startswith("0x") and max(rate[0][nm], rate[1][nm]) > max_rate}
    base = set(prev) | heavy
    visible = [nm for nm in table if not nm.startswith("0x") and nm not in base]
    collat = {nm for nm in visible if any(b and b in nm for b in base)}
    excl = base | collat
    per = {}
    for c in (0, 1):
        calls = 0.0
        byd = [0.0] * 9
        for nm, row in table.items():
            if nm in excl:
                continue
            k = 2 * c
            if row[k] == 0:
                continue
            r = rate[c][nm]
            calls += r
            mean_d = row[k + 1] / row[k]
            byd[min(int(mean_d), 8)] += r  # mean depth < D  <=>  int(mean_d) < D for D >= 1
        per[c] = (calls, byd)
    chosen = None
    for d in range(1, 7):
        t = []
        for c in (0, 1):
            calls, byd = per[c]
            pairs = sum(byd[:d])
            t.append((calls * CALL_TICKS + pairs * PAIR_TICKS, calls, pairs))
        if t[0][0] <= budget and t[1][0] <= budget:
            chosen = (d, t)
    return excl, collat, heavy, chosen


def main(argv=None):
    ap = argparse.ArgumentParser(description="PROF4B: exclude list and depth D of the fprof lane from a census.")
    ap.add_argument("log", nargs="?", help="census game log (resolves names with the lane map, writes fprof_census.txt)")
    ap.add_argument("--census", default="", help="census file (name n0 dsum0 n1 dsum1) instead of a log")
    ap.add_argument("--lane", default="PROF4B")
    ap.add_argument("--max", type=int, default=60, help="heavy: calls per step above this on either core")
    ap.add_argument("--budget", type=int, default=480000, help="ticks per step per core")
    a = ap.parse_args(argv)
    if a.census:
        table, steps, combined = read_census(a.census)
        hist = None
    elif a.log:
        table, steps, combined, hist = table_from_log(a.log, a.lane)
        write_census(CENSUS_OUT, table, steps, combined)
        print("fprof_excl census file %s names %d" % (CENSUS_OUT, len(table)))
    else:
        print("fprof_excl TOOL FAULT: give a log or --census <file>")
        return 2
    if not steps[0] or not steps[1]:
        print("fprof_excl TOOL FAULT: steps %s in the census" % steps)
        return 2
    prev = previous_names()
    mx = a.max
    while True:
        excl, collat, heavy, chosen = pick(mx, table, steps, prev, a.budget)
        if chosen:
            break
        if mx - 20 < 20:
            print("fprof_excl NO FIT: no D in 1..6 at max %d down to 20 (budget %d). Send to oracle NATIVE18." % (a.max, a.budget))
            return 1
        mx -= 20
    d, t = chosen
    unmapped = sum(1 for nm in table if nm.startswith("0x"))
    names = sorted(excl)
    with open(LIST, "w", encoding="utf-8", newline="") as fh:
        fh.write("# PROF4B: function names that stay inside their caller time (tools/fprof_excl.py, census max %d)\r\n" % mx)
        for nm in names:
            fh.write(nm + "\r\n")
    print("fprof_excl max %d heavy %d prev %d unmapped %d combined %d" % (mx, len(heavy), len(prev), unmapped, combined))
    if hist:
        print("fprof_excl hist0 %s hist1 %s" % (" ".join(str(x) for x in hist[0]), " ".join(str(x) for x in hist[1])))
    for nm in sorted(collat):
        print("collateral %s" % nm)
    print("fprof_excl names %d collateral %d core0 calls %d pairs %d core1 calls %d pairs %d D %d ticks0 %d ticks1 %d combined %d" % (
        len(names), len(collat), t[0][1], t[0][2], t[1][1], t[1][2], d, t[0][0], t[1][0], combined))
    return 0


if __name__ == "__main__":
    sys.exit(main())
