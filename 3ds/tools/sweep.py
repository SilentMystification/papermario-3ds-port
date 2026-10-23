#!/usr/bin/env python3
"""sweep.py - runner of the FULL SWEEP (cutscenes and credits; boss fights are manual and not part of it) in Azahar. Runs ONLY when the user asks for a full sweep.
It is not part of any gate or feature test. No other agent may run it. See docs/plans/sweep_notes.md.
  sweep.py run <list> [--x 3dsx] [--only a,b] [--timeout s] [--force] [--dry]   list = all (cutscenes, credits_full and credits_scenes) | cutscenes | credits (the whole chain to the thank-you screen) | credits_scenes (12 short scenes of 20 s, fault hunting) | item names (comma)
  sweep.py list                                                                 print the items of 3ds/sweep/sweep.txt
  sweep.py report                                                               table (sweep_report.py) and contact sheets (sweep_sheet.py)
One Azahar start per item, so every item has a clean game state, its own log and its own shots: 3ds/logs/sweep/<name>/ (git-ignored).
Data: 3ds/sweep/sweep.txt, one line per item: <name> kind=cut|credits|credits_scene|credits_more warp=<map>:<entry> [story=<n>] [cap=<steps>] [jump=<x>] [until=thankyou] [timeout=<s>] [extra=<switch+switch>] [note=<text>]
A line that starts with # is a comment. Switch strings hold 256 bytes and no commas, so the tool prints the length and refuses a longer one."""
import os, subprocess, sys, shutil, time, argparse

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
DATA = os.path.join(ROOT, '3ds/sweep/sweep.txt')
OUT = os.path.join(ROOT, '3ds/logs/sweep')
WIN = os.name == 'nt'
SD = (os.path.join(os.environ.get('APPDATA', ''), 'Azahar', 'sdmc', '3ds', 'PaperMario') if WIN
      else os.path.expanduser('~/.var/app/org.azahar_emu.Azahar/data/azahar-emu/sdmc/3ds/PaperMario'))

def azahar(sw, timeout, until, log, force, x, env):
    """One Azahar start. Windows: 3ds/run_azahar.ps1 (-Turbo is the fast mode, it runs build3ds/pm_3ds.3dsx or the PM_LANE build, so x is not used).
    Linux: 3ds/run_azahar.sh. The game log goes to <log>, the shots next to it (<dir>/shots), memory fault lines to <dir>/azahar.txt."""
    d = os.path.dirname(log)
    if WIN:
        cmd = ['powershell', '-NoProfile', '-File', '3ds/run_azahar.ps1', '-Turbo', '-Timeout', str(timeout), '-Until', until, '-Debug', sw]
        if force: cmd.append('-Force')
        r = subprocess.run(cmd, cwd=ROOT, env=env, capture_output=True, text=True)
        lane = env.get('PM_LANE')
        out = os.path.join(ROOT, 'build3ds', 'lane-' + lane) if lane else os.path.join(ROOT, 'build3ds')
        if os.path.exists(os.path.join(out, 'last_log.txt')): shutil.copy(os.path.join(out, 'last_log.txt'), log)
        if os.path.isdir(os.path.join(out, 'shots')): shutil.copytree(os.path.join(out, 'shots'), os.path.join(d, 'shots'), dirs_exist_ok=True)
        try:
            lines = [l for l in open(os.path.join(out, 'last_unmapped.txt'), errors='replace') if l.strip()]
            open(os.path.join(d, 'azahar.txt'), 'w').write(''.join(lines[:200]))
        except OSError: pass
        return r
    r = subprocess.run(['sh', '3ds/run_azahar.sh', '-F', '-x', x, '-d', sw, '-t', str(timeout), '-u', until, '-o', log], cwd=ROOT, env=env, capture_output=True, text=True)
    try:
        lines = [l for l in open('/tmp/claude-1000/azahar_stdout.txt', errors='replace') if 'Unmapped' in l or 'Crash' in l or 'CRASH' in l]
        open(os.path.join(d, 'azahar.txt'), 'w').write(''.join(lines[:200]))
    except OSError: pass
    return r

