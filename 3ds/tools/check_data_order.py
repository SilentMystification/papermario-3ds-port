#!/usr/bin/env python3
"""H06c: check that data symbols of decomp game code keep source (ROM) order in the link.
Usage (repo root): python 3ds/tools/check_data_order.py [build3ds/pm_3ds.map]
For each object file and each kind (.data, .rodata, .bss) it walks the kept input sections in link order and
compares the position of each symbol definition in the .c file. A descent is a mismatch (a symbol sits earlier in
the source than the symbol before it in the link). Symbols with no definition in the .c file (inc.c files, macro
names, static locals) are counted as unresolved and not checked.
const data (.rodata) and non-const data (.data, .bss) are separate output sections: they are reported apart."""
import re, sys, collections, pathlib

args = [a for a in sys.argv[1:] if not a.startswith("--")]
txt = open(args[0] if args else "build3ds/pm_3ds.map", errors="ignore").read()
rx = re.compile(r"^ \.(data|rodata|bss)\.(\S+)\s+(0x[0-9a-f]+)\s+(0x[0-9a-f]+)\s+(\S+\.o)\s*$", re.M)
byobj = collections.OrderedDict()
for m in rx.finditer(txt):
    kind, name, addr, size, obj = m.groups()
    if "." in name or int(size, 16) == 0 or int(addr, 16) == 0:  # address 0 = dropped section
        continue
    byobj.setdefault(obj, collections.defaultdict(list))[kind].append(name)

HEAD = r"^(?!\s|extern|//|#)[^\n]*?"
TAIL = r"\s*(\[[^\]\n]*\]\s*)*(=|;)"


def pos(t, name):
    # the map name may carry a namespace prefix (N(rest) in the source): try the name, then N(rest) for each prefix cut
    m = re.search(r"^(?!\s|extern|//|#)[^\n(]*?\b" + re.escape(name) + r"\b" + TAIL, t, re.M)
    parts = name.split("_")
    for i in range(1, min(len(parts), 5)):
        if m:
            break
        m = re.search(HEAD + r"N\(" + re.escape("_".join(parts[i:])) + r"\)" + TAIL, t, re.M)
    return m.start() if m else None


# Checked by hand 2026-10-19 (H06c): the game reads these symbols by name, never by adjacency. Not counted as mismatches.
BY_NAME = {"src/world/area_hos/hos_01/model_fx.c": "two s32 alpha values, set and read by name",
           "src/world/area_hos/hos_05/intro.c": "IntroCamState floats, passed as &Name to the lerp calls, read by name",
           "src/menu_hud_scripts.c": "duplicate definitions in version ifdefs",
           "src/msg_data.c": "duplicate definitions in version ifdefs"}
tot = collections.Counter()
bad = collections.defaultdict(list)
skip = 0
for obj, kinds in byobj.items():
    rel = obj.split("/work/src/", 1)[-1]
    src = pathlib.Path(rel[:-2] + ".c")
    if not src.is_file() or src.parts[0] == "3ds":  # port layer is not checked
        continue
    t = src.read_text(errors="ignore")
    for kind, names in kinds.items():
        last = {}
        for n in names:
            p = pos(t, n)
            tag = "src"
            a = re.match(r"D_([0-9A-F]{8})_", n)
            if p is None and a:  # defined in an included .inc.c or by a macro: the ROM address in the name gives the order
                p, tag = int(a.group(1), 16), "addr"
            if p is None:
                skip += 1
                continue
            tot[kind] += 1
            if tag in last and p < last[tag][0]:
                bad[(str(src), kind)].append((last[tag][1], n))
            if tag not in last or p > last[tag][0]:
                last[tag] = (p, n)
print("checked:", dict(tot), "unresolved:", skip)
nm = collections.Counter()
for (f, k), v in sorted(bad.items()):
    if f.replace("\\", "/") in BY_NAME:
        continue
    nm[k] += len(v)
    print(f"MISMATCH {k:6} {len(v):3} {f}  e.g. {v[0][0]} before {v[0][1]}")
print("mismatches:", dict(nm), "files:", len({f for f, _ in bad if f.replace("\\", "/") not in BY_NAME}))
