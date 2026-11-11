#!/usr/bin/env python3
"""NAPI0 census reader: the napicen lines of a run log (switch napicen) mapped to the object file and the
function that recorded each typed op. Usage: napicen.py <log> [--map <pm_3ds.map>] [--label <name>]

The game log holds, per 300 frame window, the 40 largest callers (return address of nd_gx_w) as
"napicen f<N> ra <hex> n <ops per frame>" and "napicen f<N> done total <ops per frame>". Each ra is mapped with the
.text.<name> section lines of the linker map (or the object's .text line when the object has no per-function sections).
Writes build3ds/out/napicen_<label>.txt (relative to the working directory): per window, per file ops per frame and
per function ops per frame, largest first. Only the 40 largest callers of each window are in the log, so the file and
function sums can be below the done total."""
import argparse
import bisect
import os
import re
import sys

LINE = re.compile(r"napicen f(\d+) ra ([0-9a-f]+) n (\d+)")
DONE = re.compile(r"napicen f(\d+) done total (\d+)")
# one line: " .text.<name> 0x<addr> 0x<size> <object>"; ld wraps a long name onto the next line
SEC1 = re.compile(r"^ (\.text(?:\.\S+)?)\s+(0x[0-9a-f]+)\s+(0x[0-9a-f]+)\s+(\S.*?)\s*$", re.M)
SEC2 = re.compile(r"^ (\.text(?:\.\S+)?)\s*\n\s+(0x[0-9a-f]+)\s+(0x[0-9a-f]+)\s+(\S.*?)\s*$", re.M)


def load_map(path):
    """Sorted (start, end, function, object) ranges of the .text input sections of the map."""
    text = open(path, encoding="utf-8", errors="replace").read()
    rows = {}
    for rx in (SEC1, SEC2):
        for m in rx.finditer(text):
            sec, addr, size, obj = m.group(1), int(m.group(2), 16), int(m.group(3), 16), m.group(4)
            if size == 0:
                continue
            name = sec[len(".text."):] if sec.startswith(".text.") else "(.text of object)"
            rows[(addr, addr + size)] = (name, obj.replace("\\", "/"))
    ranges = sorted((a, b, n, o) for (a, b), (n, o) in rows.items())
    return ranges


def lookup(ranges, starts, ra):
    i = bisect.bisect_right(starts, ra) - 1
    if i >= 0 and ranges[i][0] <= ra < ranges[i][1]:
        return ranges[i][2], ranges[i][3]
    return "(unmapped)", "(unmapped)"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("log")
    ap.add_argument("--map", default=os.path.join("build3ds", "lane-NAPI0", "pm_3ds.map"))
    ap.add_argument("--label", default="")
    a = ap.parse_args()
    label = a.label or os.path.splitext(os.path.basename(a.log))[0]
    if not os.path.exists(a.log):
        print("no such log: %s" % a.log)
        return 2
    if not os.path.exists(a.map):
        print("no such map: %s" % a.map)
        return 2
    ranges = load_map(a.map)
    starts = [r[0] for r in ranges]

    windows = {}  # frame -> list of (ra, n)
    totals = {}
    for line in open(a.log, encoding="utf-8", errors="replace"):
        m = LINE.search(line)
        if m:
            windows.setdefault(int(m.group(1)), []).append((int(m.group(2), 16), int(m.group(3))))
            continue
        m = DONE.search(line)
        if m:
            totals[int(m.group(1))] = int(m.group(2))
    if not windows:
        print("no napicen lines in %s. Nothing written." % a.log)
        return 3

    out = []
    for f in sorted(windows):
        files, funcs = {}, {}
        for ra, n in windows[f]:
            fn, obj = lookup(ranges, starts, ra)
            files[obj] = files.get(obj, 0) + n
            key = (fn, obj)
            funcs[key] = funcs.get(key, 0) + n
        out.append("window f%d total %s ops/frame (done line); callers listed %d; sum over listed %d" % (
            f, totals.get(f, "?"), len(windows[f]), sum(n for _, n in windows[f])))
        out.append("per file (ops/frame, largest first):")
        for obj, n in sorted(files.items(), key=lambda kv: (-kv[1], kv[0])):
            out.append("  %6d  %s" % (n, obj))
        out.append("per function (ops/frame, largest first):")
        for (fn, obj), n in sorted(funcs.items(), key=lambda kv: (-kv[1], kv[0])):
            out.append("  %6d  %s  %s" % (n, fn, obj))
        out.append("")

    dst_dir = os.path.join("build3ds", "out")
    os.makedirs(dst_dir, exist_ok=True)
    dst = os.path.join(dst_dir, "napicen_%s.txt" % label)
    with open(dst, "w", encoding="utf-8") as fh:
        fh.write("\n".join(out) + "\n")
    print("wrote %s (%d windows)" % (dst, len(windows)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
