#!/usr/bin/env python3
"""MS01: static data of the linked maps, from the linker map (-Wl,-Map, build3ds/pm_3ds.map).
Usage: map_data_sizes.py <map file> [ranges.txt]
  prints bytes per group (map dir src/world/area_X/Y, shared dirs, rest) for .data and .bss
  ranges.txt (optional) is the input of the runtime probe (debug switch dataprobe, pm_mapdata.c):
  lines "<map id|@machi|@dgb> <hex addr> <size> <d|b> <symbol>".
Also a module: sections(mapfile) gives dicts {group, owner, kind, sec, sym, addr, size, file}."""
import re, sys, collections

def group(f):
    mm = re.search(r'/src/world/(area_\w+/\w+)/[^/]+\.o$', f)
    if mm: return mm.group(1)
    mm = re.search(r'/src/world/([\w/]+?)/[^/]+\.o$', f)
    if mm: return 'world/' + mm.group(1)
    return 'other'

def owner(g):
    if g.startswith('area_'):
        i = g.split('/')[1]
        return '@machi' if i == 'machi' else i
    return '@dgb' if g == 'world/area_dgb' else None

def sections(path):
    rows, pend = [], None
    sec_re = re.compile(r'^ (\.\S+)\s*$')
    full_re = re.compile(r'^ (\.\S+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+)')
    cont_re = re.compile(r'^\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+)')
    for ln in open(path, errors='replace'):
        m = full_re.match(ln)
        if m:
            rows.append((m.group(1), int(m.group(2), 16), int(m.group(3), 16), m.group(4))); pend = None; continue
        m = sec_re.match(ln)
        if m: pend = m.group(1); continue
        m = cont_re.match(ln)
        if m and pend:
            rows.append((pend, int(m.group(1), 16), int(m.group(2), 16), m.group(3))); pend = None; continue
        pend = None
    out = []
    for s, a, z, f in rows:
        if not (s.startswith('.data') or s.startswith('.bss')) or z == 0 or a == 0: continue
        g = group(f)
        plain = s in ('.data', '.bss')
        base = f.split('/')[-1][:-2]
        out.append(dict(group=g, owner=owner(g), kind='d' if s.startswith('.data') else 'b', sec=s,
                        sym=(base + s.replace('.', '_')) if plain else s.split('.', 2)[2],
                        addr=a, size=z, file=f.split('/work/src/')[-1]))
    return out

if __name__ == '__main__':
    out = sections(sys.argv[1])
    grp = collections.Counter()
    for r in out: grp[(r['group'], r['kind'])] += r['size']
    for k, v in sorted(grp.items(), key=lambda x: -x[1])[:12]: print(v, *k)
    md = {k: v for k, v in grp.items() if k[0].startswith('area_') and k[1] == 'd'}
    mb = {k: v for k, v in grp.items() if k[0].startswith('area_') and k[1] == 'b'}
    print('map .data: %d groups %d bytes (+ shared area_dgb %d)' % (len(md), sum(md.values()), grp[('world/area_dgb', 'd')]))
    print('map .bss: %d groups %d bytes (+ shared area_dgb %d)' % (len(mb), sum(mb.values()), grp[('world/area_dgb', 'b')]))
    if len(sys.argv) > 2:
        with open(sys.argv[2], 'w') as f:
            for r in out:
                if r['owner']: f.write('%s %x %d %s %s\n' % (r['owner'], r['addr'], r['size'], r['kind'], r['sym'][:47]))
