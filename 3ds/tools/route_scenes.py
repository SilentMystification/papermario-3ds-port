#!/usr/bin/env python3
"""route_scenes.py - one line per scene of a `tour=` run (scene = a map entered by the tour switch).
  python 3ds/tools/route_scenes.py LOG [--settle 20] [--shots SHOTDIR]

A scene runs from its `tour: next <map>` line (or from the log start for the warp scene) to the next `tour: next` line,
or to `tour: done`. Counts (sdrd, IN DROPPED RANGE, ops, closes) are taken over this whole scene: load and play.
Scene start: `tour: in map area a map m entry e frame f t=<ms> heap used <n> KB free <n> KB`. Load wall ms = that t minus the t of the
preceding `tour: next` line. Heap is the game heap at the scene start: used, and free = heap total minus used (KB).
Burst lines (switch shotmode=4:21, one per frame from the change to the world mode, in pm_gpu.c dump_shot):
  `shot <n> frame <f> map a:m t=<ms> cmd <C_CMD> flush <C_FLUSH>`
Steady frame ms = (t of last burst frame - t of first) / (frames between them), wall clock, no pc lines.
ops = sum of cmd over the burst frames (C_CMD, command count of the frame before). closes = sum of flush (C_FLUSH).
These counters are only filled with the switch `phase`. Without it they are 0.
shot = the i-th file of SHOTDIR (sorted, <n>_<a>_<m>.ppm) for scene i, checked against the map (when --shots is given).
sdrd = count of `sdrd f<n>` lines (phase, SD streaming) in the scene.
drop = count of `IN DROPPED RANGE` lines (ROM read in a dropped range, pm_os.c) in the scene."""
import re, sys, os, glob

NEXT = re.compile(r"tour: next (\S+) entry (-?\d+) \(area (\d+) map (\d+)\) t=(\d+) ms")
IN = re.compile(r"tour: in map area (\d+) map (\d+) entry (-?\d+) frame (\d+) t=(\d+) ms(?: heap used (\d+) KB free (\d+) KB)?")
BURST = re.compile(r"^#\d+ shot (\d+) frame (\d+) map (\d+):(\d+) t=(\d+) ms cmd (\d+) flush (\d+)$")
SDRD = re.compile(r"sdrd f(\d+) ")
END = re.compile(r"tour: done")

def new_scene(name, t0=None, sd=0, drop=0):
    return dict(name=name, t0=t0, sd=sd, drop=drop, area=None, map=None, f=None, t_in=None, hu=None, hf=None)

def main(argv):
    if not argv: print(__doc__); return 2
    path = argv[0]
    settle = int(argv[argv.index("--settle") + 1]) if "--settle" in argv else 20
    shotdir = argv[argv.index("--shots") + 1] if "--shots" in argv else None
    scenes, burst, end = [], [], False
    cur = None
    pre_sd = pre_drop = 0 # counts before the first tour: next line (the warp scene)
    for ln in open(path, errors="replace"):
        if "tour:" in ln:
            m = NEXT.search(ln)
            if m:
                cur = new_scene(m.group(1), t0=int(m.group(5)))
                scenes.append(cur)
                continue
            m = IN.search(ln)
            if m:
                if cur is None or cur["area"] is not None:
                    cur = new_scene("warp", sd=pre_sd, drop=pre_drop)
                    scenes.append(cur)
                cur["area"], cur["map"], cur["f"], cur["t_in"] = int(m.group(1)), int(m.group(2)), int(m.group(4)), int(m.group(5))
                if m.group(6): cur["hu"], cur["hf"] = int(m.group(6)), int(m.group(7))
                continue
            if END.search(ln): end = True
        elif "shot " in ln:
            m = BURST.match(ln.rstrip())
            if m: burst.append(tuple(int(x) for x in m.groups()))
            continue
        if "sdrd f" in ln and SDRD.search(ln):
            if cur is None: pre_sd += 1
            else: cur["sd"] += 1
        if "IN DROPPED RANGE" in ln:
            if cur is None: pre_drop += 1
            else: cur["drop"] += 1
    print("scene  map      area:map  frame   load_ms  steady_ms  ops    closes  sdrd  drop  heap_used_KB heap_free_KB  shot")
    for i, s in enumerate(scenes):
        if s["area"] is None: # a scene that never reached the world mode
            print("%-6d %-8s %-9s %s" % (i, s["name"], "n/a", "no tour: in map line"))
            continue
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
        shot = "-"
        if rows:
            shot = "burst %d frames" % len(rows)
            if shotdir:
                # one file per burst, numbered in order: scene i takes the i-th file (names carry area_map, not the frame)
                files = sorted(os.path.basename(x) for x in glob.glob(os.path.join(shotdir, "*.ppm")))
                ok = i < len(files) and files[i].endswith("_%d_%d.ppm" % (s["area"], s["map"]))
                shot = files[i] if ok else "NO FILE / MAP MISMATCH"
        hu = "%d" % s["hu"] if s["hu"] is not None else "n/a"
        hf = "%d" % s["hf"] if s["hf"] is not None else "n/a"
        print("%-6d %-8s %-9s %-7d %-8s %-10s %-6s %-7s %-5d %-5d %-12s %-12s %s" % (i, s["name"], "%d:%d" % (s["area"], s["map"]), f, load, steady, ops, closes, s["sd"], s["drop"], hu, hf, shot))
    print("tour done: %s, scenes %d, burst lines %d, sdrd total %d, drop total %d" % ("yes" if end else "no", len(scenes), len(burst), sum(s["sd"] for s in scenes) + pre_sd, sum(s["drop"] for s in scenes) + pre_drop))
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
