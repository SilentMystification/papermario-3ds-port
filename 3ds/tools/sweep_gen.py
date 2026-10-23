#!/usr/bin/env python3
"""sweep_gen.py - lists the boss formations of the decomp for the full sweep (docs/plans/sweep_notes.md).
Reads src/battle/area/NAME/area.c (BattleList, Formation_nn actors, StageList) and src/battle/battle_names.h.
A formation is a boss formation when one actor blueprint matches BOSS below. Prints one sweep.txt line per formation:
  <name> kind=boss battle=<BTL formation>:<BTL stage> story=<n> cap=<steps> note=<boss actor>
Use: python 3ds/tools/sweep_gen.py [bosses|cutscenes] > block.txt  (then merge into 3ds/sweep/sweep.txt).
  python 3ds/tools/sweep_gen.py bosstable    writes 3ds/src/pm_boss_table.h, the one source of the boss list of the Select menu tab Boss (no boss lines in sweep.txt).
Mode cutscenes: finds `CaseEq(<map>_ENTRY_<n>)` followed by an `Exec(N(EVS_Scene_...))` or `MV_Scene_` in src/world/area_*/<map>/*.c (the entry scripts of the maps) and prints
  cut_<map>_e<n>_<scene> kind=cut warp=<map>:<n> story=<value of the chapter of the area> cap=<steps>
The story value is only a start state of the chapter (table AREA_STORY). A scene that needs an exact story value or a game flag is listed as it plays: the report shows an empty or short scene."""
import os, re, sys
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
BOSS = re.compile(r'goomba_king|tubba_blubba|tutankoopa|monty_mole_boss|huff_n_puff|lava_piranha|jr_troopa|kammy_koopa|ninja_koopa|bowser|crystal_king|blooper|general_guy|kent_c_koopa|koopa_bros|fuzzipede|buzzar|tubbas_heart')
# story value that fits the chapter of the area (the numbers are in include/enums.h). Only a start state, the battle does not read it.
STORY = {'kmr_2': -112, 'kmr_3': -118, 'nok': -100, 'trd_2': -78, 'trd_3': -78, 'isk_2': -56, 'tik2': -20, 'arn': -16, 'dgb': -16, 'dig': -16,
         'omo2': 3, 'kgr': 10, 'kzn2': 35, 'flo2': 56, 'pra2': 86, 'kpa2': 95, 'kkj': -108, 'iwa': -69}
AREA_STORY = {'kmr': -107, 'mac': -107, 'osr': -105, 'hos': -105, 'kkj': -108, 'nok': -100, 'trd': -90, 'dro': -69, 'sbk': -69, 'isk': -60, 'iwa': -72, 'mim': -48, 'obk': -40, 'arn': -20,
              'dgb': -20, 'omo': -3, 'flo': 50, 'jan': 20, 'kzn': 25, 'sam': 70, 'pra': 78, 'kpa': 95, 'tik': -25, 'kgr': 10, 'end': 96}
SPECIAL = {('kmr_20', 0): 'intro', ('kkj_00', 6): 'epilogue', ('kmr_20', 1): -106, ('kmr_20', 2): 'epilogue', ('kmr_20', 3): 'epilogue'}
def cutscenes():
    rows = []
    wb = os.path.join(root, 'src/world')
    for area in sorted(os.listdir(wb)):
        if not area.startswith('area_') or area == 'area_end': continue
        for m in sorted(os.listdir(os.path.join(wb, area))):
            md = os.path.join(wb, area, m)
            if not os.path.isdir(md) or not re.match(r'^[a-z]+_\d+$', m): continue
            seen = set()
            for f in sorted(os.listdir(md)):
                if not f.endswith('.c'): continue
                lines = open(os.path.join(md, f), errors='replace').read().split('\n')
                for i, ln in enumerate(lines):
                    mm = re.search(r'CaseEq\(%s_ENTRY_(\d+)\)' % m, ln)
                    if not mm: continue
                    for ln2 in lines[i + 1:i + 8]:
                        if 'CaseEq' in ln2 or 'CaseDefault' in ln2: break
                        sc = re.search(r'(?:EVS|MV)_Scene_(\w+)', ln2)
                        if sc:
                            key = (m, int(mm.group(1)), sc.group(1))
                            if key in seen: break
                            seen.add(key)
                            st = SPECIAL.get((m, key[1]), AREA_STORY.get(m.split('_')[0], 0))
                            rows.append('cut_%s_e%d_%s kind=cut warp=%s:%d story=%s cap=1200' % (m, key[1], sc.group(1)[:24], m, key[1], st))
                            break
    return rows
