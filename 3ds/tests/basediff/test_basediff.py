# BASEKEEP host test: bench.shots_diff_all lists every differing shot with its pixel count and bbox.
# Run from the repo root (run.sh does that).
import sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent.parent
sys.path.insert(0, str(ROOT / "3ds" / "tools"))
import bench  # bench.py guards main, so the import runs nothing

fails = []


def check(name, ok):
    print("%s: %s" % ("PASS" if ok else "FAIL", name))
    if not ok:
        fails.append(name)


def ppm(path, px):
    """2 x 2 P6 file; px is 12 bytes of RGB."""
    path.write_bytes(b"P6 2 2 255\n" + bytes(px))


base = bytes(range(12))

with tempfile.TemporaryDirectory() as tmp:
    a, b, c = Path(tmp, "a"), Path(tmp, "b"), Path(tmp, "c")
    for d in (a, b, c):
        d.mkdir()
        for n in ("0001.ppm", "0002.ppm", "0003.ppm"):
            ppm(d / n, base)
    # b: one pixel changed in 0001 and in 0002 (pixel 0 red channel: +1)
    changed = bytearray(base)
    changed[0] += 1
    ppm(b / "0001.ppm", changed)
    changed2 = bytearray(base)
    changed2[9] += 1  # pixel 3 (x 1, y 1), blue channel
    ppm(b / "0002.ppm", changed2)

    got = bench.shots_diff_all(str(a), str(b))
    check("a: two differing shots give exactly two entries", len(got) == 2)
    check("a: entries are 0001.ppm and 0002.ppm", [g[0] for g in got] == ["0001.ppm", "0002.ppm"])
    check("a: each entry has px 1", all(g[1].startswith("px 1 ") for g in got))
    check("a: 0002.ppm bbox is x1..1 y1..1", got[1][1] == "px 1 bbox x1..1 y1..1")
    check("b: equal folders give empty list", bench.shots_diff_all(str(a), str(c)) == [])
    check("c: same folder gives empty list", bench.shots_diff_all(str(a), str(a)) == [])
    Path(tmp, "b", "0003.ppm").unlink()
    miss = bench.shots_diff_all(str(a), str(b))
    check("d: missing in b is named", ("0003.ppm", "missing in b") in miss)

if fails:
    print("FAIL: %d case(s)" % len(fails))
    sys.exit(1)
print("PASS: all cases")
