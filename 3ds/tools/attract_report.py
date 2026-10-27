"""Attract-run report: per-scene rows and the PI1-PI5 checks of docs/handoff/plan_perf_instrumentation.md.

Usage: python 3ds/tools/attract_report.py RUN_LOG OUT_REPORT ROW CONSOLE_READY_OUT

Check definitions (each checked against the attract run data, not a fit):
  PI1 task span = exclusive phases nested in the DL walk. audio runs in pm_audio_step before the walk, and
      other, logic, dl, fend, idle run outside it. Reference: gbi gpu tex gcache gdraw fbread rom yay0 g_*.
  PI2 fl (flush_impl, timed by pm_gpu_flush_at) = exclusive phases pushed inside flush_impl: gpu gbind gtev gst gdraw.
      Direct flush() calls in pm_gpu.c are not timed in fl, so the ratio sits a little under 1.
  PI3 phase count texcall is an integer division of the window's frame sum. Check: floor(th n + tm n per frame) == texcall.
  PI4 gpusplit on every frame, kick n per frame 0 or 1.
  PI5 log and hud fields present on every gpusplit line. The frame wall compare with a second run is in console_ready.txt.
"""
import re
import sys

src, out, row, cr_out = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]
NM = {0: 'STARTUP', 1: 'LOGOS', 2: 'TITLE', 3: 'DEMO_ENTER', 4: 'WORLD', 5: 'CHANGE_MAP', 6: 'GAME_OVER', 7: 'ENTER_WORLD',
      8: 'BATTLE', 9: 'END_BATTLE', 10: 'PAUSE', 11: 'UNPAUSE', 12: 'LANG_SEL', 13: 'END_LANG', 14: 'FILE_SEL', 15: 'END_FILE',
      16: 'INTRO', 17: 'DEMO', -1: 'BOOT'}
PH_WALK = ['gbi', 'gpu', 'tex', 'gcache', 'gdraw', 'fbread', 'rom', 'yay0', 'g_vtx', 'g_tri', 'g_mtx', 'g_op', 'g_tev', 'g_bind', 'g_state']
PH_FLUSH = ['gpu', 'g_bind', 'g_tev', 'g_state', 'gdraw']
pre = re.compile(r'^#\d+ ')
RE_MODE = re.compile(r'^game mode (\d+) frame (\d+)')
RE_TASK = re.compile(r'^taskpl f(\d+) bg (\d+) main (\d+) key (\d+) us (\d+) (\d+) (\d+) ops (\d+) push (\d+) nopush (\d+) ret (\d+) range (\d+)')
RE_GBI = re.compile(r'^gbisplit f(\d+) sync (\d+) gv (\d+) tex (\d+)')
RE_FL = re.compile(r'^flsplit f(\d+) (.*)')
RE_TH = re.compile(r'^thsplit f(\d+) hit (\d+)/(\d+) miss (\d+)/(\d+) dc (\d+)/(\d+)')
RE_GPU = re.compile(r'^gpusplit f(\d+) cpu (\d+) fb (\d+) fe (\d+) idle (\d+) gdraw (\d+) gproc (\d+) kick (\d+)/(\d+) draws (\d+) cmd (\d+) vb (\d+) iv (\d+) hud (\d+) log (\d+)')
RE_WIN = re.compile(r'^phase W f(\d+)-(\d+) n (\d+)')
RE_CNT = re.compile(r'^phase count avg/max per frame:.*texcall (\d+)/')

segs = [{'mode': -1, 'pf': 0, 'wins': [], 'fr': []}]
frames = {}
pend = {}
cur_w = None
for line in open(src, errors='replace'):
    s = pre.sub('', line.rstrip('\n'))
    m = RE_MODE.match(s)
    if m:
        segs.append({'mode': int(m[1]), 'pf': int(m[2]), 'wins': [], 'fr': []})
        continue
    m = RE_TASK.match(s)
    if m:
        pend['task'] = [int(x) for x in m.groups()]
        continue
    m = RE_GBI.match(s)
    if m:
        pend['gbi'] = [int(x) for x in m.groups()]
        continue
    m = RE_FL.match(s)
    if m:
        pend['fl'] = [(int(a), int(b), int(c)) for a, b, c in re.findall(r'(\d+)/(\d+)/(\d+)', m[2])]
        continue
    m = RE_TH.match(s)
    if m:
        f = int(m[1])
        v = [int(x) for x in m.groups()[1:]]
        frames[f] = {'task': pend.get('task'), 'fl': pend.get('fl'), 'hit': (v[0], v[1]), 'miss': (v[2], v[3]), 'g': None}
        segs[-1]['fr'].append(f)
        pend = {}
        continue
    m = RE_GPU.match(s)
    if m:
        f = int(m[1])
        v = [int(x) for x in m.groups()[1:]]
        if f in frames:
            frames[f]['g'] = {'fb': v[1], 'gdraw': v[4], 'kick_n': v[6], 'hud': v[12], 'log': v[13]}
        continue
    m = RE_WIN.match(s)
    if m:
        m2 = re.search(r'wall avg/p95/max ([\d.]+)', s)
        cur_w = {'a': int(m[1]), 'b': int(m[2]), 'n': int(m[3]), 'gbi': 0.0, 'wall': float(m2[1]) if m2 else 0.0, 'texcall': None}
        segs[-1]['wins'].append(cur_w)
        continue
    if s.startswith('phase ms') and cur_w is not None:
        for k, v in re.findall(r'([a-z0-9_]+) ([\d.]+)/', s):
            cur_w[k] = float(v)
    m = RE_CNT.match(s)
    if m and cur_w is not None:
        cur_w['texcall'] = int(m[1])


