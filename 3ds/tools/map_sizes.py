#!/usr/bin/env python3
"""MEM: size report of a GNU ld map (pm_3ds.map).

Usage: python 3ds/tools/map_sizes.py [map] [--top N]
Default map: build3ds/pm_3ds.map. Prints the output-section totals (.text, .rodata, .mapstat,
.data, .bss, file-stored total), the split of .text and .data by owner (game, port, libs), and the
top N input sections of .data and .bss by size. Run it before and after a size change; the numbers
go into the commit message and the bench database, not into prose.
"""
import re, sys, os

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
SEC = re.compile(r'^ (\.\S+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)(?:\s+(\S.*))?$')
SEC_LONG = re.compile(r'^ (\.\S+)$')
VAL = re.compile(r'^\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S.*)$')
OUT = re.compile(r'^(\.\S+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s*$')
FILE_STORED = ('.text', '.rodata', '.mapstat', '.data', '.tdata', '.preinit_array', '.init_array', '.fini_array', '.ARM.exidx', '.ARM.extab')

def owner(path):
    p = path.replace(chr(92), '/')
    if '.a(' in p or p.endswith('.a') or '/libs/' in p or 'devkitpro' in p.lower() or 'libctru' in p or 'libcitro' in p:
        return 'libs'
    if '/3ds/src/' in p or '/3ds/' in p or p.startswith('3ds/') or 'pm_' in os.path.basename(p) or 'nd_' in os.path.basename(p):
        return 'port'
    return 'game'

def parse(path):
    secs = {}   # output name -> (addr, size)
    inputs = []  # (outsec, size, file, insec)
    cur_out = None
    in_map = False
    pending = None
    for line in open(path, errors='replace'):
        line = line.rstrip('\n')
        if line.startswith('Linker script and memory map'):
            in_map = True
            continue
        if not in_map:
            continue
        m = OUT.match(line)
        if m:
            cur_out = m.group(1)
            secs[cur_out] = (int(m.group(2), 16), int(m.group(3), 16))
            pending = None
            continue
        if line.startswith('.') and ' 0x' in line and not line.startswith(' '):
            cur_out = line.split()[0]
            pending = None
            continue
        m = SEC.match(line)
        if m and cur_out:
            inputs.append((cur_out, int(m.group(3), 16), (m.group(4) or '').strip(), m.group(1)))
            pending = None
            continue
        m = SEC_LONG.match(line)
        if m and cur_out:
            pending = m.group(1)
            continue
        if pending:
            m = VAL.match(line)
            if m:
                inputs.append((cur_out, int(m.group(2), 16), m.group(3).strip(), pending))
            pending = None
    return secs, inputs

def main():
    args = sys.argv[1:]
    top = 30
    if '--top' in args:
        i = args.index('--top'); top = int(args[i + 1]); del args[i:i + 2]
    mp = args[0] if args else os.path.join(ROOT, 'build3ds', 'pm_3ds.map')
    secs, inputs = parse(mp)
    print('map', mp)
    stored = 0
    for name in ('.text', '.rodata', '.mapstat', '.data', '.bss'):
        if name in secs:
            print('%-10s %10d B  %8.3f MB' % (name, secs[name][1], secs[name][1] / 1048576.0))
    for name in FILE_STORED:
        if name in secs:
            stored += secs[name][1]
    print('file-stored (PROGBITS sections, no headers) %d B  %.3f MB' % (stored, stored / 1048576.0))
    tot = {}
    for out, size, f, insec in inputs:
        tot[(out, owner(f))] = tot.get((out, owner(f)), 0) + size
    for out in ('.text', '.data', '.bss', '.mapstat'):
        row = ['%s %.3f' % (o, tot.get((out, o), 0) / 1048576.0) for o in ('game', 'port', 'libs')]
        print(out, ' | '.join(row), 'MB')
    for out in ('.data', '.bss'):
        rows = sorted([r for r in inputs if r[0] == out], key=lambda r: -r[1])[:top]
        print('--- top %d input sections of %s' % (top, out))
        for o, size, f, insec in rows:
            print('%9d  %-40s %s' % (size, insec, f))

if __name__ == '__main__':
    main()
