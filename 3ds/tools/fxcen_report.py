#!/usr/bin/env python3
"""FXCEN1: per-effect cost census from the fxtest log lines (fxcena, fxcenb). Counts only; no picture.

  python 3ds/tools/fxcen_report.py <log>... [--label azahar]

Input: the log copies of fxtest runs with the census (fxtest on: pm_cnt_on). Lines:
  fxcena <idx> <name> frames <n> us <us> live <k>
  fxcenb <idx> tri <n> vtx <n> ... mclr <n>     (deltas over the window of fxcena)
  fxcenc <idx> gpu <n> walk <n>                 (deltas over the window of fxcena)
idx -1 is the base window (frames 360 to 400, no effect). Every value is divided by the frames of its window; the
base per-frame value is subtracted from each effect's per-frame value. Each log file has its own base line (one boot
per chunk), so each file's effects are measured against that file's own base.
gpu = GPU drawing us per frame (C3D_GetDrawingTime), walk = core 0 stream walk us per frame; both above the base window.
Azahar values are not the console.

Output: build3ds/out/fxcen_<label>.txt. One table sorted by us per frame, then the top 15 by draw, ndraw, tri+gvtri,
texmiss, cmd, upd, cfull and mmiss.
The us column is a proxy on Azahar (tick count of the logic step): the console decides.
"""
import argparse
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT_DIR = os.path.join(ROOT, "build3ds", "out")

RE_A = re.compile(r"fxcena (-?\d+) (\S+) frames (\d+) us (\d+) live (\d+)")
RE_B = re.compile(r"fxcenb (-?\d+) ((?:[a-z]+ -?\d+ ?)+)")
RE_C = re.compile(r"fxcenc (-?\d+) gpu (\d+) walk (\d+)")

COUNTS = ["tri", "vtx", "draw", "tex", "texmiss", "texkb", "cmd", "gvtri", "gvrej", "gvfb",
          "flkb", "ndraw", "nidx", "prg", "shr", "upd", "spl", "sprmiss", "cfull", "mmiss", "mclr", "gpu", "walk"]
TOP = [("draw", ["draw"]), ("ndraw", ["ndraw"]), ("tri+gvtri", ["tri", "gvtri"]), ("texmiss", ["texmiss"]),
       ("cmd", ["cmd"]), ("upd", ["upd"]), ("cfull", ["cfull"]), ("mmiss", ["mmiss"])]


def parse(path):
    """Return {idx: {"name", "frames", "us", "live", "c": {field: value}}} for one log file; a repeated index keeps the last line."""
    rec = {}
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            m = RE_A.search(line)
            if m:
                idx = int(m.group(1))
                r = rec.setdefault(idx, {"c": {}})
                r.update(name=m.group(2), frames=int(m.group(3)), us=int(m.group(4)), live=int(m.group(5)))
                continue
            m = RE_B.search(line)
            if m:
                idx = int(m.group(1))
                r = rec.setdefault(idx, {"c": {}})
                toks = m.group(2).split()
                r["c"].update({toks[i]: int(toks[i + 1]) for i in range(0, len(toks) - 1, 2)})
                continue
            m = RE_C.search(line)
            if m:
                idx = int(m.group(1))
                rec.setdefault(idx, {"c": {}})["c"].update(gpu=int(m.group(2)), walk=int(m.group(3)))
    return rec


def per_frame(rec, base):
    """Per-frame values of one record minus the base per-frame values."""
    fr = rec["frames"]
    bf = base["frames"]
    out = {"us": rec["us"] / fr - base["us"] / bf}
    for k in COUNTS:
        out[k] = rec["c"].get(k, 0) / fr - base["c"].get(k, 0) / bf
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("logs", nargs="+")
    ap.add_argument("--label", default="azahar")
    a = ap.parse_args()
    rows = []
    base_lines = []
    for path in a.logs:
        rec = parse(path)
        if -1 not in rec or "frames" not in rec[-1]:
            sys.exit("fxcen_report: no base line (fxcena -1) in %s" % path)
        base = rec[-1]
        base_lines.append("base %s: frames %d us per frame %.1f draw per frame %.2f" % (
            os.path.basename(path), base["frames"], base["us"] / base["frames"],
            base["c"].get("draw", 0) / base["frames"]))
        for idx in sorted(k for k in rec if k >= 0 and "frames" in rec[k] and rec[k]["c"]):
            r = rec[idx]
            pf = per_frame(r, base)
            rows.append((idx, r["name"], r["live"], r["frames"], pf))
    lines = []
    lines.append("Azahar us is a proxy; the console decides")
    lines.extend(base_lines)
    lines.append("values: per frame, base per-frame value of each effect's own log subtracted; us in us per frame (logic step)")
    lines.append("")
    hdr = "%4s %-24s %4s %4s %9s " % ("idx", "name", "frm", "live", "us") + " ".join("%8s" % k for k in COUNTS)
    lines.append("table sorted by us")
    lines.append(hdr)
    for idx, name, live, frames, pf in sorted(rows, key=lambda x: -x[4]["us"]):
        lines.append("%4d %-24s %4d %4d %9.1f " % (idx, name, frames, live, pf["us"]) +
                     " ".join("%8.2f" % pf[k] for k in COUNTS))
    lines.append("")
    for label, keys in TOP:
        lines.append("top 15 by %s (per frame)" % label)
        scored = sorted(rows, key=lambda x: -sum(x[4][k] for k in keys))[:15]
        for idx, name, live, frames, pf in scored:
            lines.append("  %4d %-24s %10.2f" % (idx, name, sum(pf[k] for k in keys)))
        lines.append("")
    os.makedirs(OUT_DIR, exist_ok=True)
    out_path = os.path.join(OUT_DIR, "fxcen_%s.txt" % a.label)
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print("fxcen_report: %d effects, base frames %d" % (len(rows), base["frames"]))
    print(out_path)


if __name__ == "__main__":
    main()
