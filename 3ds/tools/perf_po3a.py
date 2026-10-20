"""perf_po3a.py - PO3a table: matrix ids, mixed-id triangles and override cases per segment of one full Azahar cycle.
Usage: python3 3ds/tools/perf_po3a.py <log with switch po3a> "<header>" > 3ds/logs/perf/po3a_azahar_<build>.txt
Segments as in perf_az_full.py (game mode lines). Needs the pc lines (switch phase phasecsv) for draws per frame."""
import re, sys
PO = re.compile(r"^#\d+ po3a f(\d+) frames (\d+) cmd (\d+) drawn (\d+) mixed (\d+) ids (\d+) idmax (\d+) vcmd (\d+) vload (\d+) lit (\d+) tgen (\d+) scr (\d+) primz (\d+) env (\d+) plan (\d+) a255 (\d+) fog (\d+) boost (\d+) plain (\d+) plain2 (\d+)")
KEYS = "frames cmd drawn mixed ids idmax vcmd vload lit tgen scr primz env plan a255 fog boost plain plain2".split()
GM = re.compile(r"^#\d+ game mode (\d+) frame (\d+)")
PC = re.compile(r"^#\d+ pc (\d+) .*\| (\d+) (\d+) (\d+) ")
path, head = sys.argv[1], sys.argv[2]
po, modes, draws = [], [], {}
for l in open(path, errors="replace"):
    m = PO.match(l)
    if m: po.append((int(m.group(1)), dict(zip(KEYS, map(int, m.groups()[1:]))))); continue
    m = GM.match(l)
    if m: modes.append((int(m.group(1)), int(m.group(2)))); continue
    m = PC.match(l)
    if m: draws[int(m.group(1))] = int(m.group(4))
first2 = next(f for m, f in modes if m == 2)
cur = [("intro (boot, logos, intro)", 0), ("title", first2)]; k = 0
for i, (m, f) in enumerate(modes):
    if f <= first2: continue
    if m == 17 and i + 1 < len(modes) and modes[i + 1][0] in (3, 8):
        k += 1; cur.append(("demo %d %s" % (k, "world" if modes[i + 1][0] == 3 else "battle"), f))
    if m == 2: cur.append(("END", f)); break
segs = [(n, a, b) for (n, a), (_, b) in zip(cur, cur[1:])]
def pc(a, b): return 100.0 * a / b if b else 0.0
out = [head, "PO3a counters. Window = 10 frames, a window belongs to the segment of its last frame. Matrix id = FNV-1a of the combined matrix at each G_VTX.",
       "ids/frame = distinct matrix ids that were current at a G_VTX of the frame. draws/frame = batches the CPU path sends today (pc line).",
       "mixed = triangle commands whose three vertices come from different matrix ids. plain = drawn triangle with one id, unlit, no texgen, no screen vertex, no prim z and no colour override (alpha 255 counted as an override). plain2 = same but alpha 255 allowed.", ""]
tot = {k: 0 for k in KEYS}; tot["draws"] = 0; tot["dn"] = 0
hdr = "%-26s %6s %8s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s"
out.append(hdr % ("segment", "frames", "cmd/fr", "drawn%", "mixed%", "ids/fr", "idmax", "draw/fr", "lit%", "tgen%", "primz%", "env%", "a255%", "plain%"))
rows = []
for n, a, b in segs:
    s = {k: 0 for k in KEYS}
    for f, d in po:
        if a <= f < b:
            for k in KEYS:
                if k == "idmax": s[k] = max(s[k], d[k])
                else: s[k] += d[k]
    fr = [draws[f] for f in range(a, b) if f in draws]
    dn, dsum = len(fr), sum(fr)
    for k in KEYS: tot[k] = max(tot[k], s[k]) if k == "idmax" else tot[k] + s[k]
    tot["draws"] += dsum; tot["dn"] += dn
    w = max(1, s["frames"])
    rows.append((n, s, w, dsum, dn))
    out.append(hdr % (n[:26], s["frames"], "%.0f" % (s["cmd"] / w), "%.1f" % pc(s["drawn"], s["cmd"]), "%.1f" % pc(s["mixed"], s["cmd"]), "%.1f" % (s["ids"] / w), s["idmax"], "%.0f" % (dsum / max(1, dn)),
               "%.1f" % pc(s["lit"], s["cmd"]), "%.1f" % pc(s["tgen"], s["cmd"]), "%.1f" % pc(s["primz"], s["drawn"]), "%.1f" % pc(s["env"], s["drawn"]), "%.1f" % pc(s["a255"], s["drawn"]), "%.1f" % pc(s["plain"], s["drawn"])))
w = max(1, tot["frames"])
out.append(hdr % ("TOTAL", tot["frames"], "%.0f" % (tot["cmd"] / w), "%.1f" % pc(tot["drawn"], tot["cmd"]), "%.1f" % pc(tot["mixed"], tot["cmd"]), "%.1f" % (tot["ids"] / w), tot["idmax"], "%.0f" % (tot["draws"] / max(1, tot["dn"])),
          "%.1f" % pc(tot["lit"], tot["cmd"]), "%.1f" % pc(tot["tgen"], tot["cmd"]), "%.1f" % pc(tot["primz"], tot["drawn"]), "%.1f" % pc(tot["env"], tot["drawn"]), "%.1f" % pc(tot["a255"], tot["drawn"]), "%.1f" % pc(tot["plain"], tot["drawn"])))
out += ["", "Other totals over the cycle: vertex loads %d in %d G_VTX commands, screen vertices (scr) %d, plan colour %d (%.1f%% of drawn), fog %d, boost %d, plain2 %.1f%% of drawn." %
        (tot["vload"], tot["vcmd"], tot["scr"], tot["plan"], pc(tot["plan"], tot["drawn"]), tot["fog"], tot["boost"], pc(tot["plain2"], tot["drawn"]))]
print("\n".join(out))
