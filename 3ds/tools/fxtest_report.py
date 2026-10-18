#!/usr/bin/env python3
"""FX1a: run the game switch fxtest in Azahar and build the effect table and contact sheets.

  python 3ds/tools/fxtest_report.py run [all|n,n,...]   run chunks, resume after a crash, then report
  python 3ds/tools/fxtest_report.py report              report from 3ds/logs/fx/raw only

Needs 3ds/run_azahar.ps1 (PM_LANE=fx, files in build3ds/lane-fx) (Azahar run mutex) and the Pillow module. Raw shots: 3ds/logs/fx/raw/<n>_<name>_<phase>.ppm.
Contact sheets: 3ds/logs/fx/sheet_<k>.png (12 effects each). Table: docs/plans/FX1_table.md.
Heuristics (frame is 240 x 400, a pixel counts as changed when the sum of the channel differences is over 48):
  drew     = 15 or more dense cells (8 x 8 cells with 12 or more changed pixels) in shot A or B; weak otherwise.
  black    = at least 300 changed pixels turned near black (sum under 30) and they are at least 40 percent of the changed pixels.
  solid    = at least 800 changed pixels, half in one colour (a flat block).
  zoomclip = the dense cells cover over 60 percent of the frame by bounding box, or touch 3 or more frame edges.
"""
import os, re, shutil, subprocess, sys, json
from PIL import Image, ImageDraw

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
RAW = os.path.join(ROOT, '3ds', 'logs', 'fx', 'raw')
OUT = os.path.join(ROOT, '3ds', 'logs', 'fx')
# PM_LANE=fx: run_azahar.ps1 writes last_log.txt and shots into build3ds/lane-fx, so another agent's later run cannot overwrite them
LANE = os.path.join(ROOT, 'build3ds', 'lane-fx')
SHOTS = os.path.join(LANE, 'shots')
LOG = os.path.join(LANE, 'last_log.txt')
TABLE = os.path.join(ROOT, 'docs', 'plans', 'FX1_table.md')
META = os.path.join(OUT, 'meta.json')
SLOT, START = 44, 400
FXN = 127
NOISE_CELLS = 15  # idle animation of Mario and the NPCs gives 4 to 12 dense cells in a sweep, so 15 or more means the effect drew


def run_chunk(idx, preset='toad'):
    sw = 'preset=%s shots fxtest=%s smokeframes=%d' % (preset, ','.join(map(str, idx)), START + SLOT * len(idx) + 500)
    cmd = ['powershell', '-File', os.path.join(ROOT, '3ds', 'run_azahar.ps1'), '-Turbo', '-TurboFactor', '5', '-Smoke',
           '-FaultStop', '-Timeout', '1500', '-Debug', sw]
    os.makedirs(LANE, exist_ok=True)
    shutil.copy(os.path.join(ROOT, 'build3ds', 'pm_3ds.3dsx'), os.path.join(LANE, 'pm_3ds.3dsx'))
    if os.path.exists(LOG): os.remove(LOG)
    rc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, env=dict(os.environ, PM_LANE='fx')).returncode
    log = open(LOG, errors='replace').read() if os.path.exists(LOG) else ''
    if 'fxtest:' not in log:
        print('no fxtest lines in the log, run did not start (rc %d)' % rc)
    names, shots, spawned = {}, {}, []
    pend = None  # the game logs "shot N frame F" for each dump, in request order: that N is the file number
    for l in log.splitlines():
        m = re.search(r'fxtest: shot (\d+) effect (\d+) (\w+) (base|a|b)', l)
        if m:
            pend = (int(m[2]), m[3], m[4])
            continue
        m = re.search(r'shot (\d+) frame (\d+)', l)
        if m and pend:
            names[pend[0]] = pend[1]
            shots[(pend[0], pend[2])] = int(m[1])
            pend = None
    for m in re.finditer(r'fxtest: spawn (\d+) (\w+)', log):
        spawned.append(int(m[1]))
    os.makedirs(RAW, exist_ok=True)
    for (i, ph), n in shots.items():
        src = os.path.join(SHOTS, '%04d.ppm' % n)
        if os.path.exists(src):
            shutil.copy(src, os.path.join(RAW, '%03d_%s_%s.ppm' % (i, names[i], ph)))
    nolog = [l for l in log.splitlines() if 'not spawned' in l]
    return rc, names, shots, spawned, nolog


