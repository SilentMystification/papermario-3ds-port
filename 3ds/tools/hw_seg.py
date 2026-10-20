"""hw_seg.py - per-segment table of one full cycle from ONE log (O3DS live log, FTP-pulled log.txt or an Azahar log).
Usage: python3 3ds/tools/hw_seg.py <log.txt> ["<header>"]
Same segments and same columns as perf_az_full.py (it reuses its parse, segments and stats functions).
Added columns for hardware: wall_p99_ms and steps_wall_over_50ms. On hardware the pc column w is REAL time per drawn step
(paced to 33.3 ms by two vblanks), so w over 50 ms is a late step the user can see. In Azahar w is fixed, so these two stay at 33.3 and 0.
Needs the switches: phase phasecsv loglevel=debug (pc lines and game mode lines). Add audiostat for audio_posts. A pulled log.txt has every line, a live log can drop lines."""
import sys, os
src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "perf_az_full.py")).read()
exec(compile(src[:src.index("build, head, pc_")], "perf_az_full.py", "exec"))
d = parse(sys.argv[1])
if not d[0] or not d[1]:
    sys.exit("no pc lines or no game mode lines: run with phase phasecsv loglevel=debug")
segs = segments(d[1], max(d[0]))
cols = ["frames", "audio_posts", "steps_over_33ms", "cpu_p99_ms", "cpu_max_ms", "gbi_ms", "audio_ms", "wall_p99_ms", "steps_wall_over_50ms", "linear_peak_KB", "tex_fail", "vram_free_min_KB"]
def ext(a, b):
    s = stats(d, a, b)
    w = sorted(d[0][f]["wall"] for f in range(a, b) if f in d[0])
    s["wall_p99_ms"] = (w[min(len(w) - 1, int(len(w) * 0.99))] / 10.0) if w else 0.0
    s["steps_wall_over_50ms"] = sum(1 for x in w if x > 500)
    return s
print(sys.argv[2] if len(sys.argv) > 2 else "per-segment table of " + sys.argv[1])
print("cpu = wall - idle - fbegin wait of one step. gbi_ms, audio_ms: per-step average. ms are real time on hardware.")
print("%-28s" % "segment" + "".join("%12s" % c[:11] for c in cols))
rows = [(n, ext(a, b)) for n, a, b in segs] + [("WINDOW pc 1700-1800", ext(1700, 1800)), ("TOTAL", ext(1, max(d[0]) + 1))]
for n, s in rows:
    print("%-28s" % n[:27] + "".join("%12s" % (("%.2f" % s[c]) if isinstance(s[c], float) else s[c]) for c in cols))
