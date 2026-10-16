"""PT01: read a game log made with switches `phase phasecsv` and print per-scene phase tables.
  python 3ds/tools/phase_report.py log.txt [over_ms]
Scene = the last `game mode N` / `load map area A map M` line before a frame. Times are ms.
Columns come from the `pc` line (units of 0.1 ms): w wall, i idle, lo logic, dl display list build, gb GBI, gp GPU work,
tx texture, au audio, ro ROM, y Yay0, fb C3D_FrameBegin (waits for the previous GPU frame and the vblank-locked screen transfer), fe C3D_FrameEnd, ot other,
gc GSPGPU_FlushDataCache per draw, gd BufInfo + C3D_DrawArrays (gp is the rest of the flush: state setup).
cpu = wall - idle - fbegin.
Counters after the bar: triout, vtxslot, draws, texmiss, texKB, romn, romB, atick."""
import re
import sys
from collections import defaultdict

COLS = ['wall', 'cpu', 'logic', 'dl', 'gbi', 'gpu', 'gcache', 'gdraw', 'g_vtx', 'g_tri', 'g_mtx', 'g_op', 'tex', 'audio', 'rom+yay0', 'fbegin', 'fend', 'idle', 'other']
MODES = {0: 'startup', 1: 'logos', 2: 'title', 3: 'enter_demo', 4: 'world', 5: 'change_map', 6: 'game_over', 7: 'enter_world',
         8: 'battle', 9: 'end_battle', 10: 'pause', 11: 'unpause', 12: 'lang_select', 13: 'end_lang', 14: 'file_select',
         15: 'end_file_select', 16: 'intro', 17: 'demo'}
KEYS = ['w', 'i', 'lo', 'dl', 'gb', 'gp', 'tx', 'au', 'ro', 'y', 'fb', 'fe', 'ot', 'gc', 'gd', 'gv', 'gt', 'gm', 'go']
CKEYS = ['triout', 'vtx', 'draw', 'texmiss', 'texKB', 'romn', 'romB', 'atick']
PC = re.compile(r'^#(\d+) pc (\d+) w (-?\d+) i (-?\d+) lo (-?\d+) dl (-?\d+) gb (-?\d+) gp (-?\d+) tx (-?\d+) au (-?\d+) ro (-?\d+) y (-?\d+) fb (-?\d+) fe (-?\d+) ot (-?\d+) gc (-?\d+) gd (-?\d+) gv (-?\d+) gt (-?\d+) gm (-?\d+) go (-?\d+) \| (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)')
MODE = re.compile(r'^#(\d+) game mode (\d+)')
MAP = re.compile(r'^#(\d+) load map area (\d+) map (\d+)')


