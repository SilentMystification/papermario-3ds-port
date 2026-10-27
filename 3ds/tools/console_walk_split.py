"""Yard (game mode 4) per-frame averages of gbisplit/flsplit/combsplit/rsplit/dlcsplit/thsplit/taskpl. Usage: python console_walk_split.py LOG"""
import re, sys
pre = re.compile(r'^#\d+ ')
mode = -1; fr = {}
for line in open(sys.argv[1], errors='replace'):
    s = pre.sub('', line.rstrip('\n'))
    m = re.match(r'game mode (\d+) frame', s)
    if m: mode = int(m[1]); continue
    m = re.match(r'(rsplit|gbisplit|combsplit|flsplit|dlcsplit|thsplit|taskpl) f(\d+) (.*)', s)
    if not m or mode != 4: continue
    d = fr.setdefault(int(m[2]), {})
    t = m[3]
    for k, a, b, c in re.findall(r'\b([a-z0-9]+) (\d+)/(\d+)/(\d+)', t): d[m[1]+'.'+k+'.n'], d[m[1]+'.'+k+'.e'], d[m[1]+'.'+k+'.us'] = int(a), int(b), int(c)
    for k, a, b in re.findall(r'\b([a-z0-9]+) (\d+)/(\d+)\b(?!/)', t): d[m[1]+'.'+k+'.a'], d[m[1]+'.'+k+'.b'] = int(a), int(b)
    for k, v in re.findall(r'\b([a-z0-9]+) (\d+)(?![\d/])', t): d.setdefault(m[1]+'.'+k, int(v))
# drop frames with an empty gbisplit/ops (undrawn)
fs = [f for f in fr if fr[f].get('taskpl.ops', 0) > 0]
print('yard frames', len(fs))
keys = sorted({k for f in fs for k in fr[f]})
for k in keys: print('%-22s %10.1f' % (k, sum(fr[f].get(k, 0) for f in fs) / len(fs)))
