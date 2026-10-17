#!/usr/bin/env python3
"""R01: classify the ROM reads of a romtrace file (lines `addr size fnv`, switch romtrace) by splat segment.
Usage (repo root): python 3ds/tools/rom_trace_classify.py trace.txt [more.txt ...]
Prints reads per segment (innermost splat segment with a known start). Flags:
  TINY   offset below 0x1000
  CODE   read inside a top level `code` segment that is not a .data/.rodata part (a data read there is a placeholder suspect)
  ODD    odd start offset (a placeholder 1 plus an offset ends odd)
Dropped ranges are checked by the game itself (ROM READ IN DROPPED RANGE)."""
import sys, yaml, bisect
Y = yaml.load(open('ver/us/splat.yaml'), Loader=yaml.CSafeLoader)
segs = []  # (start, name, type, top_name, top_type)
def walk(lst, top):
    for s in lst:
        if isinstance(s, dict):
            st, nm, ty = s.get('start'), s.get('name') or s.get('dir'), s.get('type', '?')
            t = top or (str(nm), ty)
            if isinstance(st, int) and nm: segs.append((st, str(nm), ty, t[0], t[1]))
            if 'subsegments' in s: walk(s['subsegments'], t)
        elif isinstance(s, list) and s and isinstance(s[0], int) and len(s) > 2 and isinstance(s[2], str):
            segs.append((s[0], s[2], s[1], top[0] if top else '?', top[1] if top else '?'))
walk(Y['segments'], None)
segs.sort(key=lambda x: x[0])
keys = [s[0] for s in segs]
rows = {}
for f in sys.argv[1:]:
    for l in open(f):
        w = l.split()
        if len(w) != 3: continue  # last buffered line can be cut
        a, n = w[0], w[1]
        a, n = int(a, 16), int(n)
        i = bisect.bisect_right(keys, a) - 1
        s = segs[i] if i >= 0 else (0, '?', '?', '?', '?')
        flags = ''
        if a < 0x1000: flags += 'TINY '
        if s[4] == 'code' and s[2] in ('code', 'c', 'asm', 'hasm') and s[0] == next(x[0] for x in segs if x[3] == s[3]): flags += 'CODE '
        if a & 1: flags += 'ODD '
        r = rows.setdefault((s[1], s[2], s[3], flags), [0, 0, a])
        r[0] += 1; r[1] += n; r[2] = min(r[2], a)
for (nm, ty, top, fl), (c, b, a) in sorted(rows.items(), key=lambda x: x[1][2]):
    print('%-6s %-34s %-12s top %-22s reads %5d bytes %9d first %08x' % (fl.strip() or '-', nm, ty, top, c, b, a))
