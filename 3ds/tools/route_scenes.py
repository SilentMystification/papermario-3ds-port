#!/usr/bin/env python3
"""route_scenes.py - one line per scene of a `tour=` run (scene = a map entered by the tour switch).
  python 3ds/tools/route_scenes.py LOG [--settle 20]

Reads the lines `tour: next <map> entry <e> (area a map m) t=<ms>` (start of the map change) and
`tour: in map area a map m entry e frame <f> t=<ms>` (map is in, mode WORLD). Load wall ms = in-map t - next t.
Steady frame ms = median of the pc lines w (0.1 ms units, /10) over the settle frames f+1 .. f+settle.
pc lines need the switch `phase phasecsv`; a scene without pc lines shows n/a.
sdrd = count of `sdrd f<n>` lines (phase, SD streaming) with f in f..f+settle; 0 when the ROM is resident.
ops and closes: no field in the game log (n/a). Shots: `shot <n> frame <f>` lines show the burst frame.
A scene shot is written as shots/<n>.ppm in the SD app folder only when the frame buffer exists (see docs/handoff/route_test.md)."""
import re, sys, statistics

NEXT = re.compile(r"tour: next (\S+) entry (-?\d+) \(area (\d+) map (\d+)\) t=(\d+) ms")
IN = re.compile(r"tour: in map area (\d+) map (\d+) entry (-?\d+) frame (\d+) t=(\d+) ms")
PC = re.compile(r"^#\d+ pc (\d+) w (-?\d+) ")
SDRD = re.compile(r"sdrd f(\d+) ")
SHOT = re.compile(r"^#\d+ shot (\d+) frame (\d+)$")
END = re.compile(r"tour: done")

def main(argv):
    if not argv: print(__doc__); return 2
    path = argv[0]
    settle = int(argv[argv.index("--settle") + 1]) if "--settle" in argv else 20
    pend, scenes, pcw, sdrd, shots, end = None, [], {}, [], {}, False
    for ln in open(path, errors="replace"):
        if "tour:" in ln:
            m = NEXT.search(ln)
            if m: pend = (m.group(1), int(m.group(5))); continue
            m = IN.search(ln)
            if m:
                name = pend[0] if pend else "%s:%s" % (m.group(1), m.group(2))
                t0 = pend[1] if pend else None
                scenes.append(dict(name=name, area=m.group(1), map=m.group(2), f=int(m.group(4)), t_in=int(m.group(5)), t0=t0))
                pend = None
                continue
            if END.search(ln): end = True
        elif " pc " in ln:
            m = PC.match(ln.rstrip())
            if m: pcw[int(m.group(1))] = int(m.group(2))
        elif "sdrd f" in ln:
            m = SDRD.search(ln)
            if m: sdrd.append(int(m.group(1)))
        elif "shot " in ln:
            m = SHOT.match(ln.rstrip())
            if m: shots[int(m.group(2))] = int(m.group(1))
    print("scene  map      area:map  frame   load_ms  steady_ms(med)  pc_n  sdrd  ops  closes  shot")
    for i, s in enumerate(scenes):
        f = s["f"]
        ws = [pcw[k] / 10.0 for k in range(f + 1, f + settle + 1) if k in pcw]
        med = "%.1f" % statistics.median(ws) if ws else "n/a"
        load = "%d" % (s["t_in"] - s["t0"]) if s["t0"] is not None else "n/a"
        sd = sum(1 for x in sdrd if f <= x <= f + settle)
        sh = "burst frame %d" % (f + settle) if (f + settle) in shots else "-"
        print("%-6d %-8s %-9s %-7d %-8s %-15s %-5d %-5d n/a  n/a     %s" % (i, s["name"], "%s:%s" % (s["area"], s["map"]), f, load, med, len(ws), sd, sh))
    print("tour done: %s, scenes %d, shots logged %d, shot files on SD: see route_test.md" % ("yes" if end else "no", len(scenes), len(shots)))
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