def load():
    items = []
    for ln in open(DATA):
        ln = ln.strip()
        if not ln or ln.startswith('#'): continue
        t = ln.split()
        d = {'name': t[0]}
        for x in t[1:]:
            if '=' in x:
                k, v = x.split('=', 1)
                d[k] = v
            else:
                d['note'] = d.get('note', '') + ' ' + x
        items.append(d)
    return items

def select(items, what):
    if what == 'all': return [i for i in items if i.get('kind') != 'boss']  # bosses are never automated
    if what == 'cutscenes': return [i for i in items if i.get('kind') == 'cut']
    if what == 'credits': return [i for i in items if i.get('kind') == 'credits']
    if what == 'credits_scenes': return [i for i in items if i.get('kind') in ('credits_scene', 'credits_more')]
    if what == 'credits_scenes7': return [i for i in items if i.get('kind') == 'credits_scene']
    names = what.split(',')
    return [i for i in items if i['name'] in names]

_NAMES = {}
def num(n):
    # BTL_ names to numbers (battle_names.h, stage_names.h): the switch string is short
    if not _NAMES:
        import re
        for f in ('src/battle/battle_names.h', 'src/battle/stage_names.h'):
            for m in re.finditer(r'(BTL_\w+)\s*=\s*(0x[0-9A-Fa-f]+)', open(os.path.join(ROOT, f)).read()): _NAMES[m.group(1)] = m.group(2)
    return _NAMES.get(n, n)

def switches(it):
    kind = it.get('kind', 'cut')
    sw = ['fast']
    sw += ['warp=' + it['warp']]
    if True:
        if 'story' in it: sw.append('story=' + it['story'])
        if 'partner' in it: sw.append('partner=' + it['partner'])
        if 'pad' in it: sw.append('padscript=@sweep/' + it['pad'])
        else: sw.append('autoa')
    cap = it.get('cap', '1200')
    sw += ["sweep=" + it["name"], 'sweepcap=' + cap]
    if it.get('jump'): sw.append('sweepjump=' + it['jump'])
    if it.get('until'): sw.append('sweepuntil=' + it['until'])
    sw += ['shots', 'phase', 'phasecsv', 'loglevel=debug', 'fixedseed']
    if it.get('extra'): sw += it['extra'].split('+')
    s = ' '.join(sw)
    return s, int(cap)

def lockwait(out):
    return 0.0

def story_num(v):
    if v in ('intro', 'epilogue'):
        import re
        m = re.search(r'STORY_%s\s*=\s*(-?\d+)' % v.upper(), open(os.path.join(ROOT, 'include/enums.h')).read())
        return m.group(1)
    return v

def run_chain(sel, x, timeout, dry):
    # one Azahar start for the whole list: numbers only, the list file goes to the SD app folder (for the console: curl it to sdmc:/3ds/PaperMario/sweep/list.txt)
    lines = []
    for it in sel:
        t = [it['name'], 'kind=' + it.get('kind', 'cut'), 'cap=' + it.get('cap', '1200')]
        if 'warp' in it: t.append('warp=' + it['warp'])
        if 'story' in it: t.append('story=' + story_num(it['story']))
        if it.get('jump'): t.append('jump=' + it['jump'])
        if it.get('until'): t.append('until=' + it['until'])
        lines.append(' '.join(t))
    d = os.path.join(OUT, 'chain'); os.makedirs(d, exist_ok=True)
    open(os.path.join(d, 'list.txt'), 'w').write('\n'.join(lines) + '\n')
    os.makedirs(os.path.join(SD, 'sweep'), exist_ok=True)
    shutil.copy(os.path.join(d, 'list.txt'), os.path.join(SD, 'sweep', 'list.txt'))
    s = 'fast warp=end_00:0 story=96 autoa sweeplist=@sweep/list.txt phase phasecsv loglevel=debug fixedseed'
    print('chain: %d items, %s' % (len(lines), s))
    if dry: return
    env = dict(os.environ); env.setdefault('PM_ROW', 'SWEEP'); env['PM_BUDGET'] = env.get('PM_BUDGET', '200')
    t0 = time.time()
    r = azahar(s, timeout, r'sweep: chain end', os.path.join(d, 'log.txt'), False, x, env)
    open(os.path.join(d, 'run.txt'), 'w').write(r.stdout + r.stderr)
    print('chain done in %.0f s' % (time.time() - t0))

