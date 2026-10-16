#!/usr/bin/env python3
"""M01: rom_trace_check.py full.txt [packed.txt]. Reports reads into dropped ranges. With two logs, compares the
set of (addr,size,checksum) reads and lists differences."""
import sys
D=[(0x800880,0xE20110),(0xE9ECD0,0xF00000),(0x1942C40,0x1943000),(0x1B8220C,0x1B83000),(0x1C84D30,0x1E40000),(0x27FEE22,0x2800000)]
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