def run_all(sel):
    meta = {'status': {}, 'names': {}, 'rc': []}
    if os.path.exists(META):
        meta = json.load(open(META))
    todo = list(sel)
    while todo:
        rc, names, shots, spawned, nolog = run_chunk(todo)
        meta['rc'].append([todo[0], todo[-1], rc])
        for i, nm in names.items():
            meta['names'][str(i)] = nm
        done = [i for i in todo if (i, 'b') in shots]
        for i in done:
            meta['status'][str(i)] = 'ok'
        for l in nolog:
            m = re.search(r'not spawned (\w+): (.*)', l)
            if m:
                meta['status'][str([k for k, v in meta['names'].items() if v == m[1]][0])] = 'not spawned: ' + m[2]
        rest = [i for i in todo if i not in done]
        if rest and spawned and spawned[-1] not in done:
            bad = spawned[-1]
            meta['status'][str(bad)] = 'crash'
            print('crash at effect', bad)
            rest = [i for i in rest if i != bad]
            if len(todo) > 1:  # try the crashing effect alone once
                rc1, n1, s1, sp1, nl1 = run_chunk([bad])
                if (bad, 'b') in s1:
                    meta['status'][str(bad)] = 'ok (crashes only after earlier effects in one run)'
                    meta['names'][str(bad)] = n1[bad]
                else:
                    meta['status'][str(bad)] = 'crash alone'
            todo = [i for i in rest if i > bad]
        else:
            todo = []
        json.dump(meta, open(META, 'w'), indent=1)
    return meta


def load(path):
    im = Image.open(path).convert('RGB')
    return im


def analyse(base, shot):
    """Returns (changed px, near black px, dense bbox fraction, dense edges, dense cells). A dense cell is an 8 x 8 cell with
    12 or more changed pixels, so sparse noise (a walking NPC) does not widen the box."""
    w, h = base.size
    pb, ps = base.load(), shot.load()
    ch = blk = 0
    cells = {}
    hist = {}
    for y in range(h):
        for x in range(w):
            a, b = pb[x, y], ps[x, y]
            if abs(a[0] - b[0]) + abs(a[1] - b[1]) + abs(a[2] - b[2]) > 48:
                ch += 1
                if sum(b) < 30 and sum(a) >= 30:
                    blk += 1
                q = (b[0] >> 4, b[1] >> 4, b[2] >> 4)
                hist[q] = hist.get(q, 0) + 1
                c = (x >> 3, y >> 3)
                cells[c] = cells.get(c, 0) + 1
    dense = [c for c, n in cells.items() if n >= 12]
    top = max(hist.values()) / float(ch) if ch else 0.0
    if not dense:
        return ch, blk, 0.0, 0, 0, top
    cx0, cx1 = min(c[0] for c in dense), max(c[0] for c in dense)
    cy0, cy1 = min(c[1] for c in dense), max(c[1] for c in dense)
    edges = set()
    if cx0 == 0: edges.add('l')
    if cx1 == (w - 1) >> 3: edges.add('r')
    if cy0 == 0: edges.add('t')
    if cy1 == (h - 1) >> 3: edges.add('b')
    area = (cx1 - cx0 + 1) * (cy1 - cy0 + 1) / float(((w + 7) >> 3) * ((h + 7) >> 3))
    return ch, blk, area, len(edges), len(dense), top


def upright(im):
    return im.transpose(Image.ROTATE_90).resize((200, 120))


