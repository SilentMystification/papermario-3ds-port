#!/usr/bin/env python3
"""route_report.py - per-segment table of a route run (switch route=<name>), from an Azahar log or a log pulled from the console.
  route_report.py LOG [--title TEXT]      table per segment
  route_report.py LOG --compare LOG2      statehash check: every `statehash f<n>` line of both logs must be equal, and the seg lines at the same steps
Needs the switches: route=<name> phase phasecsv loglevel=debug statehash=300 fixedseed (phsum adds the phs lines, used when no pc lines exist).
Segments come from the lines `route: seg <name> f<step>` and the last line `route: end f<step>`. The part before the first seg line is named `pre`.
Units: the pc line holds 0.1 ms. cpu = w - i - fb = work of one step without the idle wait and without the FrameBegin wait. w = wall time of one drawn step.
On the console w is real time (paced to 33.3 ms by 2 vblanks), so `wall>36` counts the steps the player sees late. In Azahar w is host time and cpu is guest time.
Phase columns are the average ms per step of the pc fields: lo dl gb gp tx au ro y fb fe ot gc gd (same names as phase_report.py).
Counter columns are the average per step of the values after the `|`: tri draw flush texmiss (texmiss and flush are sums too: texmissT, flushT)."""
import re, sys, collections

PC = re.compile(r"^#\d+ pc (\d+) w (-?\d+) i (-?\d+) lo (-?\d+) dl (-?\d+) gb (-?\d+) gp (-?\d+) tx (-?\d+) au (-?\d+) ro (-?\d+) y (-?\d+) fb (-?\d+) fe (-?\d+) ot (-?\d+) gc (-?\d+) gd (-?\d+).*?\| ([\d ]+)$")
PHN = "lo dl gb gp tx au ro y fb fe ot gc gd".split()
CN = "tri triout vtxload vtxslot draw flush texcall texmiss texKB romn romKB yayn yayKB dbgcall".split()
SEG = re.compile(r"route: seg (\S+) f(\d+)")
END = re.compile(r"route: end f(\d+)")
ST = re.compile(r"route: st f(\d+) mode (\d+) map (\d+):(\d+)")
GOTO = re.compile(r"route: goto (\S+) f(\d+)")
SAVE = re.compile(r"route: save slot (\d+) f(\d+) took (\d+) ms")
SH = re.compile(r"statehash f(\d+) (\w+) run (\w+) n (\d+) norand (\w+)")
PHS = re.compile(r"^#\d+ phs (-?\d+) (-?\d+):(-?\d+) (\w+) n (\d+) f(\d+)-(\d+) \| ([\d ]+) \| (\d+) (\d+)")
PHS_NAMES = "lo dl gb gp gc gd tx au ro y fb fe ot gv gt gm go fr gtev gbind gst".split()

def parse(path):
    d = dict(pc={}, seg=[], end=None, st=[], goto=[], save=[], sh={}, phs=[])
    for ln in open(path, errors="replace"):
        if "route:" in ln:
            m = SEG.search(ln)
            if m: d["seg"].append((m.group(1), int(m.group(2)))); continue
            m = END.search(ln)
            if m: d["end"] = int(m.group(1)); continue
            m = ST.search(ln)
            if m: d["st"].append(tuple(int(x) for x in m.groups())); continue
            m = GOTO.search(ln)
            if m: d["goto"].append((m.group(1), int(m.group(2)))); continue
            m = SAVE.search(ln)
            if m: d["save"].append((int(m.group(1)), int(m.group(2)), int(m.group(3)))); continue
        elif " pc " in ln:
            m = PC.match(ln.rstrip())
            if m:
                g = m.groups(); f = int(g[0])
                d["pc"][f] = dict(w=int(g[1]), i=int(g[2]), fb=int(g[11]), ph=dict(zip(PHN, [int(x) for x in (g[3:11] + g[11:16])])), c=dict(zip(CN, [int(x) for x in g[16].split()])))
            continue
        elif "statehash f" in ln:
            m = SH.search(ln)
            if m: d["sh"][int(m.group(1))] = (m.group(2), m.group(3), m.group(5)); continue
        elif " phs " in ln:
            m = PHS.match(ln)
            if m: d["phs"].append(dict(f0=int(m.group(6)), f1=int(m.group(7)), n=int(m.group(5)), ph=dict(zip(PHS_NAMES, [int(x) for x in m.group(8).split()])), p50=int(m.group(9)), p95=int(m.group(10))))
    return d

def pctl(v, p):
    if not v: return 0.0
    v = sorted(v); return v[min(len(v) - 1, int(len(v) * p))]

def bounds(d):
    last = d["end"] if d["end"] is not None else (max(d["pc"]) if d["pc"] else (d["seg"][-1][1] if d["seg"] else 0))
    segs = d["seg"]
    out = []
    if segs and segs[0][1] > 1 and d["pc"] and min(d["pc"]) < segs[0][1]: out.append(("pre", min(d["pc"]), segs[0][1]))
    for i, (n, f) in enumerate(segs):
        out.append((n, f, segs[i + 1][1] if i + 1 < len(segs) else last))
    return out

def seg_map(d, a, b):
    """game mode and map of the route: st lines inside [a, b): the modes seen and the first map"""
    xs = [s for s in d["st"] if a <= s[0] < b]
    if not xs: return "-"
    maps = []
    for s in xs:
        k = "%d:%d" % (s[2], s[3])
        if k not in maps: maps.append(k)
    modes = sorted(set(s[1] for s in xs))
    return "m%s map %s" % ("/".join(str(x) for x in modes), ",".join(maps[:3]))

