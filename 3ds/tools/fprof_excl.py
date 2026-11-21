#!/usr/bin/env python3
"""fprof_excl.py <log> [--lane PROF4B] [--max 60] [--budget 480000]: PROF4B. Picks the exclude list of the fprof lane and the depth D
from a census log (lane <L>_fprof built with PM_FPROF=1, game switch fprofn: the fprofn lines of pm_fprof.c).

Per core, per step: n of each function (census calls). A name is heavy when its n per step is above --max on either core.
Excluded = previous list (3ds/tools/fprof_exclude.txt, not visible in the census) + heavy + collateral (a kept name that
contains an excluded name as a substring: GCC -finstrument-functions-exclude-function-list matches substrings).
Prediction per core: calls = sum of kept n per step; pairs(D) = sum of kept n per step with mean depth < D.
ticks = calls x 53 + pairs x 838 (PROF4 hook calibration). D = largest 1..6 with both cores at most --budget.
No D fits: --max lowered by 20 and repeated (down to 20). Writes the list (names only, sorted) and prints the lines.

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
import hotset  # noqa: E402  (lane_dir, parse_map)

CALL_TICKS = 53
PAIR_TICKS = 838
LIST = HERE / "fprof_exclude.txt"
ENT = re.compile(r"fprofn f(\d+) c([01]) ([0-9a-f]{8}) n (\d+) dsum (\d+)")
STEPS = re.compile(r"fprofn f(\d+) c([01]) steps (\d+) calls (\d+) d ((?:\d+ ?){9})")


def read_log(log):
    """-> per core: n and dsum per address (sums over windows), steps and calls totals, depth histogram.
    Only the first counts window of the log counts (the core 0 steps line sets its frame): later window blocks are not the census."""
    n = [defaultdict(int), defaultdict(int)]
    dsum = [defaultdict(int), defaultdict(int)]
    steps, calls, hist = [0, 0], [0, 0], [[0] * 9, [0] * 9]
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
        if m:
            if m.group(1) != first:
                continue
            c = int(m.group(2))
            steps[c] += int(m.group(3))
            calls[c] += int(m.group(4))
            for i, v in enumerate(m.group(5).split()):
                hist[c][i] += int(v)
    return n, dsum, steps, calls, hist


def previous_names():
    if not LIST.exists():
        return set()
    return {l.strip() for l in LIST.read_text(encoding="utf-8").splitlines() if re.match(r"^[A-Za-z_]", l)}


def pick(max_rate, n, dsum, steps, fmap, prev, budget):
    """-> (excluded names, collateral names, heavy names, per-core dict of calls/pairs/ticks for each D, chosen D or None)."""
    rate = [{a: n[c][a] / steps[c] for a in n[c]} for c in (0, 1)]
    addrs = set(rate[0]) | set(rate[1])
    names = {}
    for a in addrs:
        names[a] = fmap[a]["name"] if a in fmap else None
    heavy = {names[a] for a in addrs if names[a] and max(rate[0].get(a, 0.0), rate[1].get(a, 0.0)) > max_rate}
    base = set(prev) | heavy
    visible = {names[a] for a in addrs if names[a] and names[a] not in base}
    collat = {nm for nm in visible if any(b and b in nm for b in base)}
    excl = base | collat
    per = {}
    for c in (0, 1):
        calls = 0.0
        byd = [0.0] * 9
        for a, r in rate[c].items():
            nm = names[a]
            if nm and nm in excl:
                continue
            calls += r
            mean_d = dsum[c][a] / n[c][a]
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
    ap = argparse.ArgumentParser(description="PROF4B: exclude list and depth D of the fprof lane from a census log.")
    ap.add_argument("log")
    ap.add_argument("--lane", default="PROF4B")
    ap.add_argument("--max", type=int, default=60, help="heavy: calls per step above this on either core")
    ap.add_argument("--budget", type=int, default=480000, help="ticks per step per core")
    a = ap.parse_args(argv)
    mapf = hotset.lane_dir(a.lane, fprof=True) / "pm_3ds.map"
    if not mapf.exists():
        print("fprof_excl TOOL FAULT: no map %s (build the lane first)" % mapf)
        return 2
    fmap = hotset.parse_map(mapf)
    n, dsum, steps, calls, hist = read_log(a.log)
    if not steps[0]:
        print("fprof_excl TOOL FAULT: no fprofn steps line for core 0 in %s" % a.log)
        return 2
    combined = 0
    if not steps[1]:  # the capchk words set det: every step runs inline on core 0, core 1 has no steps
        combined = 1
        n[1], dsum[1], steps[1], calls[1], hist[1] = n[0], dsum[0], steps[0], calls[0], hist[0]
    prev = previous_names()
    mx = a.max
    while True:
        excl, collat, heavy, chosen = pick(mx, n, dsum, steps, fmap, prev, a.budget)
        if chosen:
            break
        if mx - 20 < 20:
            print("fprof_excl NO FIT: no D in 1..6 at max %d down to 20 (budget %d). Send to oracle NATIVE18." % (a.max, a.budget))
            return 1
        mx -= 20
    d, t = chosen
    unmapped = sum(1 for c in (0, 1) for x in n[c] if x not in fmap)
    names = sorted(excl)
    with open(LIST, "w", encoding="utf-8", newline="") as fh:
        fh.write("# PROF4B: function names that stay inside their caller time (tools/fprof_excl.py, census max %d)\r\n" % mx)
        for nm in names:
            fh.write(nm + "\r\n")
    print("fprof_excl max %d heavy %d prev %d unmapped %d hist0 %s hist1 %s" % (mx, len(heavy), len(prev), unmapped,
          " ".join(str(x) for x in hist[0]), " ".join(str(x) for x in hist[1])))
    for nm in sorted(collat):
        print("collateral %s" % nm)
    print("fprof_excl names %d collateral %d core0 calls %d pairs %d core1 calls %d pairs %d D %d ticks0 %d ticks1 %d combined %d" % (
        len(names), len(collat), t[0][1], t[0][2], t[1][1], t[1][2], d, t[0][0], t[1][0], combined))
    return 0


if __name__ == "__main__":
    sys.exit(main())
