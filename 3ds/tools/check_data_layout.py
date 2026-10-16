#!/usr/bin/env python3
"""D02: check that variables the decomp reads as one block are adjacent, in source order, in the linked ELF.
Usage: arm-none-eabi-nm -n -S pm_3ds.elf > nm.txt ; python 3ds/tools/check_data_layout.py nm.txt [--scan]
--scan also greps src/ for neighbour-read patterns (&(&X)[n], *(&X +- n), (T*)&X)[n], &X < &Y)."""
import re, sys, pathlib
# adjacency groups, in source order (file, symbols). Add a row when a scan finds a new dependency.
GROUPS = [
    ("src/world/actions.c (7B440.c reads &DefaultMoveSpeed[-1])", ["DefaultWalkSpeed", "DefaultMoveSpeed", "D_800F7B78"]),
    ("src/evt/script_list.c (demo_api.c SetSpriteShading, DMA 8 B and 256 B)",
     ["ShadingOffsetsBuffer", "D_802DBB64", "PackedShadingData", "D_802DB8B6C", "D_802DBB70"]),
]
SCAN = [r"\(&\s*\w+\s*\)\s*\[", r"\*\s*\(\s*&\s*\w+\s*[-+]", r"&\s*[A-Za-z_]\w*\s*[-+]\s*\d+\b",
        r"\(\s*\(\s*\w+\s*\*\s*\)\s*&\s*\w+\s*\)\s*\[", r"&\s*\w+\s*[<>]=?\s*&\s*\w+"]
syms = {}
for l in open(sys.argv[1]):
    p = l.split()
    if len(p) == 4: syms[p[3]] = (int(p[0], 16), int(p[1], 16))
bad = 0
for name, g in GROUPS:
    miss = [s for s in g if s not in syms]
    if miss: print(f"MISSING  {name}: {miss}"); bad += 1; continue
    ok = all(syms[a][0] + syms[a][1] == syms[b][0] for a, b in zip(g, g[1:]))
    print(("ok       " if ok else "BROKEN   ") + name)
    for s in g: print(f"           {syms[s][0]:08x} +{syms[s][1]:<4x} {s}")
    bad += not ok
if "--scan" in sys.argv:
    rx = re.compile("|".join(SCAN))
    for f in sorted(pathlib.Path("src").rglob("*.c")):
        for n, l in enumerate(f.read_text(errors="ignore").splitlines(), 1):
            if rx.search(l) and not l.lstrip().startswith("//"): print(f"scan {f}:{n}: {l.strip()}")
sys.exit(1 if bad else 0)