def table(d, title):
    out = [title, "cpu = w - i - fb of a drawn step, ms (0.1 ms units / 10). wall = w. over33 = steps with cpu over 33.3 ms. wall36 = steps with w over 36 ms.", ""]
    hdr = ["segment", "f0", "steps", "cpu_avg", "cpu_p95", "cpu_max", "wall_avg", "wall_p95", "wall_max", "over33", "wall36", "texmissT", "flushT", "tri/st", "draw/st", "map"]
    rows = []
    pcs = d["pc"]
    for n, a, b in bounds(d):
        fr = [f for f in range(a, b) if f in pcs]
        if not fr:
            rows.append([n, a, b - a] + ["-"] * 12 + [seg_map(d, a, b)]); continue
        cpu = [pcs[f]["w"] - pcs[f]["i"] - pcs[f]["fb"] for f in fr]
        wl = [pcs[f]["w"] for f in fr]
        k = len(fr)
        rows.append([n, a, k, "%.1f" % (sum(cpu) / k / 10), "%.1f" % (pctl(cpu, .95) / 10), "%.1f" % (max(cpu) / 10),
                     "%.1f" % (sum(wl) / k / 10), "%.1f" % (pctl(wl, .95) / 10), "%.1f" % (max(wl) / 10),
                     sum(1 for x in cpu if x > 333), sum(1 for x in wl if x > 360),
                     sum(pcs[f]["c"].get("texmiss", 0) for f in fr), sum(pcs[f]["c"].get("flush", 0) for f in fr),
                     "%.0f" % (sum(pcs[f]["c"].get("tri", 0) for f in fr) / k), "%.1f" % (sum(pcs[f]["c"].get("draw", 0) for f in fr) / k), seg_map(d, a, b)])
    out += fmt(hdr, rows)
    out.append("")
    hdr2 = ["segment"] + PHN
    rows2 = []
    for n, a, b in bounds(d):
        fr = [f for f in range(a, b) if f in pcs]
        if fr: rows2.append([n] + ["%.2f" % (sum(pcs[f]["ph"][p] for f in fr) / len(fr) / 10) for p in PHN])
    if rows2:
        out.append("phase columns: average ms per drawn step (lo dl gb gp tx au ro y fb fe ot gc gd as in phase_report.py)")
        out += fmt(hdr2, rows2)
    elif d["phs"]:
        out.append("no pc lines (add phasecsv). phs windows per segment (phsum): steps, average ms per step of the phase sums, worst p95 of a window in ms")
        rows3 = []
        for n, a, b in bounds(d):
            ws = [w for w in d["phs"] if a <= w["f0"] < b]
            k = sum(w["n"] for w in ws)
            if k: rows3.append([n, k, "%.1f" % (max(w["p95"] for w in ws) / 10)] + ["%.2f" % (sum(w["ph"][p] for w in ws) / k / 10) for p in PHN if p in PHS_NAMES])
        out += fmt(["segment", "steps", "p95max"] + [p for p in PHN if p in PHS_NAMES], rows3)
    else:
        out.append("no pc lines and no phs lines: run with phase phasecsv (or phase phsum) loglevel=debug")
    out.append("")
    if d["goto"]: out.append("goto (map change by token): " + ", ".join("%s f%d" % g for g in d["goto"]))
    for s in d["save"]: out.append("save: slot %d at f%d took %d ms" % s)
    sh = sorted(d["sh"].items())
    if sh:
        out.append("statehash (every 300 steps): " + " ".join("f%d=%s" % (f, v[0]) for f, v in sh))
        out.append("final run hash: " + sh[-1][1][1])
    out.append("route end: f%s, %d seg lines, %d pc lines" % (d["end"], len(d["seg"]), len(pcs)))
    return out

def fmt(hdr, rows):
    w = [max(len(str(x)) for x in [h] + [r[i] for r in rows]) for i, h in enumerate(hdr)]
    out = ["  ".join(str(h).ljust(w[i]) if i in (0, len(hdr) - 1) else str(h).rjust(w[i]) for i, h in enumerate(hdr))]
    for r in rows: out.append("  ".join(str(x).ljust(w[i]) if i in (0, len(hdr) - 1) else str(x).rjust(w[i]) for i, x in enumerate(r)))
    return out

def compare(a, b):
    out, bad = [], 0
    common = sorted(set(a["sh"]) & set(b["sh"]))
    nr = 0
    for f in common:
        if a["sh"][f][0] != b["sh"][f][0]: bad += 1; out.append("DIFF f%d %s vs %s" % (f, a["sh"][f][0], b["sh"][f][0]))
        if a["sh"][f][2] != b["sh"][f][2]: nr += 1
    out.append("statehash: %d common samples, %d differ, %d differ without the random seed (norand), run hash %s vs %s" % (len(common), bad, nr, (sorted(a["sh"].items()) or [(0, ("", ""))])[-1][1][1], (sorted(b["sh"].items()) or [(0, ("", ""))])[-1][1][1]))
    if bad and not nr: out.append("note: only the random seed differs (a skipped draw does not run the effect draw code that calls the random number generator). The game state is equal.")
    bad = nr
    if a["seg"] != b["seg"]: out.append("DIFF seg lines: %s vs %s" % (a["seg"], b["seg"])); bad += 1
    else: out.append("seg lines equal (%d)" % len(a["seg"]))
    out.append("route end: %s vs %s" % (a["end"], b["end"]))
    out.append("RESULT: " + ("equal" if bad == 0 and common else "NOT EQUAL or no samples"))
    return out

if __name__ == "__main__":
    args = sys.argv[1:]
    if not args: sys.exit(__doc__)
    d = parse(args[0])
    if "--compare" in args:
        print("\n".join(compare(d, parse(args[args.index("--compare") + 1]))))
    else:
        title = args[args.index("--title") + 1] if "--title" in args else "route table of " + args[0]
        print("\n".join(table(d, title)))
