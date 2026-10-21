#!/usr/bin/env python3
"""dlsrc_report.py LOG: table of the dlsrc and dlsrc2 line pairs (DLC0). Per pair: per drawn frame ops, triangles and vertex loads by list class and the static share (classes shape, texheap, rodata)."""
import re, sys
C = "main shape texh rodata entity genh sprh other".split()
def nums(s): return [int(x) for x in s.split()]
lines = open(sys.argv[1], errors="replace").read().splitlines()
for i, ln in enumerate(lines):
    m = re.search(r"dlsrc (\d+) (\d+):(\d+) n (\d+) ops \| (.*)", ln)
    if not m: continue
    mode, a, mp, n = (int(x) for x in m.groups()[:4])
    f = [nums(p) for p in m.group(5).split("|")]
    m2 = next((re.search(r"dlsrc2 .* n \d+ tri ([\d ]+)\| trivs ([\d ]+)\| vtx ([\d ]+)\| dl (\d+) stable (\d+)/(\d+)", l) for l in lines[i + 1:i + 4] if "dlsrc2" in l), None)
    if not m2: continue
    tri, triv, vtx = nums(m2.group(1)), nums(m2.group(2)), nums(m2.group(3))
    ops = [sum(f[k][j] for k in range(3)) for j in range(8)]
    st = lambda v: sum(v[1:4]) / max(1, sum(v))
    print(f"mode {mode} {a}:{mp} n {n}: ops/frame {sum(ops)/n:.0f} (geo {sum(f[0])/n:.0f} flow {sum(f[1])/n:.0f} state {sum(f[2])/n:.0f}) static {st(ops)*100:.0f}% | tri {sum(tri)/n:.0f} static {st(tri)*100:.0f}% (by vertex class {st(triv)*100:.0f}%) | vtx {sum(vtx)/n:.0f} static {st(vtx)*100:.0f}% | dl {int(m2.group(4))/n:.1f}/frame stable {m2.group(5)}/{m2.group(6)}")
    print("   ops/frame by class: " + " ".join(f"{C[j]} {ops[j]/n:.0f}" for j in range(8)))
