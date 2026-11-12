# BASEKEY host test: the base store key (bench.scen_hash, bench.stored). Run from the repo root (run.sh does that).
import json, os, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent.parent
sys.path.insert(0, str(ROOT / "3ds" / "tools"))
import bench  # bench.py guards main, so the import runs nothing

fails = []


def check(name, ok):
    print("%s: %s" % ("PASS" if ok else "FAIL", name))
    if not ok:
        fails.append(name)


base = bench.scenario("spot_mac01_det")
h_now = bench.scen_hash(base)

# (a) status and reason are not part of the key
a = dict(base, status="unverified", reason="x")
b = dict(base, status="verified", reason="y")
check("a: scen_hash equal when only status and reason differ", bench.scen_hash(a) == bench.scen_hash(b))

# (b) debug is part of the key
c = dict(base, debug=str(base.get("debug")) + " loglevel=info")
check("b: scen_hash differs when debug differs", bench.scen_hash(c) != h_now)

with tempfile.TemporaryDirectory() as tmp:
    bench.store_root = lambda: Path(tmp)
    commit = "d048e654d0000000000000000000000000000000"
    d = Path(tmp) / commit / "spot_mac01_det"

    def write_store(meta):
        d.mkdir(parents=True, exist_ok=True)
        (d / "shot_000200.ppm").write_bytes(b"P6\n1 1\n255\n\x00\x00\x00")
        if meta is not None:
            (d / "meta.json").write_text(json.dumps(meta), encoding="utf-8")

    # (c) a store with no scen_hash is stale
    write_store({"bench": "spot_mac01_det", "commit": commit})
    check("c: store without scen_hash -> stored() False", bench.stored(commit, "spot_mac01_det") is False)

    # (d) a store with the current hash is stored
    (d / "meta.json").write_text(json.dumps({"scen_hash": h_now}), encoding="utf-8")
    check("d: store with the current scen_hash -> stored() True", bench.stored(commit, "spot_mac01_det") is True)

    # (e) a store with another hash is stale
    (d / "meta.json").write_text(json.dumps({"scen_hash": "000000000000"}), encoding="utf-8")
    check("e: store with hash 000000000000 -> stored() False", bench.stored(commit, "spot_mac01_det") is False)

n = 5
print("basekey: %d/%d PASS" % (n - len(fails), n) if not fails else "basekey: %d/%d PASS (FAIL: %s)" % (n - len(fails), n, ", ".join(fails)))
sys.exit(1 if fails else 0)
