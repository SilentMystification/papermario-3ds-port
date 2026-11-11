#!/usr/bin/env python3
"""pmu_report.py: the PMU report of one prof log (profrot and profph lines).

  python 3ds/tools/pmu_report.py <log> [--label L]

Reads the pmx lines (core 0, per clean frame), the pmx1 lines (core 1, per logic step) and the pmxph lines (per sampled frame, per
phase: a and b parts of one window joined). Groups the windows by counter set (m, s, b, w, q, t) and averages per set.
Derived (run averages over all windows of the set named): CPI = cyc / inst (set b); I-miss and D-read-miss per 1000 instructions
(set m e0, e1 over the set b inst); bmiss per 1000 inst (set b); istall and dstall share of cyc (set s).
Phase share: the phase e0 (or e1) of the window over the sum of the printed phases of that window, then averaged over the windows
of the set. The pmxph lines carry the 6 phases with the largest e0 of the window; the share is of those 6 only.
Writes build3ds/out/pmu_<label>.txt (repo root of this worktree) and prints it.
Console values only: Azahar does not count.
"""
import os, re, sys, collections

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from prof_by_scene import parse_windows  # noqa: E402  (window count of the log)

PMX = re.compile(r"prof f(\d+) w\d+ pmx set ([a-z]) (\w+) (\d+) (\w+) (\d+) cyc (\d+)")
PMX1 = re.compile(r"prof f(\d+) w\d+ pmx1 set ([a-z]) c1_(\w+) (\d+) c1_(\w+) (\d+) c1_cyc (\d+)")
PMXPH = re.compile(r"prof f(\d+) w\d+ pmxph ([ab]) set ([a-z]) (.*)")
SETS = "msbwqt"


def mean(xs):
    return sum(xs) / len(xs) if xs else None


def fmt(x, nd=2):
    return "n/a" if x is None else ("%." + str(nd) + "f") % x


def parse(path):
    core0 = collections.defaultdict(list)   # set -> [(frame, n0, e0, n1, e1, cyc)]
    core1 = collections.defaultdict(list)   # set -> [(frame, n0, ic1, n1, dc1, cyc1)]
    ph = collections.defaultdict(dict)      # (set, frame) -> {part: [(name, e0, e1)]}
    with open(path, errors="replace") as fh:
        for ln in fh:
            m = PMX.search(ln)
            if m:
                core0[m.group(2)].append((int(m.group(1)), m.group(3), int(m.group(4)), m.group(5), int(m.group(6)), int(m.group(7))))
                continue
            m = PMX1.search(ln)
            if m:
                core1[m.group(2)].append((int(m.group(1)), m.group(3), int(m.group(4)), m.group(5), int(m.group(6)), int(m.group(7))))
                continue
            m = PMXPH.search(ln)
            if m:
                toks = m.group(4).split()
                rows = []
                for i in range(0, len(toks) - 2, 3):
                    if toks[i] == "-":
                        continue
                    rows.append((toks[i], int(toks[i + 1]), int(toks[i + 2])))
                ph[(m.group(3), int(m.group(1)))][m.group(2)] = rows
    return core0, core1, ph


def avg_by_set(recs, idx):
    out = {}
    for s in SETS:
        if s in recs and recs[s]:
            out[s] = mean([r[idx] for r in recs[s]])
    return out


