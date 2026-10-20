#!/usr/bin/env python3
"""HWM1: tables from the phs, phh and phc lines of a pulled log (switch phsum, optional abtog).
  phs_report.py LOG                  per scene and state: steps, ms per step of each phase, cpu p50 p95, on-time share, counters per step
  phs_report.py LOG --az AZLOG       add the ratio to the same scene and state in an Azahar phs log
  phs_report.py LOG --check A B      sum the pc lines of steps A to B and compare with the phs windows inside A to B (needs phasecsv)
  phs_report.py LOG --drops          count 'lines dropped' lines and gaps in the line numbers
Units: phase sums are in 0.1 ms. gbi time of the phasegbi state is inflated by its tick reads (0.933 us each on the O3DS): the report
subtracts gbiops x 0.933 us x 2 from gb in that state (HW) when --hw is given."""
import re, sys, collections
PH = "lo dl gb gp gc gd tx au ro y fb fe ot gv gt gm go fr gtev gbind gst".split()
CN = "tri triout vtxload vtxslot draw flush texcall texmiss texKB romn romKB yayn yayKB dbgcall atick gbiops ajob".split()
PHS = re.compile(r'^#(\d+) phs (-?\d+) (-?\d+):(-?\d+) (\w+) n (\d+) f(\d+)-(\d+) \| ([\d ]+) \| (\d+) (\d+)')
PHH = re.compile(r'^#(\d+) phh (-?\d+) (-?\d+):(-?\d+) (\w+) f(\d+) \| ([\d ]+) \| ([\d ]+)')
PHC = re.compile(r'^#(\d+) phc (-?\d+) (-?\d+):(-?\d+) (\w+) f(\d+) \| ([\d ]+) \| gpu (\d+) (\d+) cmdmax (\d+)')
PC = re.compile(r'^#\d+ pc (\d+) w (-?\d+) i (-?\d+) lo (-?\d+) dl (-?\d+) gb (-?\d+) gp (-?\d+) tx (-?\d+) au (-?\d+) ro (-?\d+) y (-?\d+) fb (-?\d+) fe (-?\d+) ot (-?\d+) gc (-?\d+) gd (-?\d+) gv (-?\d+) gt (-?\d+) gm (-?\d+) go (-?\d+)')

def load(path):
    wins = []  # dict per window, joined by (mode, area, map, state, first frame)
    idx = {}
    for ln in open(path, errors="replace"):
        m = PHS.match(ln)
        if m:
            v = [int(x) for x in m.group(9).split()]
            w = dict(mode=int(m.group(2)), area=int(m.group(3)), map=int(m.group(4)), st=m.group(5), n=int(m.group(6)), f0=int(m.group(7)), f1=int(m.group(8)),
                     ph=dict(zip(PH, v)), p50=int(m.group(10)) / 10, p95=int(m.group(11)) / 10, ok=[0] * 14, late=[0] * 14, c={}, gpu=(0, 0), cmd=0)
            wins.append(w); idx[(w['mode'], w['area'], w['map'], w['st'], w['f0'])] = w
            continue
        m = PHH.match(ln)
        if m:
            w = idx.get((int(m.group(2)), int(m.group(3)), int(m.group(4)), m.group(5), int(m.group(6))))
            if w: w['ok'] = [int(x) for x in m.group(7).split()]; w['late'] = [int(x) for x in m.group(8).split()]
            continue
        m = PHC.match(ln)
        if m:
            w = idx.get((int(m.group(2)), int(m.group(3)), int(m.group(4)), m.group(5), int(m.group(6))))
            if w: w['c'] = dict(zip(CN, [int(x) for x in m.group(7).split()])); w['gpu'] = (int(m.group(8)), int(m.group(9))); w['cmd'] = int(m.group(10))
    return wins

