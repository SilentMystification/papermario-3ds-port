# FRAMEHASH4 host test: bench.fhash_extract, bench.shot_names, bench.fhash_cmp. Run from the repo root (run.sh does that).
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


H = ["0123456789abcdef", "fedcba9876543210", "0f0f0f0f0f0f0f0f"]
LOG = "\n".join([
    "boot ok",
    "fhash f10 h %s draws 12 words 340" % H[0],
    "some other line with f10 but no hash",
    "fhash f11 h %s draws 13 words 350" % H[1],
    "fhash f12 h %s draws 14 words 360" % H[2],
    "end",
]) + "\n"
WANT = ["f10 h %s draws 12 words 340" % H[0], "f11 h %s draws 13 words 350" % H[1], "f12 h %s draws 14 words 360" % H[2]]

with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    log = tmp / "run.log"
    log.write_text(LOG, encoding="utf-8")

    # (a) extract: 3 entries, exact lines, file written
    d1 = tmp / "shots1"
    n = bench.fhash_extract(log, d1)
    check("a: fhash_extract returns 3 for a log with 3 fhash entries", n == 3)
    got = (d1 / "fhash.txt").read_text(encoding="utf-8").split("\n")[:-1] if (d1 / "fhash.txt").is_file() else []
    check("a: fhash.txt holds the exact 3 lines", got == WANT)

    # (b) shot_names of a folder with only fhash.txt
    check("b: shot_names of the folder == ['fhash.txt']", bench.shot_names(d1) == ["fhash.txt"])

    # (c) two equal files
    d2 = tmp / "shots2"
    bench.fhash_extract(log, d2)
    bad, lines = bench.fhash_cmp(d1 / "fhash.txt", d2 / "fhash.txt")
    check("c: equal files give bad 0", bad == 0)
    check("c: equal files end with '3/3 frames equal'", lines[-1] == "3/3 frames equal")

    # (d) one hash changed in frame 2 of 3
    d3 = tmp / "shots3"
    bench.fhash_extract(log, d3)
    changed = (d3 / "fhash.txt").read_text(encoding="utf-8").replace(H[1], "00000000000000ff")
    (d3 / "fhash.txt").write_text(changed, encoding="utf-8")
    bad, lines = bench.fhash_cmp(d1 / "fhash.txt", d3 / "fhash.txt")
    check("d: one changed hash gives bad 1", bad == 1)
    check("d: DIFF line names frame 2 and the draw", "  f11 DIFF (rerun with fhashdraw=11 to name the draw)" in lines)
    check("d: '2/3 frames equal' line", "2/3 frames equal" in lines)

    # (e) empty file
    e = tmp / "empty.txt"
    e.write_text("", encoding="utf-8")
    bad, lines = bench.fhash_cmp(e, e)
    check("e: empty file gives bad 1", bad == 1)

    # (f) ingest_fhash: kind azahar_hash writes fhash.txt, another kind writes nothing
    fn = getattr(bench, "ingest_fhash", None)
    d4 = tmp / "shots4"
    r4 = fn({"kind": "azahar_hash"}, log, d4) if fn else None
    check("f: ingest_fhash azahar_hash == 3 with fhash.txt present", r4 == 3 and (d4 / "fhash.txt").is_file())
    d5 = tmp / "shots5"
    r5 = fn({"kind": "azahar_pic"}, log, d5) if fn else None
    check("f: ingest_fhash azahar_pic == 0 with no fhash.txt", r5 == 0 and not (d5 / "fhash.txt").exists())

if fails:
    print("FAILED: %d case(s)" % len(fails))
    sys.exit(1)
print("ALL PASS")
