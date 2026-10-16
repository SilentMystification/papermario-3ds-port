"""Check 3ds/src/area_table.c against src/world/world.c (S22b).
Compares, per area: the map array name, the map ids in order, and the extra fields
(.bgName, .songVariation, .sfxReverb ...). MAP/MAP_ID/MAP_WITH_INIT are all read as the id.
Also compares the AREA() rows of gAreas. Exit 1 on any difference."""
import re, sys, os
root = os.path.join(os.path.dirname(__file__), "..", "..")
def rd(p):
    return re.sub(r"//[^\n]*", "", open(os.path.join(root, p), encoding="utf8", errors="replace").read())
def maps(t):
    out = {}
    for m in re.finditer(r"MapConfig\s+(\w+)\[\]\s*=\s*\{(.*?)\n\};", t, re.S):
        rows = []
        for r in re.finditer(r"\{\s*MAP(?:_ID|_WITH_INIT)?\((\w+)\)\s*,?(.*?)\}\s*,", m.group(2), re.S):
            rows.append((r.group(1), " ".join(r.group(2).split())))
        out[m.group(1)] = rows
    return out
def areas(t):
    m = re.search(r"AreaConfig\s+gAreas\[\]\s*=\s*\{(.*?)\n\};", t, re.S)
    return re.findall(r"AREA\(([^)]*)\)", m.group(1)) if m else None
w, p = rd("src/world/world.c"), rd("3ds/src/area_table.c")
wm, pm = maps(w), maps(p)
bad = 0
for k in sorted(set(wm) | set(pm)):
    if wm.get(k) != pm.get(k):
        bad += 1
        a, b = wm.get(k) or [], pm.get(k) or []
        print("DIFF", k, "world rows", len(a), "port rows", len(b))
        for x, y in zip(a, b):
            if x != y: print("   world", x, "port", y)
wa, pa = areas(w), areas(p)
if wa is None or pa is None: print("gAreas not found (cut at AREA?)"); bad += 1
elif wa != pa: print("DIFF gAreas AREA() rows"); bad += 1
print("arrays", len(wm), "maps", sum(map(len, wm.values())), "differences", bad)
sys.exit(1 if bad else 0)
