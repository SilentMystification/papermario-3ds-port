#!/usr/bin/env python3
"""M01: rom_trace_check.py full.txt [packed.txt]. Reports reads into dropped ranges. With two logs, compares the
set of (addr,size,checksum) reads and lists differences."""
import sys
import re
D=[(int(a,16),int(b,16)) for a,b in re.findall(r'\{ 0x([0-9A-F]+), 0x([0-9A-F]+), "', open('3ds/src/rom_remap.h').read())]  # M02: read from the generated header
def load(p):
    r=[]
    for l in open(p):
        t=l.split()
        if len(t)==3 and len(t[2])==8: r.append((int(t[0],16),int(t[1]),t[2]))
    return r
a=load(sys.argv[1])
hit=[x for x in a if any(x[0]<e and x[0]+x[1]>s for s,e in D)]
print(sys.argv[1],len(a),'reads',len(set(a)),'distinct',len(hit),'in dropped ranges')
for x in hit[:10]: print('  HIT %08x %d'%x[:2])
if len(sys.argv)>2:
    b=load(sys.argv[2]); sa,sb=set(a),set(b)
    print(sys.argv[2],len(b),'reads',len(sb),'distinct')
    print('only in',sys.argv[1],len(sa-sb),'only in',sys.argv[2],len(sb-sa))
    for x in sorted(sa-sb)[:8]: print('  A-B %08x %d %s'%x)
    for x in sorted(sb-sa)[:8]: print('  B-A %08x %d %s'%x)
