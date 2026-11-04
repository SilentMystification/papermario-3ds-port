#!/usr/bin/env python3
"""prof_by_scene.py: group prof windows by the scene they ran in and compare runs scene by scene.

  python 3ds/tools/prof_by_scene.py <log> [<log2> ...]

The scene line `prof f<n> w<n> at <map> <area>:<map> m<mode> sp<story> b<battle> chg<0|1> pos0 <x y z> pos <x y z>` (Mario at the start and the end of the window) follows the other prof lines of its window (the first line is too long for the 256 byte log slot) (pm_phase.c pf_report).
Key = (map name, mode, story, battle, entry). The map id names the building or room (each Paper Mario interior is its own map); the entry tells the doors of one map apart. pos = Mario x y z at the end of the window (the table shows the x and z range over the windows of the scene). A window with chg1 mixed two scenes and is dropped. Lines without the tag (older builds) go to the key "untagged".
One log: one row per scene (windows, mean fps, ft, nd, lg, gb). Two or more logs: the same scene side by side, with the change of fps and ft from the first log.
"""
import re, sys, collections

AT = re.compile(r"prof f(\d+) w\d+ at (\S+) (-?\d+):(-?\d+) m(-?\d+) sp(-?\d+) b(-?\d+) e(-?\d+) chg(\d)(?: pos0 (-?\d+) (-?\d+) (-?\d+))?(?: pos (-?\d+) (-?\d+) (-?\d+))?")
F1 = re.compile(r"prof f(\d+) w\d+ fps ([\d.]+) \| ft ([\d.]+)")
F2 = re.compile(r"prof f(\d+) w\d+ nd ([\d.]+) lg ([\d.]+) .* gb ([\d.]+)")


def parse(path):
    win = collections.OrderedDict()  # frame -> dict
    for ln in open(path, errors="replace"):
        m1, m2, a = F1.search(ln), F2.search(ln), AT.search(ln)
        if m1:
            w = win.setdefault(int(m1.group(1)), {})
            w["fps"], w["ft"] = float(m1.group(2)), float(m1.group(3))
        elif m2:
            w = win.setdefault(int(m2.group(1)), {})
            w["nd"], w["lg"], w["gb"] = float(m2.group(2)), float(m2.group(3)), float(m2.group(4))
        elif a:
            w = win.setdefault(int(a.group(1)), {})
            w["key"] = "%s m%s sp%s b%s e%s" % (a.group(2), a.group(5), a.group(6), a.group(7), a.group(8))
            w["chg"] = a.group(9) == "1"
            w["pos0"] = tuple(int(a.group(i)) for i in (10, 11, 12)) if a.group(10) else None
            w["pos"] = tuple(int(a.group(i)) for i in (13, 14, 15)) if a.group(13) else None
    for w in win.values():
        w.setdefault("key", "untagged")
        w.setdefault("chg", False)
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
        ps = [q for by, _ in runs for w in by.get(k, []) for q in (w.get("pos0"), w.get("pos")) if q]
        mv = [((w["pos"][0] - w["pos0"][0]) ** 2 + (w["pos"][2] - w["pos0"][2]) ** 2) ** 0.5 for by, _ in runs for w in by.get(k, []) if w.get("pos") and w.get("pos0")]
        extra = ""
        if ps:
            extra = "  moved %.0f avg (start to end) pos x %d..%d z %d..%d" % ((sum(mv) / len(mv)) if mv else 0.0, min(p[0] for p in ps), max(p[0] for p in ps), min(p[2] for p in ps), max(p[2] for p in ps))
        if len(runs) > 1 and k in runs[0][0] and k in runs[-1][0]:
            a, b = runs[0][0][k], runs[-1][0][k]
            extra += "  d fps %+.1f d ft %+.1f" % (mean(b, "fps") - mean(a, "fps"), mean(b, "ft") - mean(a, "ft"))
        print("%-36s %s%s" % (k, " | ".join(cells), extra))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
