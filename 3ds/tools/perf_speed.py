"""perf_speed.py - emulated speed per segment of one full Azahar cycle.
Usage: python3 3ds/tools/perf_speed.py <log> [<ref log>]     (log from -F -d "... phase phasecsv ... cpulimit=80")
fps = frames / guest seconds. Guest time is the emulated ARM11 clock (sum of the `w` column of the pc lines), so it follows emulated CPU cost
and not the host load. With a second log the change in percent against it is printed (positive = faster than the reference).
Columns: gbi ms and audio ms are per frame averages (guest time)."""
import re, sys
PC = re.compile(r"^#\d+ pc (\d+) w (\d+) i \d+ lo (\d+) dl (\d+) gb (\d+) gp (\d+) tx (\d+) au (\d+) ")
GM = re.compile(r"^#\d+ game mode (\d+) frame (\d+)")
def load(p):
    pcs, modes = {}, []
    for l in open(p, errors="replace"):
        m = PC.match(l)
        if m: g = list(map(int, m.groups())); pcs[g[0]] = g[1:]; continue
        m = GM.match(l)
        if m: modes.append((int(m.group(1)), int(m.group(2))))
    first2 = next(f for m, f in modes if m == 2)
    cur = [("intro", 0), ("title", first2)]; k = 0
    for i, (m, f) in enumerate(modes):
        if f <= first2: continue
        if m == 17 and i + 1 < len(modes) and modes[i + 1][0] in (3, 8):
            k += 1; cur.append(("demo%d %s" % (k, "world" if modes[i + 1][0] == 3 else "battle"), f))
        if m == 2: cur.append(("END", f)); break
    segs = []
    for (n, a), (_, b) in zip(cur, cur[1:]):
        fr = [pcs[f] for f in range(a, b) if f in pcs]
        w = sum(x[0] for x in fr) / 10000.0
        segs.append((n, len(fr), w, len(fr) / w if w else 0, sum(x[3] for x in fr) / 10.0 / max(1, len(fr)), sum(x[6] for x in fr) / 10.0 / max(1, len(fr))))
    return segs
a = load(sys.argv[1]); r = load(sys.argv[2]) if len(sys.argv) > 2 else None
tf = sum(s[1] for s in a); tw = sum(s[2] for s in a)
print("%-16s %6s %9s %8s %8s %8s %s" % ("segment", "frames", "guest s", "fps", "gbi ms", "aud ms", "change vs ref" if r else ""))
for i, s in enumerate(a):
    ch = ""
    if r and i < len(r) and r[i][3]: ch = "%+.1f%%" % ((s[3] - r[i][3]) * 100 / r[i][3])
    print("%-16s %6d %9.2f %8.2f %8.2f %8.2f %s" % (s[0], s[1], s[2], s[3], s[4], s[5], ch))
ch = ""
if r:
    rf = sum(x[1] for x in r); rw = sum(x[2] for x in r); ch = "%+.1f%%" % ((tf / tw - rf / rw) * 100 / (rf / rw))
print("%-16s %6d %9.2f %8.2f %8s %8s %s" % ("TOTAL", tf, tw, tf / tw, "", "", ch))
