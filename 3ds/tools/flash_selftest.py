#!/usr/bin/env python3
"""Check save.flash after a run with the switches `freshsave flashselftest`. Usage: flash_selftest.py save.flash
Builds the bytes the OLD write path gives (every erase and page write goes to the file in call order, so the file equals the
RAM chip) and compares byte for byte. Also prints the ram hash the game logs (hash = h*31 + byte over 128 KB, u32)."""
import sys
m = bytearray(b"\xff" * 131072)
m[3 * 16384:4 * 16384] = b"\xff" * 16384
for p in range(39):
    o = 3 * 16384 + p * 128
    m[o:o + 128] = bytes((p * 7 + i) & 255 for i in range(128))
m[0:16384] = b"\xff" * 16384
m[0:128] = bytes((0xA5 + i) & 255 for i in range(128))
h = 0
for b in m:
    h = (h * 31 + b) & 0xFFFFFFFF
got = open(sys.argv[1], "rb").read()
print("expected hash %08x" % h)
print("file", "EQUAL byte for byte" if got == bytes(m) else "DIFFERENT (size %d)" % len(got))
sys.exit(0 if got == bytes(m) else 1)
