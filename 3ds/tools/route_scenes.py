#!/usr/bin/env python3
"""route_scenes.py - one line per scene of a `tour=` run (scene = a map entered by the tour switch).
  python 3ds/tools/route_scenes.py LOG [--settle 20] [--shots SHOTDIR]

Scene start: `tour: in map area a map m entry e frame f t=<ms>`. Load wall ms = that t minus the t of the
preceding `tour: next` line.
Burst lines (switch shotmode=4:21, one per frame from the change to the world mode, in pm_gpu.c dump_shot):
  `shot <n> frame <f> map a:m t=<ms> cmd <C_CMD> flush <C_FLUSH>`
Steady frame ms = (t of last burst frame - t of first) / (frames between them), wall clock, no pc lines.
ops = sum of cmd over the burst frames (C_CMD, command count of the frame before). closes = sum of flush (C_FLUSH).
These counters are only filled with the switch `phase`. Without it they are 0.
shot = the i-th file of SHOTDIR (sorted, <n>_<a>_<m>.ppm) for scene i, checked against the map (when --shots is given).
sdrd = count of `sdrd f<n>` lines (phase, SD streaming) with f in f..f+settle."""
import re, sys, os, glob

NEXT = re.compile(r"tour: next (\S+) entry (-?\d+) \(area (\d+) map (\d+)\) t=(\d+) ms")
IN = re.compile(r"tour: in map area (\d+) map (\d+) entry (-?\d+) frame (\d+) t=(\d+) ms")
BURST = re.compile(r"^#\d+ shot (\d+) frame (\d+) map (\d+):(\d+) t=(\d+) ms cmd (\d+) flush (\d+)$")
SDRD = re.compile(r"sdrd f(\d+) ")
END = re.compile(r"tour: done")

def main(argv):
    if not argv: print(__doc__); return 2
    path = argv[0]
    settle = int(argv[argv.index("--settle") + 1]) if "--settle" in argv else 20
    shotdir = argv[argv.index("--shots") + 1] if "--shots" in argv else None
    pend, scenes, burst, sdrd, end = None, [], [], [], False
    for ln in open(path, errors="replace"):
        if "tour:" in ln:
            m = NEXT.search(ln)
            if m: pend = (m.group(1), int(m.group(5))); continue
            m = IN.search(ln)
            if m:
                name = pend[0] if pend else "warp"
                t0 = pend[1] if pend else None
                scenes.append(dict(name=name, area=int(m.group(1)), map=int(m.group(2)), f=int(m.group(4)), t_in=int(m.group(5)), t0=t0))
                pend = None
                continue
            if END.search(ln): end = True
        elif "shot " in ln:
            m = BURST.match(ln.rstrip())
            if m: burst.append(tuple(int(x) for x in m.groups()))
        elif "sdrd f" in ln:
            m = SDRD.search(ln)
            if m: sdrd.append(int(m.group(1)))
    print("scene  map      area:map  frame   load_ms  steady_ms  ops    closes  sdrd  shot")
    for i, s in enumerate(scenes):
        f = s["f"]
        rows = [b for b in burst if f <= b[1] <= f + settle and (b[2], b[3]) == (s["area"], s["map"])]
        rows.sort(key=lambda b: b[1])
        if len(rows) >= 2 and rows[-1][1] > rows[0][1]:
            steady = "%.1f" % ((rows[-1][4] - rows[0][4]) / float(rows[-1][1] - rows[0][1]))
        else:
            steady = "n/a"
        ops = sum(b[5] for b in rows) if rows else "n/a"
        closes = sum(b[6] for b in rows) if rows else "n/a"
        load = "%d" % (s["t_in"] - s["t0"]) if s["t0"] is not None else "n/a"
        sd = sum(1 for x in sdrd if f <= x <= f + settle)
        shot = "-"
        if rows:
            shot = "burst %d frames" % len(rows)
            if shotdir:
                # one file per burst, numbered in order: scene i takes the i-th file (names carry area_map, not the frame)
                files = sorted(os.path.basename(x) for x in glob.glob(os.path.join(shotdir, "*.ppm")))
                ok = i < len(files) and files[i].endswith("_%d_%d.ppm" % (s["area"], s["map"]))
                shot = files[i] if ok else "NO FILE / MAP MISMATCH"
        print("%-6d %-8s %-9s %-7d %-8s %-10s %-6s %-7s %-5d %s" % (i, s["name"], "%d:%d" % (s["area"], s["map"]), f, load, steady, ops, closes, sd, shot))
    print("tour done: %s, scenes %d, burst lines %d" % ("yes" if end else "no", len(scenes), len(burst)))
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