def pct(v, p):
    s = sorted(v)
    return s[min(len(s) - 1, (len(s) * p + 99) // 100 - 1)]


def main():
    path = sys.argv[1]
    over_ms = float(sys.argv[2]) if len(sys.argv) > 2 else 34.0
    mode, amap = 'startup', ''
    scenes = defaultdict(list)
    order = []
    frames = []
    for ln in open(path, encoding='utf-8', errors='replace'):
        m = MODE.match(ln)
        if m:
            mode = MODES.get(int(m.group(2)), m.group(2))
            continue
        m = MAP.match(ln)
        if m:
            amap = 'a%sm%s' % (m.group(2), m.group(3))
            continue
        m = PC.match(ln)
        if not m:
            continue
        g = [int(x) for x in m.groups()]
        rec = {'seq': g[0], 'f': g[1]}
        for k, v in zip(KEYS, g[2:21]):
            rec[k] = v / 10.0
        for k, v in zip(CKEYS, g[21:29]):
            rec[k] = v
        name = mode if mode in ('logos', 'intro', 'title', 'file_select', 'end_file_select', 'startup') else '%s %s' % (mode, amap)
        rec['scene'] = name
        if name not in order:
            order.append(name)
        scenes[name].append(rec)
        frames.append(rec)
    print('frames %d, over %.0f ms threshold on wall' % (len(frames), over_ms))
    cols = [c for c in COLS if not c.startswith('g_') or any(r[{'g_vtx': 'gv', 'g_tri': 'gt', 'g_mtx': 'gm', 'g_op': 'go'}[c]] for r in frames)]
    print('%-22s %5s | %s' % ('scene', 'n', ' '.join('%-17s' % k for k in cols)))
    print('(each cell: avg/p95/max ms)')
    for name in order:
        fr = scenes[name]
        if len(fr) < 5:
            continue
        cells = []
        series = {
            'wall': [r['w'] for r in fr], 'work': [r['w'] - r['i'] for r in fr], 'logic': [r['lo'] for r in fr], 'dl': [r['dl'] for r in fr],
            'cpu': [r['w'] - r['i'] - r['fb'] for r in fr], 'gcache': [r['gc'] for r in fr], 'gdraw': [r['gd'] for r in fr], 'g_vtx': [r['gv'] for r in fr], 'g_tri': [r['gt'] for r in fr], 'g_mtx': [r['gm'] for r in fr], 'g_op': [r['go'] for r in fr],
            'gbi': [r['gb'] for r in fr], 'gpu': [r['gp'] for r in fr], 'tex': [r['tx'] for r in fr], 'audio': [r['au'] for r in fr],
            'rom+yay0': [r['ro'] + r['y'] for r in fr], 'fbegin': [r['fb'] for r in fr], 'fend': [r['fe'] for r in fr],
            'idle': [r['i'] for r in fr], 'other': [r['ot'] for r in fr]}
        for k in cols:
            v = series[k]
            cells.append('%5.1f/%4.1f/%5.1f' % (sum(v) / len(v), pct(v, 95), max(v)))
        print('%-22s %5d | %s' % (name, len(fr), ' '.join(cells)))
    print()
    print('counters per frame (avg/max): scene | triout vtxslot draws texmiss texKB romn romB')
    for name in order:
        fr = scenes[name]
        if len(fr) < 5:
            continue
        print('%-22s ' % name + ' '.join('%s %d/%d' % (k, sum(r[k] for r in fr) / len(fr), max(r[k] for r in fr)) for k in CKEYS[:7]))
    print()
    # frames over the threshold and what caused them: the phase that is farthest above the scene median (3 ms or more), else 'base'
    pk = ['lo', 'dl', 'gb', 'gp', 'gc', 'gd', 'gv', 'gt', 'gm', 'go', 'tx', 'au', 'ro', 'y', 'fe', 'ot']  # cpu phases (fbegin is the wait)
    med = {n: {k: pct([r[k] for r in scenes[n]], 50) for k in pk} for n in scenes}
    cause = defaultdict(int)
    cause_ms = defaultdict(float)
    over = [r for r in frames if r['w'] > over_ms]
    for r in over:
        ex = {k: r[k] - med[r['scene']][k] for k in pk}
        k = max(ex, key=ex.get)
        if ex[k] < 3.0:
            k = 'base'  # no phase is 3 ms above its scene median: the frame is slow because the steady load is
        cause[k] += 1
        cause_ms[k] += ex.get(k, 0.0)
    print('frames over %.0f ms: %d of %d (%.1f%%)' % (over_ms, len(over), len(frames), 100.0 * len(over) / max(1, len(frames))))
    for k in sorted(cause, key=lambda x: -cause[x]):
        print('  cause %-4s %4d frames, mean excess over scene median %.1f ms' % (k, cause[k], cause_ms[k] / cause[k]))
    by_scene = defaultdict(int)
    for r in over:
        by_scene[r['scene']] += 1
    cpuover = [r for r in frames if r['w'] - r['i'] - r['fb'] > 33.3]
    print('frames with cpu work over 33.3 ms: %d (%.1f%%)' % (len(cpuover), 100.0 * len(cpuover) / max(1, len(frames))))
    print('over-threshold frames per scene:', dict(by_scene))
    # periodicity: gaps between over-threshold frames
    gaps = [b['f'] - a['f'] for a, b in zip(over, over[1:]) if a['scene'] == b['scene']]
    hist = defaultdict(int)
    for g in gaps:
        hist[g] += 1
    top = sorted(hist.items(), key=lambda x: -x[1])[:8]
    print('gap (frames) between consecutive over frames in one scene, most common (gap, count):', top)
    print('first 60 over frames (frame:wall:scene):', ' '.join('%d:%.0f' % (r['f'], r['w']) for r in over[:60]))


if __name__ == '__main__':
    main()
