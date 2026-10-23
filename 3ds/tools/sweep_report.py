#!/usr/bin/env python3
"""sweep_report.py - table of a full sweep (3ds/logs/sweep/<item>/log.txt, made by sweep.py). One row per item.
Columns: result, steps (game steps of the item, from the end line), ms per step avg/p95/max (cpu = w - i - fb of the phasecsv pc line, as route_report.py), steps over 33.3 ms,
texmiss (sum of the texture misses of the item), battle turns (p = player turns, e = enemy moves), and the thumbnails path.
Result: ok, or FAIL: crash (a [CRASH] line), unmapped (Azahar memory fault), assert or FAIL line in the game log, stuck (end reason stuck, nobattle, or no end line and no new marker), nolog.
A chain log (several items in one log.txt, switch sweeplist) gives one row per item: the steps between its start g and end g.
Use: sweep_report.py [--dir 3ds/logs/sweep] [--md out.txt]. Azahar numbers are host numbers. Only console numbers count."""
import os, re, sys, importlib.util, argparse

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('route_report', os.path.join(HERE, 'route_report.py'))
rr = importlib.util.module_from_spec(spec); spec.loader.exec_module(rr)
END = re.compile(r'sweep: (\S+) end (\S+) f(\d+) g(\d+) pturn (\d+) eturn (\d+) pmove (\d+) ehp (-?\d+) hp (-?\d+) maps (\d+)')
START = re.compile(r'sweep: (\S+) start g(\d+)')

def analyse(d):
    """one dir = one log with one item (sweep.py run) or several items (a chain run). Returns a list of rows."""
    base = os.path.basename(d)
    lg = os.path.join(d, 'log.txt')
    if not os.path.exists(lg): return [dict(name=base, res='nolog', steps=0, avg=0, p95=0, mx=0, over=0, tex=0, turns='-', sheet='-', sec='-')]
    text = open(lg, errors='replace').read()
    lines = text.splitlines()
    starts = {}; ends = {}; order = []
    for l in lines:
        m = START.search(l)
        if m and m.group(1) not in starts: starts[m.group(1)] = int(m.group(2)); order.append(m.group(1))
        m = END.search(l)
        if m and m.group(1) not in ends: ends[m.group(1)] = m.groups()
    d2 = rr.parse(lg)
    crash = '[CRASH]' in text
    az = os.path.join(d, 'azahar.txt')
    unm = os.path.exists(az) and 'Unmapped' in open(az, errors='replace').read()
    bad = [(i, l) for i, l in enumerate(lines) if re.search(r'(?i:assert)|FAIL', l) and 'log rotate failed' not in l and 'texture failures 0' not in l]
    rows = []
    if not order: order = [base]
    for k, name in enumerate(order):
        r = dict(name=name, res='ok', steps=0, avg=0, p95=0, mx=0, over=0, tex=0, turns='-', sheet='-', sec='-')
        fails = []
        g0 = starts.get(name)
        e = ends.get(name)
        if g0 is None: fails.append('nostart')
        if crash and (e is None or k == len(order) - 1): fails.append('crash')
        if unm and (e is None or k == len(order) - 1): fails.append('unmapped')
        if e is None: fails.append('stuck: no end line')
        else:
            r['steps'] = int(e[2]); r['turns'] = 'p%s e%s m%s' % (e[4], e[5], e[6])
            if e[1] in ('stuck', 'nobattle'): fails.append('stuck: ' + e[1])
        g1 = int(e[3]) if e else (max(d2['pc']) if d2['pc'] else 0)
        # lines of the item: from its start to its end line (assert/FAIL lines inside the item only)
        si = next((i for i, l in enumerate(lines) if ('sweep: %s start' % name) in l), 0)
        ei = next((i for i, l in enumerate(lines) if ('sweep: %s end' % name) in l), len(lines))
        for i, l in bad:
            if si <= i <= ei: fails.append('assert/FAIL: ' + l[:70].strip()); break
        if g0 is not None:
            cpu = [(v['w'] - v['i'] - v['fb']) / 10.0 for f, v in sorted(d2['pc'].items()) if g0 <= f <= g1]
            if cpu:
                r['avg'] = sum(cpu) / len(cpu); r['p95'] = rr.pctl(cpu, 0.95); r['mx'] = max(cpu); r['over'] = sum(1 for x in cpu if x > 33.3)
                r['tex'] = sum(v['c'].get('texmiss', 0) for f, v in d2['pc'].items() if g0 <= f <= g1)
                if not r['steps']: r['steps'] = len(cpu)
            elif e: fails.append('nopc')
        if fails: r['res'] = 'FAIL ' + '; '.join(fails)
        tf = os.path.join(d, 'time.txt')
        if len(order) == 1 and os.path.exists(tf): r['sec'] = open(tf).read().strip()
        sh = os.path.join(d, 'sheet.png')
        r['sheet'] = sh if os.path.exists(sh) else '-'
        rows.append(r)
    return rows

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--dir', default=os.path.abspath(os.path.join(HERE, '..', 'logs', 'sweep'))); ap.add_argument('--md', default='')
    a = ap.parse_args()
    rows = []
    for n in sorted(os.listdir(a.dir)):
        if os.path.isdir(os.path.join(a.dir, n)): rows += analyse(os.path.join(a.dir, n))
    out = ['%-34s %-6s %6s %6s %6s %6s %5s %7s %-12s %s' % ('item', 'result', 'steps', 'avg', 'p95', 'max', '>33', 'texmiss', 'turns', 'thumbnails')]
    for r in rows:
        out.append('%-34s %-6s %6d %6.1f %6.1f %6.1f %5d %7d %-12s %5s %s' % (r['name'][:34], r['res'][:60] if r['res'] != 'ok' else 'ok', r['steps'], r['avg'], r['p95'], r['mx'], r['over'], r['tex'], r['turns'], r['sec'], r['sheet']))
    nok = sum(1 for r in rows if r['res'] == 'ok')
    out.append('%d of %d items ok' % (nok, len(rows)))
    txt = '\n'.join(out)
    print(txt)
    open(a.md or os.path.join(a.dir, 'report.txt'), 'w').write(txt + '\n')

if __name__ == '__main__':
    main()