def run_item(it, x, timeout, dry, force=False):
    s, cap = switches(it)
    if len(s) > 255 or ',' in s:
        print('SKIP %s: switch string %d bytes or comma: %s' % (it['name'], len(s), s)); return 'skip'
    d = os.path.join(OUT, it['name'])
    print('== %s (%d bytes) %s' % (it['name'], len(s), s), flush=True)
    if dry: return 'dry'
    os.makedirs(d, exist_ok=True)
    os.makedirs(os.path.join(SD, 'sweep'), exist_ok=True)
    for f in os.listdir(os.path.join(ROOT, '3ds/sweep')):
        if f.endswith('.txt') and f != 'sweep.txt': shutil.copy(os.path.join(ROOT, '3ds/sweep', f), os.path.join(SD, 'sweep', f))
    log = os.path.join(d, 'log.txt')
    shutil.rmtree(os.path.join(d, 'shots'), ignore_errors=True)
    env = dict(os.environ)
    env.setdefault('PM_ROW', 'SWEEP')
    env['PM_BUDGET'] = env.get('PM_BUDGET', '200')   # the full sweep has its own budget: one start per item, set by the user request
    t0 = time.time()
    r = azahar(s, timeout, r'sweep: .* end ', log, force, x, env)
    open(os.path.join(d, 'run.txt'), 'w').write(r.stdout + r.stderr)
    open(os.path.join(d, 'time.txt'), 'w').write('%.0f\n' % (time.time() - t0 - lockwait(r.stdout)))
    print('   rc %d, %.0f s: %s' % (r.returncode, time.time() - t0, (r.stdout.strip().splitlines() or [''])[-1][:100]), flush=True)
    return 'rc%d' % r.returncode

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('cmd'); ap.add_argument('what', nargs='?', default='all')
    ap.add_argument('--x', default='build3ds/pm_3ds.3dsx', help='Linux only: the 3dsx to run. Windows runs build3ds/pm_3ds.3dsx (or the PM_LANE build).')
    ap.add_argument('--timeout', type=int, default=240); ap.add_argument('--force', action='store_true'); ap.add_argument('--dry', action='store_true')
    ap.add_argument('--only', default=''); ap.add_argument('--chain', action='store_true', help='all selected items in ONE Azahar start (no shots, no clean state between items)')
    a = ap.parse_args()
    items = load()
    if a.cmd == 'list':
        for i in items: print(i['name'], i.get('kind'), i.get('warp', ''), i.get('battle', ''), i.get('story', ''))
        print(len(items), 'items'); return
    if a.cmd == 'report':
        here = os.path.dirname(os.path.abspath(__file__))
        main_report(); return
    if a.cmd != 'run': ap.error('run, list or report')
    sel = select(items, a.what)
    if a.only: sel = [i for i in sel if i['name'] in a.only.split(',')]
    print('%d items' % len(sel), flush=True)
    if a.chain:
        run_chain(sel, a.x, a.timeout, a.dry)
        if not a.dry: main_report()
        return
    t0 = time.time()
    for it in sel:
        d = os.path.join(OUT, it['name'], 'log.txt')
        if not a.force and not a.dry and os.path.exists(d) and ' end ' in open(d, errors='replace').read() and 'sweep: %s end' % it['name'] in open(d, errors='replace').read():
            print('== %s: done before, skip (--force runs again)' % it['name']); continue
        run_item(it, a.x, int(it.get('timeout', a.timeout)), a.dry, a.force)
    print('sweep time %.0f s' % (time.time() - t0))
    if not a.dry: main_report()

def main_report():
    here = os.path.dirname(os.path.abspath(__file__))
    subprocess.run([sys.executable, os.path.join(here, 'sweep_sheet.py')])
    subprocess.run([sys.executable, os.path.join(here, 'sweep_report.py')])

if __name__ == '__main__':
    main()
