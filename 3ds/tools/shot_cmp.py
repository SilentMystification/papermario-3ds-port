#!/usr/bin/env python3
"""shot_cmp.py DIR_A DIR_B: compare the top-screen shots of two azt runs.

Rule: a top shot (a .ppm whose name does not start with bot_) must be byte-equal.
Bottom shots (bot_*.ppm) are not compared: HUD numbers and the refresh interval differ
between runs of the same binary. Exit 0 when every top shot pairs and is byte-equal, 1 otherwise.
"""
import filecmp
import os
import sys


def tops(d):
    return sorted(f for f in os.listdir(d) if f.endswith(".ppm") and not f.startswith("bot_"))


def main():
    a, b = sys.argv[1], sys.argv[2]
    ta, tb = tops(a), tops(b)
    if ta != tb:
        print(f"FAIL top shot names differ: {ta} vs {tb}")
        return 1
    if not ta:
        print("FAIL no top shots")
        return 1
    eq = 0
    for f in ta:
        same = filecmp.cmp(os.path.join(a, f), os.path.join(b, f), shallow=False)
        eq += same
        print(f"{'EQUAL' if same else 'DIFF '}  {f}")
    bots = sum(1 for f in os.listdir(a) if f.startswith("bot_"))
    print(f"TOP {eq} of {len(ta)} byte-equal; bottom skipped: {bots} files")
    return 0 if eq == len(ta) else 1


if __name__ == "__main__":
    sys.exit(main())
