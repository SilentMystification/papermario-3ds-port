"""Reads a link log on stdin, removes stub definitions that a newly compiled real file now defines.
Handles `STUB(name)` lines and one-line `type name(...) {...}` stubs in the port files."""
import re, sys, pathlib
root = pathlib.Path(__file__).resolve().parents[1] / "src"
names = set(re.findall(r"multiple definition of `(\w+)'", sys.stdin.read()))
files = ["pm_actor_stubs.c", "pm_evt_stub.c", "pm_later.c", "pm_kmr_stubs.c", "game_stubs.c", "pm_actor_link.c"]
for f in files:
    p = root / f
    lines = p.read_text().split("\n")
    keep = []
    for l in lines:
        m = re.match(r"STUB\((\w+)\)$", l)
        if m and m[1] in names:
            continue
        m = re.match(r"^[\w\*\s]+?\b(\w+)\([^)]*\)\s*\{.*\}\s*$", l)
        if m and m[1] in names and l.count("{") == 1:
            continue
        m = re.match(r"^[\w\*\s]+?\b(\w+)(\[[^\]]*\])?(\s*=.*)?;$", l)
        if m and m[1] in names and "(" not in l:
            continue
        keep.append(l)
    p.write_text("\n".join(keep))
print(sorted(names))
