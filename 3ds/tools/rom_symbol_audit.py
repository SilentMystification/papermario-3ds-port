#!/usr/bin/env python3
"""R01: audit every ROM-address symbol the 3DS build defines. Run from the repo root.
Sources: 3ds/stub_include/*.h macros, --defsym lines in 3ds/CMakeLists.txt, ver/us/splat.yaml (the real offsets).
Per stem (name without _ROM_START/_ROM_END/_VRAM) it prints the START, END and VRAM value the build gives, the splat
range, and a verdict:
  REAL         START and END equal the splat range (a data symbol read by offset, correct)
  START-ONLY   START equals splat, END is a placeholder (the code only uses START as a base, listed for the user check)
  CODE         placeholder, splat segment is type code (code overlay linked in place, START == END, never read)
  DATA?        placeholder, splat segment is not code: must be classified by hand
  BAD          value is not a placeholder and not equal to splat
Exit status 1 when a BAD or DATA? stem exists that is not in R01_VERIFIED (the reviewed list below)."""
import re, sys, glob

ROOT = '.'
# Reviewed in slice R01 (see HANDOFF "R01 result"). stem -> reason. A stem here is not reported as DATA?.
R01_VERIFIED = {
    'entity_model_BombableRock': 'END 0xEA0900 on purpose: the model is 0xE9D650..0xEA0900, splat next start is 0xE9ECD0 (M02).',
    'sprite_shading_profiles_data': 'START = sprite_shading_profiles_ROM_START + 0x1D0 (ver/us/undefined_syms.txt line 11).',
    'sprite_shading_profiles': 'START real (0x315B80), END unused, read as a base by demo_api.c.',
    'imgfx_data': 'START real, END and VRAM unused (imgfx.c adds offsets to START).',
}

sys.path.insert(0, ROOT + '/3ds/tools')
from gen_data_rom import splat_ranges
SPLAT = splat_ranges()

vals = {}  # stem -> {'START':, 'END':, 'VRAM':, 'src':}
def put(name, v, src):
    m = re.match(r'(.+)_(ROM_START|ROM_END|VRAM)$', name)
    if not m: return
    vals.setdefault(m.group(1), {'src': src})[m.group(2)] = v

for f in sorted(glob.glob(ROOT + '/3ds/stub_include/*.h')):
    for n, v in re.findall(r'#define\s+(\w+)\s+\(\(u8\*\)\s*(\w+)\)', open(f).read()):
        put(n, int(v, 0), f.split('/')[-1])
cm = open(ROOT + '/3ds/CMakeLists.txt').read()
for n, v in re.findall(r'--defsym,(\w+)=(\w+)', cm):
    put(n, int(v, 0) if re.match(r'(0x[0-9A-Fa-f]+|\d+)$', v) else 'sym:' + v, 'CMakeLists')

def is_ph(v):
    return v is None or isinstance(v, str) or v <= 1

rows = []
bad = 0
for stem in sorted(vals):
    r = vals[stem]
    s, e, vr = r.get('ROM_START'), r.get('ROM_END'), r.get('VRAM')
    sp = SPLAT.get(stem)
    if sp is None:
        ty = '-'
    else:
        ty = sp[2]
    if not is_ph(s) and not is_ph(e):
        v = 'REAL' if sp and (s, e) == (sp[0], sp[1]) else 'BAD'
    elif not is_ph(s) and is_ph(e):
        v = 'START-ONLY' if sp and s == sp[0] else 'BAD'
    elif s == e or (isinstance(s, str) and s == e):
        v = 'CODE' if (sp is None or ty == 'code') else 'DATA?'
        if stem.startswith('effect_gfx'): v = 'DATA?'
    else:
        v = 'DATA?'
    if stem in R01_VERIFIED and v in ('BAD', 'DATA?', 'START-ONLY'): v = 'VERIFIED'
    rows.append((stem, s, e, vr, sp, ty, v, r['src']))
    if v in ('BAD', 'DATA?'): bad += 1

def h(v): return '-' if v is None else (v if isinstance(v, str) else hex(v))
if '-q' not in sys.argv:
    for stem, s, e, vr, sp, ty, v, src in rows:
        if '-a' not in sys.argv and re.match(r'^(map_|[a-z]{2,3}_\d\d)', stem) and v == 'CODE': continue
        print('%-10s %-44s START %-12s END %-12s VRAM %-10s splat %-22s type %-12s %s' % (
            v, stem, h(s), h(e), h(vr), '-' if not sp else '%x..%x' % (sp[0], sp[1]), ty, src))
from collections import Counter
print(dict(Counter(r[6] for r in rows)), len(rows), 'stems')
sys.exit(1 if bad else 0)