def main(argv):
    if not argv or argv[0].startswith("-"):
        print(__doc__)
        return 2
    path = argv[0]
    label = os.path.splitext(os.path.basename(path))[0]
    if "--label" in argv:
        label = argv[argv.index("--label") + 1]
    core0, core1, ph = parse(path)
    nwin = len(parse_windows(path))
    lines = []
    lines.append("console values only; Azahar does not count")
    lines.append("label %s log %s windows %d" % (label, path, nwin))
    lines.append("")
    lines.append("core 0 per clean frame (pmx), mean over the windows of the set")
    lines.append("set  n  e0name     e0        e1name     e1        cyc")
    for s in SETS:
        recs = core0.get(s)
        if not recs:
            continue
        lines.append("%s  %3d  %-9s %9s  %-9s %9s  %9s" % (
            s, len(recs), recs[0][1], fmt(mean([r[2] for r in recs]), 1),
            recs[0][3], fmt(mean([r[4] for r in recs]), 1), fmt(mean([r[5] for r in recs]), 1)))
    lines.append("")
    lines.append("core 1 per logic step (pmx1), mean over the windows of the set")
    lines.append("set  n  ic1name    ic1       dc1name    dc1       cyc1")
    for s in SETS:
        recs = core1.get(s)
        if not recs:
            continue
        lines.append("%s  %3d  %-9s %9s  %-9s %9s  %9s" % (
            s, len(recs), recs[0][1], fmt(mean([r[2] for r in recs]), 1),
            recs[0][3], fmt(mean([r[4] for r in recs]), 1), fmt(mean([r[5] for r in recs]), 1)))
    lines.append("")
    # derived, run averages over the windows of each set
    c0 = {s: core0.get(s, []) for s in SETS}
    e0m, e1m = avg_by_set(c0, 2), avg_by_set(c0, 4)
    cym = avg_by_set(c0, 5)
    inst = e0m.get("b")
    lines.append("derived (run averages)")
    if inst:
        lines.append("CPI (set b) %s" % fmt(cym.get("b", 0) / inst if "b" in cym else None, 3))
        lines.append("I-miss per 1000 inst (set m) %s" % fmt(e0m["m"] * 1000.0 / inst if "m" in e0m else None))
        lines.append("D-read-miss per 1000 inst (set m) %s" % fmt(e1m["m"] * 1000.0 / inst if "m" in e1m else None))
        lines.append("bmiss per 1000 inst (set b) %s" % fmt(e1m["b"] * 1000.0 / inst if "b" in e1m else None))
    else:
        lines.append("CPI, I-miss, D-read-miss, bmiss: no set b window in this log (n/a)")
    if "s" in e0m and cym.get("s"):
        lines.append("istall share of cyc (set s) %s" % fmt(100.0 * e0m["s"] / cym["s"]))
        lines.append("dstall share of cyc (set s) %s" % fmt(100.0 * e1m["s"] / cym["s"]))
    else:
        lines.append("istall, dstall share: no set s window in this log (n/a)")
    lines.append("")
    # pmxph: per set, per phase: window-mean e0 and e1 per sampled frame, and share of the printed phases
    tot = collections.defaultdict(lambda: [0.0, 0.0])  # set -> sums of e0, e1 over the windows
    acc = collections.defaultdict(lambda: collections.defaultdict(lambda: [0.0, 0.0, 0]))  # set -> phase -> [e0, e1, windows]
    for (s, frame), parts in ph.items():
        rows = parts.get("a", []) + parts.get("b", [])
        for name, e0, e1 in rows:
            acc[s][name][0] += e0
            acc[s][name][1] += e1
            acc[s][name][2] += 1
            tot[s][0] += e0
            tot[s][1] += e1
    nwin_s = collections.Counter(s for (s, _f) in ph.keys())
    phase_show = {"gbi": "draw (gbi)", "dl": "gamedraw (dl)"}  # display only: log keys stay gbi and dl
    for s in SETS:
        if s not in acc:
            continue
        n = nwin_s[s]
        lines.append("phases, set %s (%d windows, per sampled frame, mean over windows)" % (s, n))
        for idx, ev in ((0, "e0"), (1, "e1")):
            lines.append("ranked by %s:" % ev)
            lines.append("  phase           %s/frame   share" % ev)
            order = sorted(acc[s].items(), key=lambda kv: -kv[1][idx])
            for name, v in order:
                share = 100.0 * v[idx] / tot[s][idx] if tot[s][idx] else 0.0
                lines.append("  %-15s %12s %7s%%" % (phase_show.get(name, name), fmt(v[idx] / n, 1), fmt(share, 1)))
        lines.append("")
    text = "\n".join(lines) + "\n"
    root = os.path.abspath(os.path.join(HERE, "..", ".."))
    outdir = os.path.join(root, "build3ds", "out")
    os.makedirs(outdir, exist_ok=True)
    outp = os.path.join(outdir, "pmu_%s.txt" % label)
    with open(outp, "w", encoding="utf-8") as fh:
        fh.write(text)
    sys.stdout.write(text)
    print("wrote " + outp)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