def fv(r):
    t = r['task'] or [0] * 12
    fl = r['fl'] or [(0, 0, 0)] * 6
    g = r['g'] or {}
    return {
        'tus': t[4] + t[5] + t[6],
        'ttk': t[1] + t[2] + t[3],
        'ops': t[7],
        'hops': t[8] + t[10] + t[11],
        'closes': sum(x[0] for x in fl),
        'C': sum(x[2] for x in fl),
        'X': r['hit'][1] + r['miss'][1],
        'G': g.get('gdraw', 0),
        'th_n': r['hit'][0] + r['miss'][0],
        'log': g.get('log', 0),
        'hud': g.get('hud', 0),
        'kick_n': g.get('kick_n', 0),
    }


def avg(lst, k):
    return sum(fv(frames[f])[k] for f in lst) / max(1, len(lst))


def wmean(wins):
    n = sum(w['n'] for w in wins)
    if n == 0:
        return None
    return sum(w['gbi'] * w['n'] for w in wins) / n * 1000.0


def line_of(name, idx, pf, f0, f1, n, W, fr):
    Ws = 'na' if W is None else '%.0f' % W
    return '%-4s %-14s %8s  %5s-%-5s %6d  %6s  %6.0f  %6.0f  %6.0f  %8.1f  %8.1f  %7.1f  %6.1f' % (
        idx, name, pf, f0, f1, n, Ws, avg(fr, 'C'), avg(fr, 'X'), avg(fr, 'G'), avg(fr, 'ops'), avg(fr, 'closes'), avg(fr, 'ttk'), avg(fr, 'hops'))


HDR = 'idx  name           pm_frame  f_from-f_to  frames  W_us/f  C_us/f  X_us/f  G_us/f   ops/f    closes/f  tasks/f  hops/f'
GHDR = 'group          segs  frames  W_us/f  C_us/f  X_us/f  G_us/f   ops/f    closes/f  tasks/f  hops/f'

seg_lines = []
for i, sg in enumerate(segs):
    fr = sg['fr']
    if not fr:
        continue
    seg_lines.append(line_of(NM.get(sg['mode'], str(sg['mode'])), str(i), sg['pf'], min(fr), max(fr), len(fr), wmean(sg['wins']), fr))

grp = {}
for sg in segs:
    if not sg['fr']:
        continue
    g = grp.setdefault(NM.get(sg['mode'], str(sg['mode'])), {'fr': [], 'wins': [], 'segs': 0})
    g['fr'] += sg['fr']
    g['wins'] += sg['wins']
    g['segs'] += 1
grp_lines = []
for name, g in grp.items():
    W = wmean(g['wins'])
    Ws = 'na' if W is None else '%.0f' % W
    grp_lines.append('%-14s %5d %7d  %6s  %6.0f  %6.0f  %6.0f  %8.1f  %8.1f  %7.1f  %6.1f' % (
        name, g['segs'], len(g['fr']), Ws, avg(g['fr'], 'C'), avg(g['fr'], 'X'), avg(g['fr'], 'G'), avg(g['fr'], 'ops'),
        avg(g['fr'], 'closes'), avg(g['fr'], 'ttk'), avg(g['fr'], 'hops')))

allf = sorted(frames)
task_present = sum(1 for f in allf if frames[f]['task'])
g_present = sum(1 for f in allf if frames[f]['g'])
kick_max = max([fv(frames[f])['kick_n'] for f in allf] or [0])
log_avg = avg(allf, 'log')
log_max = max([fv(frames[f])['log'] for f in allf] or [0])
hud_avg = avg(allf, 'hud')
wall_all = [w for sg in segs for w in sg['wins'] if w['n'] >= 2]
wall_mean = sum(w['wall'] * w['n'] for w in wall_all) / max(1, sum(w['n'] for w in wall_all))

