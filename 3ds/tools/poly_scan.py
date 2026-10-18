# S19c scan: flags shots with more olive pixels or solid black 8x8 blocks than the idle battle golden.
# Usage: python 3ds/tools/poly_scan.py [shots_dir]
import glob, os, sys
import numpy as np
from PIL import Image

def olive(a):  # approximate olive, exact RGB still to confirm on one by-eye shot
    r, g, b = a[..., 0].astype(int), a[..., 1].astype(int), a[..., 2].astype(int)
    return (r >= 100) & (r <= 160) & (g >= 100) & (g <= 160) & (b <= 60)

def black8(a):  # count 8x8 solid black blocks
    m = (a.max(axis=2) == 0).astype(np.int32)
    c = np.pad(m.cumsum(0).cumsum(1), ((1, 0), (1, 0)))
    s = c[8:, 8:] - c[:-8, 8:] - c[8:, :-8] + c[:-8, :-8]
    return int((s == 64).sum())

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SHOTS = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build3ds", "shots")  # arg: other shots dir (lane)
gold = np.asarray(Image.open(os.path.join(ROOT, "3ds", "golden", "battle.png")).convert("RGB"))
base_b, base_o = black8(gold), int(olive(gold).sum())
bad = 0
for f in sorted(glob.glob(os.path.join(SHOTS, "*.ppm"))):
    a = np.asarray(Image.open(f).convert("RGB"))
    o, b = int(olive(a).sum()), black8(a)
    flag = o > base_o or b > base_b
    bad += flag
    if flag: print(f, "olive", o, "black8", b)
print("frames", len(glob.glob(os.path.join(SHOTS, "*.ppm"))), "flagged", bad, "base olive", base_o, "base black8", base_b)
sys.exit(1 if bad else 0)
