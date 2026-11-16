#!/usr/bin/env python3
"""BASEBIN self-test of 3ds/tools/basebin.py (cache safety rule 4). Seeded faults: one changed input byte (new key), one
changed stored byte, a key.txt of another key, a missing file, PM_CACHE=off. Each must give a miss. Prints "basebin ok"."""
import os
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
import basebin as bb  # noqa: E402


def fail(msg):
    print("basebin FAIL " + msg)
    sys.exit(1)


def main():
    os.environ["PM_CACHE"] = "on"
    t = Path(tempfile.mkdtemp())
    root = t / "store"
    src = t / "a.3dsx"
    src.write_bytes(b"x1")
    inp = bytearray(b"input")
    k1 = "basebin v1\nin %s" % bb.sha1_bytes(bytes(inp))
    if bb.get(root, k1):
        fail("hit on an empty store")
    if not bb.put(root, k1, {"pm_3ds.3dsx": src}, 6):
        fail("put stored nothing")
    d = bb.get(root, k1)
    if not d or (d / "pm_3ds.3dsx").read_bytes() != b"x1":
        fail("miss after put, or other bytes")
    inp[0] ^= 1
    k2 = "basebin v1\nin %s" % bb.sha1_bytes(bytes(inp))
    if bb.get(root, k2):
        fail("one changed input byte gave a hit")
    (d / "pm_3ds.3dsx").write_bytes(b"x2")
    if bb.get(root, k1) or d.exists():
        fail("a changed stored byte gave a hit, or the entry stayed")
    d = bb.put(root, k1, {"pm_3ds.3dsx": src}, 6)
    (d / "key.txt").write_bytes(k2.encode("utf-8"))
    if bb.get(root, k1):
        fail("a key.txt of another key gave a hit")
    if bb.put(root, k1, {"pm_3ds.3dsx": t / "missing"}, 6):
        fail("a missing file was stored")
    os.environ["PM_CACHE"] = "off"
    if bb.put(root, k2, {"pm_3ds.3dsx": src}, 6):
        fail("PM_CACHE=off stored an entry")
    os.environ["PM_CACHE"] = "on"
    if bb.get(root, k2):
        fail("PM_CACHE=off left an entry")
    for i in range(8):
        bb.put(root, "k%d" % i, {"f": src}, 6)
    if len([p for p in root.iterdir() if p.is_dir()]) > 6:
        fail("trim kept more than 6 entries")
    if bb.image_id("pm3ds-build:1") != bb.image_id("pm3ds-build:1"):
        fail("image_id gave two values for one image")
    if bb.image_id("pm3ds-no-such-image:0") != "none":
        fail("image_id of a missing image is not none")
    print("basebin ok")


if __name__ == "__main__":
    main()