sum_t = sum_ref1 = sum_ref2 = 0.0
tc_ok = tc_n = 0
tc_bad = []
win_n = 0
for sg in segs:
    for w in sg['wins']:
        if w['n'] < 2:
            continue
        fs = [f for f in range(w['a'], w['b'] + 1) if f in frames]
        if not fs:
            continue
        win_n += 1
        sum_t += sum(fv(frames[f])['tus'] for f in fs)
        sum_ref1 += sum(w.get(k, 0.0) for k in PH_WALK) * len(fs) * 1000.0
        sum_ref2 += sum(w.get(k, 0.0) for k in PH_FLUSH) * len(fs) * 1000.0
        if w['texcall'] is not None:
            thn = sum(fv(frames[f])['th_n'] for f in fs)
            tc_n += 1
            if thn // len(fs) == w['texcall']:
                tc_ok += 1
            else:
                tc_bad.append((w['a'], w['b'], len(fs), thn, w['texcall']))
sumC = sum(fv(frames[f])['C'] for f in allf)
r1 = sum_t / sum_ref1 if sum_ref1 else float('nan')
r2 = sumC / sum_ref2 if sum_ref2 else float('nan')
pi1_ok = abs(r1 - 1) <= 0.05
pi2_ok = abs(r2 - 1) <= 0.05
pi3_ok = tc_n > 0 and tc_ok == tc_n
pi4_ok = g_present == len(allf) and kick_max <= 1
pi5_ok = g_present == len(allf) and log_avg > 0
pi5_val = 'log avg %.0f us, max %d us, hud avg %.0f us, frame wall avg %.2f ms' % (log_avg, log_max, hud_avg, wall_mean)

lines = []
lines.append('attract report: row %s, source %s' % (row, src))
lines.append('frames with split lines: %d, taskpl: %d, gpusplit: %d, phase windows used: %d' % (len(allf), task_present, g_present, win_n))
lines.append('Units: W, C, X, G, us per frame. G = gdraw, GPU time of frame f-1 (read at begin of f).')
lines.append('Scene = run of frames after one "game mode" line. Mode map (game_modes.h): 0 STARTUP 1 LOGOS 2 TITLE 3 ENTER_DEMO_WORLD 4 WORLD 5 CHANGE_MAP 8 BATTLE 9 END_BATTLE 14 FILE_SEL 16 INTRO 17 DEMO.')
lines.append('')
lines.append('== per scene')
lines.append(HDR)
lines.extend(seg_lines)
lines.append('')
lines.append('== per mode group')
lines.append(GHDR)
lines.extend(grp_lines)
lines.append('')
lines.append('== plan checks (attract data)')
lines.append('PI1 sum check: sum task us / sum of walk phases (%s) over %d windows = %.3f (pass 0.95-1.05: %s)' % (' '.join(PH_WALK), win_n, r1, 'yes' if pi1_ok else 'no'))
lines.append('PI2 sum check: sum fl us / sum of flush phases (%s) over %d windows = %.3f (pass 0.95-1.05: %s)' % (' '.join(PH_FLUSH), win_n, r2, 'yes' if pi2_ok else 'no'))
lines.append('PI3 texcall: floor((th n + tm n) per frame) == texcall in %d of %d windows (pass all: %s)' % (tc_ok, tc_n, 'yes' if pi3_ok else 'no'))
for b in tc_bad[:10]:
    lines.append('  PI3 mismatch window f%d-%d frames %d th %d texcall %d' % b)
lines.append('PI4 gpusplit on every frame: %d of %d (%s). Kick n per frame max %d (0 or 1: %s)' % (g_present, len(allf), 'yes' if g_present == len(allf) else 'no', kick_max, 'yes' if kick_max <= 1 else 'no'))
lines.append('PI5 fields present on every frame: %s. %s' % ('yes' if pi5_ok else 'no', pi5_val))
open(out, 'w').write('\n'.join(lines) + '\n')

verdict = 'yes' if (pi1_ok and pi2_ok and pi3_ok and pi4_ok and pi5_ok) else 'no'
cr = []
cr.append('console_ready: row %s, source %s' % (row, src))
cr.append('Columns (us per frame): W phase gbi walk, C fl sum, X th+tm, G gdraw (f-1), ops C_CMD, closes, tasks, hops (push+ret+range).')
cr.append(GHDR)
cr.extend(grp_lines)
cr.append('PI1 sum check %.3f (%s). PI2 sum check %.3f (%s). PI3 %d of %d windows (%s). PI4 %s. PI5 %s.' % (
    r1, 'yes' if pi1_ok else 'no', r2, 'yes' if pi2_ok else 'no', tc_ok, tc_n, 'yes' if pi3_ok else 'no',
    'yes' if pi4_ok else 'no', pi5_val))
cr.append('VERDICT: %s. Reason: %s' % (verdict, 'all checks pass on the attract run' if verdict == 'yes' else 'at least one check fails, see the line above'))
open(cr_out, 'w').write('\n'.join(cr) + '\n')
print('frames %d, gpusplit %d, windows %d, PI1 %.3f, PI2 %.3f, PI3 %d/%d, verdict %s' % (len(allf), g_present, win_n, r1, r2, tc_ok, tc_n, verdict))