def merge(wins):
    t = collections.OrderedDict()
    for w in wins:
        k = (w['mode'], w['area'], w['map'], w['st'])
        a = t.setdefault(k, dict(n=0, ph=collections.Counter(), c=collections.Counter(), ok=[0] * 14, late=[0] * 14, p50=[], p95=[], gpu=[0, 0], cmd=0))
        a['n'] += w['n']
        a['ph'].update(w['ph']); a['c'].update(w['c'])
        a['ok'] = [x + y for x, y in zip(a['ok'], w['ok'])]; a['late'] = [x + y for x, y in zip(a['late'], w['late'])]
        a['p50'].append((w['p50'], w['n'])); a['p95'].append((w['p95'], w['n']))
        a['gpu'][0] += w['gpu'][0]; a['gpu'][1] += w['gpu'][1]; a['cmd'] = max(a['cmd'], w['cmd'])
    return t

def wmean(l):
    n = sum(x[1] for x in l)
    return sum(x[0] * x[1] for x in l) / n if n else 0

def per_step(a, key):
    return a['ph'][key] / 10 / a['n'] if a['n'] else 0

def table(path, az=None, hw=False):
    t = merge(load(path))
    ta = merge(load(az)) if az else {}
    print("mode area:map state | n | " + " ".join(PH[:13]) + " | cpu50 cpu95 ok% | draws tris vtxslot texKB gbiops | gpuUs cmd")
    for k, a in t.items():
        n = a['n']
        ph = {p: per_step(a, p) for p in PH}
        if hw and k[3] == 'phasegbi':
            ph['gb'] -= a['c']['gbiops'] / n * 0.933e-3 * 2
        ok = sum(a['ok']); late = sum(a['late'])
        row = "%d %d:%d %s | %d | " % (k + (n,)) + " ".join("%.2f" % ph[p] for p in PH[:13])
        row += " | %.1f %.1f %.0f | %d %d %d %d %d | %d %.1f" % (wmean(a['p50']), wmean(a['p95']), 100.0 * ok / max(1, ok + late),
               a['c']['draw'] / n, a['c']['triout'] / n, a['c']['vtxslot'] / n, a['c']['texKB'] / n, a['c']['gbiops'] / n, a['gpu'][0] / n, a['cmd'] / 10.0)
        print(row)
        b = ta.get(k)
        if b:
            print("   ratio to az | " + " ".join("%.2f" % (ph[p] / (per_step(b, p) or float('nan'))) for p in PH[:13]))

def check(path, lo, hi):
    pcs = {}
    for ln in open(path, errors="replace"):
        m = PC.match(ln)
        if m: pcs[int(m.group(1))] = [int(x) for x in m.groups()[1:]]
    names = "w i lo dl gb gp tx au ro y fb fe ot gc gd gv gt gm go".split()
    wins = [w for w in load(path) if w['f0'] >= lo and w['f1'] <= hi]
    worst = 0.0
    for w in wins:
        s = collections.Counter()
        steps = [f for f in range(w['f0'], w['f1'] + 1) if f in pcs]
        if len(steps) != w['n']: print("window f%d-%d: %d pc lines of %d steps (windows with skipped steps or gaps are not exact)" % (w['f0'], w['f1'], len(steps), w['n'])); continue
        for f in steps:
            for nm, v in zip(names, pcs[f]): s[nm] += v
        for p in ("lo", "dl", "gb", "gp", "tx", "au", "fb", "fe", "ot", "gd"):
            a, b = w['ph'][p], s[p] + 0.5 * w['n']  # a pc field is cut to 0.1 ms: add the mean cut
            if max(a, b) > 20 * w['n']:
                worst = max(worst, abs(a - b) / max(a, b))
    print("windows checked %d, worst relative difference %.3f percent" % (len(wins), worst * 100))

def drops(path):
    d = 0; prev = None; gaps = 0
    for ln in open(path, errors="replace"):
        if 'lines dropped' in ln: d += int(re.search(r'log: (\d+) lines', ln).group(1))
    print("dropped lines reported:", d)

if __name__ == "__main__":
    a = sys.argv[1:]
    if len(a) >= 3 and a[1] == "--check": check(a[0], int(a[2]), int(a[3]))
    elif len(a) >= 2 and a[1] == "--drops": drops(a[0])
    else: table(a[0], a[a.index("--az") + 1] if "--az" in a else None, "--hw" in a)
