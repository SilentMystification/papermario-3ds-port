#!/usr/bin/env python3
"""prof_by_scene.py: group prof windows by the scene they ran in and compare runs scene by scene.

  python 3ds/tools/prof_by_scene.py <log> [<log2> ...]

Each prof line ends with ` | at <map> <area>:<map> m<mode> sp<story> b<battle> chg<0|1>` (pm_phase.c pf_report).
Key = (map name, mode, story, battle). A window with chg1 mixed two scenes and is dropped. Lines without the tag (older builds) go to the key "untagged".
One log: one row per scene (windows, mean fps, ft, nd, lg, gb). Two or more logs: the same scene side by side, with the change of fps and ft from the first log.
"""
import re, sys, collections

AT = re.compile(r"\| at (\S+) (-?\d+):(-?\d+) m(-?\d+) sp(-?\d+) b(-?\d+) chg(\d)")
F1 = re.compile(r"prof f(\d+) w\d+ fps ([\d.]+) \| ft ([\d.]+)")
F2 = re.compile(r"prof f(\d+) w\d+ nd ([\d.]+) lg ([\d.]+) .* gb ([\d.]+)")


def parse(path):
    win = collections.OrderedDict()  # frame -> dict
    for ln in open(path, errors="replace"):
        m1, m2 = F1.search(ln), F2.search(ln)
        if m1:
            w = win.setdefault(int(m1.group(1)), {})
            w["fps"], w["ft"] = float(m1.group(2)), float(m1.group(3))
            a = AT.search(ln)
            w["key"] = ("%s m%s sp%s b%s" % (a.group(1), a.group(4), a.group(5), a.group(6)) if a else "untagged")
            w["chg"] = bool(a and a.group(7) == "1")
        elif m2:
            w = win.setdefault(int(m2.group(1)), {})
            w["nd"], w["lg"], w["gb"] = float(m2.group(2)), float(m2.group(3)), float(m2.group(4))
    by = collections.defaultdict(list)
    dropped = 0
    for w in win.values():
        if "fps" not in w:
            continue
        if w["chg"]:
            dropped += 1
            continue
        by[w["key"]].append(w)
    return by, dropped


def mean(ws, k):
    v = [w[k] for w in ws if k in w]
    return sum(v) / len(v) if v else float("nan")


def main(argv):
    if not argv:
        print(__doc__)
        return 2
    runs = [parse(p) for p in argv]
    keys = []
    for by, _ in runs:
        for k in by:
            if k not in keys:
                keys.append(k)
    for i, p in enumerate(argv):
        print("log %d: %s (scene-change windows dropped: %d)" % (i, p, runs[i][1]))
    print("%-36s %s" % ("scene", " | ".join("log%d n fps ft nd lg gb" % i for i in range(len(runs)))))
    for k in keys:
        cells = []
        for by, _ in runs:
            ws = by.get(k)
            cells.append("%2d %5.1f %5.1f %5.1f %5.1f %5.1f" % (len(ws), mean(ws, "fps"), mean(ws, "ft"), mean(ws, "nd"), mean(ws, "lg"), mean(ws, "gb")) if ws else "-")
        extra = ""
        if len(runs) > 1 and k in runs[0][0] and k in runs[-1][0]:
            a, b = runs[0][0][k], runs[-1][0][k]
            extra = "  d fps %+.1f d ft %+.1f" % (mean(b, "fps") - mean(a, "fps"), mean(b, "ft") - mean(a, "ft"))
        print("%-36s %s%s" % (k, " | ".join(cells), extra))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
