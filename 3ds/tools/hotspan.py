#!/usr/bin/env python3
"""hotspan.py: bytes of the hot code span from the link map (PERF2 step 4).

The hot span is the part of .text that 3ds/ld/3dsx.ld.in places by name before *(.text.unlikely.*): the walker,
the batch close and the citro3d calls of one draw. It must fit the 16 KB L1 I-cache of the ARM11.

usage: python 3ds/tools/hotspan.py [map]   (default build3ds/pm_3ds.map; a lane: build3ds/lane-<lane>/pm_3ds.map)
       -v lists each function of the span with its size, largest first.
Prints: hotspan <bytes> B (<n> functions, first 0x.. end 0x..), and the bytes per linker group line.
"""
import re
import sys


def parse(path):
    lines = open(path, encoding="utf-8", errors="replace").read().split("\n")
    i = next(k for k, l in enumerate(lines) if l.startswith(".text ") or l.startswith(".text\t") or l.strip() == ".text")
    groups, cur, fns, lo, hi = [], None, [], None, None
    k = i + 1
    pend = None
    while k < len(lines):
        l = lines[k]
        if l.startswith(" *(") or l.startswith(" *lib") or l.startswith(" *"):
            if ".text.unlikely" in l:
                break
            if "(.text." in l:
                cur = [l.strip()[:90], 0]
                groups.append(cur)
            else:
                cur = None
            k += 1
            continue
        m = re.match(r"^ (\.text\.\S+)\s*(0x[0-9a-f]+)?\s*(0x[0-9a-f]+)?", l)
        if m and cur is not None:
            name = m.group(1)
            if m.group(2) is None:  # the address and size are on the next line
                m2 = re.match(r"^\s+(0x[0-9a-f]+)\s+(0x[0-9a-f]+)", lines[k + 1])
                addr, size = int(m2.group(1), 16), int(m2.group(2), 16)
                k += 1
            else:
                addr, size = int(m.group(2), 16), int(m.group(3), 16)
            if size:
                fns.append((name[len(".text."):], size, addr))
                cur[1] += size
                lo = addr if lo is None else min(lo, addr)
                hi = addr + size if hi is None else max(hi, addr + size)
        k += 1
    return groups, fns, lo, hi


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    path = args[0] if args else "build3ds/pm_3ds.map"
    groups, fns, lo, hi = parse(path)
    span = (hi - lo) if fns else 0
    print("hotspan %d B (%d functions, first 0x%x end 0x%x, sum %d B)" % (span, len(fns), lo or 0, hi or 0, sum(f[1] for f in fns)))
    for g, b in groups:
        print("  %6d B  %s" % (b, g))
    if "-v" in sys.argv:
        for n, s, a in sorted(fns, key=lambda f: -f[1]):
            print("  %6d  0x%x  %s" % (s, a, n))


if __name__ == "__main__":
    main()
