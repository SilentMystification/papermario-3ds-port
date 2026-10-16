#!/usr/bin/env python3
"""M02: static check. Every ROM range the port names with a real offset (ld_addrs.h, entity_model_offsets.h, the
CMake defsyms, the asset table region) must not overlap a dropped range in rom_remap.h. Run from the repo root."""
import re, sys
D = [(int(a, 16), int(b, 16), n) for a, b, n in re.findall(r'\{ 0x([0-9A-F]+), 0x([0-9A-F]+), "([^"]*)" \}', open('3ds/src/rom_remap.h').read())]
R = {}  # name -> [start, end]
for f in ['3ds/stub_include/ld_addrs.h', '3ds/stub_include/entity_model_offsets.h', '3ds/stub_include/effect_ld_addrs.h']:
    for n, v in re.findall(r'#define (\w+_ROM_(?:START|END))\s+\(\(u8\*\)(0x[0-9A-Fa-f]+|\d+)\)', open(f).read()):
        v = int(v, 0)
        if v > 1: R.setdefault(n.rsplit('_ROM_', 1)[0], {})[n.rsplit('_', 1)[1]] = v
# defsyms with a real offset (CMake), length from splat: sprite_shading_profiles is 0x315B80..0x3169F0
R['sprite_shading_profiles'] = {'START': 0x315B80, 'END': 0x3169F0}
bad = 0
for n, r in sorted(R.items()):
    s = r.get('START'); e = r.get('END', (s or 0) + 1)
    if s is None: continue
    for ds, de, dn in D:
        if s < de and e > ds: print('OVERLAP', n, hex(s), hex(e), 'with', hex(ds), hex(de), dn); bad += 1
print(len(R), 'named ranges checked,', bad, 'overlap a dropped range')
sys.exit(1 if bad else 0)