def report():
    meta = json.load(open(META))
    names = {int(k): v for k, v in meta['names'].items()}
    rows = []
    for i in sorted(names):
        nm = names[i]
        p = lambda ph: os.path.join(RAW, '%03d_%s_%s.ppm' % (i, nm, ph))
        st = meta['status'].get(str(i), 'not run')
        if not (os.path.exists(p('base')) and os.path.exists(p('a')) and os.path.exists(p('b'))):
            rows.append((i, nm, st if st != 'ok' else 'no shots', None))
            continue
        base = load(p('base'))
        res = [analyse(base, load(p(ph))) for ph in ('a', 'b')]
        ch = max(r[0] for r in res)
        blk = max(r[1] for r in res)
        area = max(r[2] for r in res)
        edges = max(r[3] for r in res)
        dense = max(r[4] for r in res)
        solid = max(r[5] for r in res if r[0] >= 800) if any(r[0] >= 800 for r in res) else 0.0
        drew = dense >= NOISE_CELLS
        black = blk >= 300 and ch and blk * 100 >= 40 * max(r[0] for r in res if r[1] == blk)
        zc = area > 0.6 or edges >= 3
        rows.append((i, nm, st, (drew, ch, black, blk, zc, area, edges, dense, solid)))
    # contact sheets
    for k in range(0, len(rows), 12):
        sheet = Image.new('RGB', (1200, 4 * 134), (30, 30, 30))
        d = ImageDraw.Draw(sheet)
        for j, (i, nm, st, r) in enumerate(rows[k:k + 12]):
            cx, cy = (j % 3) * 400, (j // 3) * 134
            d.text((cx + 4, cy + 1), '%d %s' % (i, nm), fill=(255, 255, 0))
            for q, ph in enumerate(('a', 'b')):
                f = os.path.join(RAW, '%03d_%s_%s.ppm' % (i, nm, ph))
                if os.path.exists(f):
                    sheet.paste(upright(load(f)), (cx + q * 200, cy + 14))
        sheet.save(os.path.join(OUT, 'sheet_%d.png' % (k // 12 + 1)))
    L = ['# FX1 effect table', '',
         'Made by `python 3ds/tools/fxtest_report.py run all` (game switch `fxtest=all` with `shots`, preset toad, player position, 44 frame slot per effect).',
         'Rules: drew = 15 or more dense cells (8 x 8 cell with 12 or more changed pixels) in shot A or B against the clean shot of the slot. Idle animation alone gives 4 to 12 cells, so weak = at most 14 cells: tiny, late or not drawn, check the sheet. black = at least 300 changed pixels turned near black and at least 40 percent of the changed pixels.',
         'solid colour = at least 800 changed pixels and at least half of them in one colour (4 bit per channel), SOLID marks a flat block. zoomed or clipped = dense cells cover over 60 percent of the frame by bounding box, or touch 3 or more frame edges.',
         'The heuristics flag candidates. The user looks at the contact sheets (3ds/logs/fx/sheet_N.png, not committed).', '',
         '| # | effect | state | drew | changed px | dense cells | black | black px | solid colour % | zoomed or clipped | dense bbox % | edges |', '|---|---|---|---|---|---|---|---|---|---|---|---|']
    nodraw = nblack = nzc = nspawn = nsolid = 0
    for i, nm, st, r in rows:
        if r is None:
            L.append('| %d | %s | %s | - | - | - | - | - | - | - | - | - |' % (i, nm, st))
            continue
        nspawn += 1
        drew, ch, black, blk, zc, area, edges, dense, solid = r
        nodraw += not drew
        nblack += bool(black)
        nzc += bool(zc)
        nsolid += solid >= 0.5
        L.append('| %d | %s | %s | %s | %d | %d | %s | %d | %s | %s | %d | %d |' % (i, nm, st, 'yes' if drew else 'weak', ch, dense, 'YES' if black else 'no', blk, ('SOLID %d' % round(solid * 100)) if solid >= 0.5 else '%d' % round(solid * 100), 'YES' if zc else 'no', round(area * 100), edges))
    L += ['', 'Spawned and shot: %d of %d. Weak or nothing: %d. Black block: %d. Zoomed or clipped: %d. Solid colour block: %d.' % (nspawn, len(rows), nodraw, nblack, nzc, nsolid)]
    open(TABLE, 'w').write('\n'.join(L) + '\n')
    print('\n'.join(L[-1:]))
    print('\n'.join(l for l in L if l.startswith('| ') and ('| NO |' in l or 'YES' in l or 'crash' in l)))


if __name__ == '__main__':
    if sys.argv[1] == 'run':
        arg = sys.argv[2] if len(sys.argv) > 2 else 'all'
        total = len(json.load(open(META))['names']) if False else None
        sel = list(range(FXN)) if arg == 'all' else [int(x) for x in arg.split(',')]
        if arg == 'all' and os.path.exists(META):
            os.remove(META)
        run_all(sel)
    report()
