"""Per-scene phase split from a phase log (console or Azahar). Complements attract_report.py (W C X G ops closes tasks hops).

Usage: python 3ds/tools/console_numbers.py LOG [segments]
Groups `phase W` windows (frame-weighted) and per-frame gpusplit/flsplit/thsplit/taskpl lines by game mode.
'segments' prints one row per game-mode segment of 60 frames or more instead of per group.
Columns are ms per drawn frame: wall, fps, logic gbi gpu tex fbegin fend idle audio other (phase W windows),
fb/fe/idle from gpusplit, log+hud, task us bg/main, fl us and empty closes per class, th hit n/us, tm miss n/us.
"""
import re
import sys

NM = {0: 'STARTUP', 1: 'LOGOS', 2: 'TITLE', 3: 'DEMO_ENTER', 4: 'WORLD', 5: 'CHANGE_MAP', 7: 'ENTER_WORLD', 8: 'BATTLE',
      9: 'END_BATTLE', 10: 'PAUSE', 14: 'FILE_SEL', 16: 'INTRO', 17: 'DEMO', -1: 'BOOT'}
PH = ['logic', 'gbi', 'gpu', 'tex', 'fbegin', 'fend', 'idle', 'audio', 'other', 'gcache', 'gdraw']
CLS = ['tex', 'cc', 'geo', 'rect', 'tri', 'oth']
pre = re.compile(r'^#\d+ ')
segs = [{'mode': -1, 'pf': 0, 'wins': [], 'f': []}]
cur = None
fr = {}
for line in open(sys.argv[1], errors='replace'):
    s = pre.sub('', line.rstrip('\n'))
    m = re.match(r'game mode (\d+) frame (\d+)', s)
    if m:
        segs.append({'mode': int(m[1]), 'pf': int(m[2]), 'wins': [], 'f': []})
        continue
    m = re.match(r'phase W f(\d+)-(\d+) n (\d+).*wall avg/p95/max ([\d.]+)', s)
    if m:
        cur = {'n': int(m[3]), 'wall': float(m[4])}
        segs[-1]['wins'].append(cur)
        continue
    if s.startswith('phase ms') and cur is not None:
        for k, v in re.findall(r'([a-z0-9_]+) ([\d.]+)/', s):
            cur[k] = float(v)
        continue
    m = re.match(r'(taskpl|gpusplit|flsplit|thsplit) f(\d+) (.*)', s)
    if m:
        f = int(m[2])
        d = fr.setdefault(f, {})
        if f not in segs[-1]['f']:
            segs[-1]['f'].append(f)
        t = m[3]
        if m[1] == 'taskpl':
            v = re.match(r'bg (\d+) main (\d+) key (\d+) us (\d+) (\d+) (\d+)', t)
            d['bgus'], d['mainus'] = int(v[4]), int(v[5])
        elif m[1] == 'gpusplit':
            for k in ('fb', 'fe', 'idle', 'hud', 'log', 'gdraw', 'cpu'):
                d['g' + k] = int(re.search(r'\b%s (\d+)' % k, t)[1])
        elif m[1] == 'flsplit':
            for c, n, e, us in re.findall(r'(\w+) (\d+)/(\d+)/(\d+)', t):
                d['n' + c], d['e' + c], d['u' + c] = int(n), int(e), int(us)
        else:
            v = [int(x) for x in re.findall(r'\d+', t)]
            d['thn'], d['thu'], d['tmn'], d['tmu'] = v[0], v[1], v[2], v[3]


def row(name, wins, fs):
    wins = [w for w in wins if w['n'] >= 2]
    n = sum(w['n'] for w in wins)
    if n == 0 or not fs:
        return None
    wa = lambda k: sum(w.get(k, 0.0) * w['n'] for w in wins) / n
    fa = lambda k: sum(fr[f].get(k, 0) for f in fs) / len(fs) / 1000.0
    fn = lambda k: sum(fr[f].get(k, 0) for f in fs) / len(fs)
    wall = wa('wall')
    out = '%-12s %5d %5.1f %4.1f | ' % (name, n, wall, 1000 / wall if wall else 0)
    out += ' '.join('%4.1f' % wa(k) for k in PH)
    out += ' | fb %4.1f fe %3.1f id %4.1f G %4.1f lh %3.1f | bg %4.1f main %4.1f | fl ' % (
        fa('gfb'), fa('gfe'), fa('gidle'), fa('ggdraw'), fa('glog') + fa('ghud'), fa('bgus'), fa('mainus'))
    out += ' '.join('%s %.1f/%d/%d' % (c, fa('u' + c), fn('n' + c), fn('e' + c)) for c in CLS)
    out += ' | th %d/%.1f tm %.1f/%.1f' % (fn('thn'), fa('thu'), fn('tmn'), fa('tmu'))
    return out


print('name         frames wall fps | ' + ' '.join(PH) + ' | gpusplit ms | task ms | fl ms/n/empty per class | th n/ms tm n/ms')
if len(sys.argv) > 2:
    for i, sg in enumerate(segs):
        r = row('%d:%s' % (i, NM.get(sg['mode'], sg['mode'])), sg['wins'], sg['f'])
        if r and len(sg['f']) >= 60:
            print(r)
else:
    grp = {}
    for sg in segs:
        g = grp.setdefault(NM.get(sg['mode'], str(sg['mode'])), {'wins': [], 'f': []})
        g['wins'] += sg['wins']
        g['f'] += sg['f']
    for k, g in grp.items():
        r = row(k, g['wins'], g['f'])
        if r:
            print(r)
