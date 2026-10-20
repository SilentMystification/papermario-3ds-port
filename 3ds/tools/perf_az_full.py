"""perf_az_full.py - per-segment table for one full boot + intro + title + demo cycle in Azahar.
Usage: python3 3ds/tools/perf_az_full.py <build> "<header>" <cur_fast.txt> <pre_fast.txt> <cur_normal.txt> > baseline_azahar_<build>.txt
Segments come from the `game mode N frame F` log lines: intro = frame 0 to the first mode 2 (title), title = mode 2 to the first
mode 17 that is followed by a demo scene, demo k = mode 17 -> mode 3 (world) or 8 (battle) up to the next mode 17 or the closing mode 2.
Counters (per-frame columns after the `|` of the pc line, memory, audio posts) do not depend on host speed. ms values are host time."""
import re, sys

PC = re.compile(r"^#\d+ pc (\d+) w (\d+) i (\d+) lo (\d+) dl (\d+) gb (\d+) gp (\d+) tx (\d+) au (\d+) ro (\d+) y (\d+) fb (\d+) fe (\d+) ot (\d+) gc \d+ gd (\d+) .*\| (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)")
GM = re.compile(r"^#\d+ game mode (\d+) frame (\d+)")
CNT = ["triout", "vtxslot", "draws", "texmiss", "texKB", "romn", "romB"]

def parse(path):
    pcs, modes, mem, posts, b7, texf = {}, [], [], [], [], []
    for l in open(path, errors="replace"):
        m = PC.match(l)
        if m:
            g = list(map(int, m.groups())); f = g[0]
            pcs[f] = dict(wall=g[1], idle=g[2], gbi=g[5], au=g[8], fb=g[11], c=g[15:22]); continue
        m = GM.match(l)
        if m: modes.append((int(m.group(1)), int(m.group(2)))); continue
        fr = max(pcs) if pcs else 0
        m = re.search(r"mem map linear used (\d+)KB peak (\d+)KB .*texture failures (\d+)", l)
        if m: mem.append((fr, int(m.group(2)), int(m.group(3)))); continue
        m = re.search(r"vram free (\d+)KB", l)
        if m: mem.append((fr, "v", int(m.group(1)))); continue
        m = re.search(r"audio posts (\d+) peak (\d+)", l)
        if m: posts.append((fr, int(m.group(1)), int(m.group(2)))); continue
        m = re.search(r"b7 f(\d+): grabs (\d+) \(max (\d+) per frame\) splits (\d+) \(max (\d+)\)", l)
        if m: b7.append((int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)), int(m.group(5))))
    return pcs, modes, mem, posts, b7

def segments(modes, last):
    segs = []; first2 = next(f for m, f in modes if m == 2)
    segs.append(("intro (boot, logos, intro)", 0, first2))
    cur = [("title", first2)]; k = 0
    for i, (m, f) in enumerate(modes):
        if f <= first2: continue
        if m == 17 and i + 1 < len(modes) and modes[i + 1][0] in (3, 8):
            cur.append(("demo %d %s" % (k + 1, "world" if modes[i + 1][0] == 3 else "battle"), f)); k += 1
        if m == 2: cur.append(("END", f)); break
    if cur[-1][0] != "END": cur.append(("END", last))
    for (n, a), (_, b) in zip(cur, cur[1:]): segs.append((n, a, b))
    return segs

def stats(d, a, b):
    pcs, modes, mem, posts, b7 = d
    fr = [f for f in range(a, b) if f in pcs]
    s = {"frames": len(fr)}
    for i, n in enumerate(CNT): s[n] = sum(pcs[f]["c"][i] for f in fr)
    cpu = sorted(pcs[f]["wall"] - pcs[f]["idle"] - pcs[f]["fb"] for f in fr) # DIP metric: cpu work of the step in 0.1 ms (wall - idle - fbegin wait)
    s["steps_over_33ms"] = sum(1 for c in cpu if c > 333)
    s["cpu_p99_ms"] = (cpu[min(len(cpu) - 1, int(len(cpu) * 0.99))] / 10.0) if cpu else 0.0
    s["cpu_max_ms"] = (cpu[-1] / 10.0) if cpu else 0.0
    s["wall_s"] = sum(pcs[f]["wall"] for f in fr) / 10000.0
    s["gbi_ms"] = sum(pcs[f]["gbi"] for f in fr) / 10.0 / max(1, len(fr))
    s["audio_ms"] = sum(pcs[f]["au"] for f in fr) / 10.0 / max(1, len(fr))
    pa = [p for p in posts if p[0] < a]; pb = [p for p in posts if p[0] < b]
    s["audio_posts"] = (pb[-1][1] if pb else 0) - (pa[-1][1] if pa else 0)
    ms = [m for m in mem if a <= m[0] < b]
    s["linear_peak_KB"] = max([m[1] for m in ms if m[1] != "v"] or [0])
    s["tex_fail"] = max([m[2] for m in ms if m[1] != "v"] or [0])
    s["vram_free_min_KB"] = min([m[2] for m in ms if m[1] == "v"] or [0])
    bs = [x for x in b7 if a <= x[0] < b]
    s["grabs_max"] = max([x[2] for x in bs] or [0]); s["splits_max"] = max([x[4] for x in bs] or [0])
    return s

