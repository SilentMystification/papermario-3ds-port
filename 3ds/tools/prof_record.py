#!/usr/bin/env python3
"""prof_record.py: keep the console prof numbers as a history (docs/handoff/perf_history.csv) and print the trend per scene.

  python 3ds/tools/prof_record.py add <log> --label "<what was measured>" [--commit <hash>] [--console o3dsxl] [--note "<text>"]
        Parses the prof lines of a console log (prof mode, scene tagged by pm_phase.c) and appends one row per scene to the CSV: the mean over the windows
        that ran in that scene (fps, frame ms, nd ms, lg ms, gb ms), the window count, the Mario x and z range, the commit, the date and the label.
  python 3ds/tools/prof_record.py trend [<scene substring>]
        One block per scene: every recorded row in time order with the change of fps and ft against the row before.
  python 3ds/tools/prof_record.py list
        The recorded rows.

Rules for a fair row: prof mode only (docs: memory feedback_prof-mode-for-perf-tests), the same warp and pos for the runs that are compared, one label per run. Rows of a log
without the scene tag are recorded under the scene "untagged" (older builds).
"""
import csv, datetime, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
CSV = os.path.join(HERE, "..", "..", "docs", "handoff", "perf_history.csv")
FIELDS = ["date", "commit", "console", "label", "scene", "windows", "fps", "ft_ms", "nd_ms", "lg_ms", "gb_ms", "x_range", "z_range", "note"]
sys.path.insert(0, HERE)
import prof_by_scene as pb


def mean(ws, k):
    v = [w[k] for w in ws if k in w]
    return sum(v) / len(v) if v else float("nan")


def git_head():
    try:
        return subprocess.check_output(["git", "rev-parse", "--short", "HEAD"], cwd=HERE, text=True).strip()
    except Exception:
        return "?"


def rows():
    if not os.path.exists(CSV):
        return []
    return list(csv.DictReader(open(CSV, newline="", encoding="utf-8")))


def add(argv):
    log = argv[0]
    opt = {"--label": "", "--commit": git_head(), "--console": "o3dsxl", "--note": ""}
    i = 1
    while i + 1 < len(argv):
        opt[argv[i]] = argv[i + 1]
        i += 2
    if not opt["--label"]:
        print("add needs --label")
        return 2
    by, dropped = pb.parse(log)
    new = not os.path.exists(CSV)
    with open(CSV, "a", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, FIELDS)
        if new:
            w.writeheader()
        for key, ws in by.items():
            ps = [q for x in ws for q in (x.get("pos0"), x.get("pos")) if q]
            xr = "%d..%d" % (min(p[0] for p in ps), max(p[0] for p in ps)) if ps else ""
            zr = "%d..%d" % (min(p[2] for p in ps), max(p[2] for p in ps)) if ps else ""
            w.writerow({"date": datetime.date.today().isoformat(), "commit": opt["--commit"], "console": opt["--console"], "label": opt["--label"], "scene": key,
                        "windows": len(ws), "fps": "%.1f" % mean(ws, "fps"), "ft_ms": "%.1f" % mean(ws, "ft"), "nd_ms": "%.1f" % mean(ws, "nd"),
                        "lg_ms": "%.1f" % mean(ws, "lg"), "gb_ms": "%.1f" % mean(ws, "gb"), "x_range": xr, "z_range": zr, "note": opt["--note"]})
            print("recorded %-36s windows %d fps %.1f ft %.1f" % (key, len(ws), mean(ws, "fps"), mean(ws, "ft")))
    print("scene-change windows dropped: %d" % dropped)
    return 0


def trend(argv):
    sel = argv[0] if argv else ""
    rs = [r for r in rows() if sel in r["scene"]]
    scenes = []
    for r in rs:
        if r["scene"] not in scenes:
            scenes.append(r["scene"])
    for sc in scenes:
        print(sc)
        prev = None
        for r in [x for x in rs if x["scene"] == sc]:
            d = ""
            if prev:
                d = "  d fps %+.1f d ft %+.1f" % (float(r["fps"]) - float(prev["fps"]), float(r["ft_ms"]) - float(prev["ft_ms"]))
            print("  %s %-8s w%-2s fps %5s ft %5s nd %5s lg %5s gb %5s x %-10s z %-10s %s%s" % (r["date"], r["commit"], r["windows"], r["fps"], r["ft_ms"], r["nd_ms"], r["lg_ms"], r["gb_ms"],
                  r["x_range"], r["z_range"], r["label"], d))
            prev = r
    return 0


def main(argv):
    if not argv:
        print(__doc__)
        return 2
    if argv[0] == "add":
        return add(argv[1:])
    if argv[0] == "trend":
        return trend(argv[1:])
    if argv[0] == "list":
        for r in rows():
            print(", ".join(r[k] for k in FIELDS))
        return 0
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