mode = sys.argv[1] if len(sys.argv) > 1 else 'bosses'
if mode == 'cutscenes':
    r = cutscenes()
    print('\n'.join(r)); print('%d cutscene entries' % len(r), file=sys.stderr); sys.exit(0)
names = open(os.path.join(root, 'src/battle/battle_names.h'), encoding='utf-8', errors='replace').read()
mode = sys.argv[1] if len(sys.argv) > 1 else 'bosses'
rows = []
base = os.path.join(root, 'src/battle/area')
for d in sorted(os.listdir(base)):
    af = os.path.join(base, d, 'area.c')
    if not os.path.isfile(af): continue
    pre = d.upper().replace('_PART_', '_')
    t = open(af, encoding='utf-8', errors='replace').read()
    forms = {}
    for m in re.finditer(r'Formation\s+A\(Formation_(\w+)\)\s*=\s*\{(.*?)\n\};', t, re.S):
        forms[m.group(1)] = re.findall(r'ACTOR_BY_(?:POS|IDX)\(A\((\w+)\)', m.group(2))
    sl = re.search(r'StageList\s+A\(Stages\)\s*=\s*\{(.*?)\n\};', t, re.S)
    stages = re.findall(r'STAGE\("[^"]*",\s*A\((\w+)\)', sl.group(1)) if sl else []
    snames = re.findall(r'STAGE\("([^"]*)"', sl.group(1)) if sl else []
    bl = re.search(r'BattleList\s+A\(Formations\)\s*=\s*\{(.*?)\n\};', t, re.S)
    if not bl: continue
    for i, m in enumerate(re.finditer(r'BATTLE\(A\(Formation_(\w+)\),\s*A\((\w+)\)', bl.group(1))):
        f, st = m.group(1), m.group(2)
        actors = forms.get(f, [])
        hit = [a for a in actors if BOSS.search(a)]
        if not hit: continue
        bf = 'BTL_%s_FORMATION_%02X' % (pre, i)
        si = stages.index(st) if st in stages else (snames.index(st) if st in snames else 0) # a stage that is not in the list: stage 00 (the arena is only a backdrop)
        bs = 'BTL_%s_STAGE_%02X' % (pre, si)
        if bf not in names or bs not in open(os.path.join(root, 'src/battle/stage_names.h'), encoding='utf-8', errors='replace').read(): continue
        key = pre.lower()
        rows.append('boss_%s_f%02X kind=boss battle=%s:%s story=%d cap=1500 note=%s' % (hit[0], i, bf, bs, STORY.get(key, 0), '+'.join(actors)))
if mode == 'bosstable':
    out = []
    for r in rows:
        t = r.split()
        kv = dict(x.split('=', 1) for x in t[1:] if '=' in x)
        m = re.match(r'boss_(.+)_f([0-9A-F]+)$', t[0])
        lab = (m.group(1).replace('_', ' ').title().replace('Jr ', 'Jr. ') + ' ' + m.group(2))[:22]
        out.append('    { "%s", "%s", %s },' % (lab, kv['battle'], kv['story']))
    hdr = ('/* The boss formations of the Boss tab of the Select menu: label, battle=<formation>:<stage> (names of battle_names.h and stage_names.h), story value of the area.\n'
           ' * Written by 3ds/tools/sweep_gen.py bosstable from the decomp (src/battle/area/NAME/area.c). Do not edit. */\n')
    open(os.path.join(root, '3ds', 'src', 'pm_boss_table.h'), 'w', newline='\n').write(hdr + '\n'.join(out) + '\n')
    print('3ds/src/pm_boss_table.h: %d bosses' % len(out), file=sys.stderr)
else:
    print('\n'.join(rows))
print('%d boss formations' % len(rows), file=sys.stderr)