def pct(a, b):
    if a == 0 and b == 0: return "0.0%"
    if a == 0: return "n/a"
    return "%+.1f%%" % ((b - a) * 100.0 / a)

build, head, pc_, pp_, pn_ = sys.argv[1:6]
cur, pre, nor = parse(pc_), parse(pp_), parse(pn_)
out = [head,
       "One full cycle: boot, logos, intro, title, all demo scenes, back to title (end marker: 2nd log line 'game mode 2 frame').",
       "Rows without 'host time' are counters and do not depend on host speed. 'host time' rows are ms or s on this PC, not O3DS.", ""]
segs = segments(cur[1], max(cur[0]))
tot = [0, 0]
out.append("=== determinism checks")
# per-frame counters: cur_fast vs pre_fast, cur_fast vs cur_normal (normal speed run ends at the 1st title)
def cmp(x, y, upto, label):
    bad = {n: 0 for n in CNT}; n = 0
    for f in range(1, upto):
        if f in x[0] and f in y[0]:
            n += 1
            for i, nm in enumerate(CNT):
                if x[0][f]["c"][i] != y[0][f]["c"][i]: bad[nm] += 1
    out.append("%s: %d frames compared, frames with a different value per counter: %s" % (label, n, ", ".join("%s %d" % (k, v) for k, v in bad.items())))
first2 = segs[0][2]
cmp(cur, nor, min(first2, max(nor[0])), "current fast vs current normal speed, frames 1..%d" % min(first2, max(nor[0])))
cmp(cur, pre, min(max(cur[0]), max(pre[0])), "current vs pre-PO2, whole cycle (a difference here is a change of the build, not noise)")
out.append("romn and romB (ROM reads) differ between runs of the same build and between fast and normal speed (async stream timing). They are dropped from the tables.")
out.append("A 0 in linear_peak_KB or vram_free_min_KB means the segment has no mem log line (mem lines print at map loads). A vram 0 in intro is the boot line before the first map.")
out.append("")
tc = [stats(cur, a, b) for _, a, b in segs]; tp = [stats(pre, a, b) for _, a, b in segs]
rows = ["frames"] + [c for c in CNT if c not in ("romn", "romB")] + ["audio_posts", "linear_peak_KB", "tex_fail", "vram_free_min_KB", "grabs_max", "splits_max", "steps_over_33ms", "cpu_p99_ms", "cpu_max_ms", "wall_s", "gbi_ms", "audio_ms"]
def block(name, sc, sp):
    out.append("=== " + name)
    out.append("%-34s%16s%16s%10s" % ("metric", "pre-PO2", "current", "change"))
    for r in rows:
        lab = r + (" (host time, not O3DS)" if r in ("wall_s", "gbi_ms", "audio_ms") else "")
        if r in ("cpu_p99_ms", "cpu_max_ms", "steps_over_33ms"): lab = r + " (Azahar guest ms)"
        a, b = sp[r], sc[r]
        fmt = (lambda v: "%.2f" % v) if isinstance(a, float) or isinstance(b, float) else str
        out.append("%-34s%16s%16s%10s" % (lab, fmt(a), fmt(b), pct(a, b)))
    out.append("")
for (n, a, b), sc, sp in zip(segs, tc, tp): block("%s (frames %d to %d)" % (n, a, b), sc, sp)
# total
tot_c = {r: sum(s[r] for s in tc) for r in rows}; tot_p = {r: sum(s[r] for s in tp) for r in rows}
for r in ("linear_peak_KB", "tex_fail", "grabs_max", "splits_max", "cpu_p99_ms", "cpu_max_ms"): tot_c[r] = max(s[r] for s in tc); tot_p[r] = max(s[r] for s in tp)
tot_c["vram_free_min_KB"] = min(s["vram_free_min_KB"] for s in tc); tot_p["vram_free_min_KB"] = min(s["vram_free_min_KB"] for s in tp)
tot_c["gbi_ms"] = sum(s["gbi_ms"] * s["frames"] for s in tc) / max(1, tot_c["frames"]); tot_p["gbi_ms"] = sum(s["gbi_ms"] * s["frames"] for s in tp) / max(1, tot_p["frames"])
tot_c["audio_ms"] = sum(s["audio_ms"] * s["frames"] for s in tc) / max(1, tot_c["frames"]); tot_p["audio_ms"] = sum(s["audio_ms"] * s["frames"] for s in tp) / max(1, tot_p["frames"])
block("TOTAL (whole cycle; gbi_ms and audio_ms are per-frame averages)", tot_c, tot_p)
for lab, d in (("pre-PO2", pre), ("current", cur)):
    po = [l for l in open(pc_ if lab == "current" else pp_, errors="replace") if "PO2 f" in l]
    if lab == "current" and po: out.append("PO2 counter, last line of the current log: " + po[-1].strip())
print("\n".join(out))
