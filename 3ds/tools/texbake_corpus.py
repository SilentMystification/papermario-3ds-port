"""Build a request file for texbake from the repo textures plus synthetic cases.
Usage: python3 texbake_corpus.py <repo_root> <out_req.bin>
Real source: every ver/us/build/**/X.png.bin with the size of assets/us/**/X.png. The format is unknown to this script (the build rule has it),
so each file is used with every N64 format that has its bytes per texel (the decode rules are the test, not the meaning).
Palette: X.pal.bin when it exists, else random. Extra cases: tile masks, mirror, stride larger than the width, odd widths."""
import os, random, struct, sys
from PIL import Image
root, out = sys.argv[1], sys.argv[2]
rnd = random.Random(7)
FM = {4: [(2, 0), (3, 0), (4, 0)], 8: [(2, 1), (3, 1), (4, 1)], 16: [(0, 2), (3, 2)], 32: [(0, 3)]}  # fmt, siz by bits per texel
reqs = []
def add(fmt, siz, w, h, stride, mw, mh, ms, mt, tn, tl, src):
    reqs.append(struct.pack("<BBBBHHHHHHHHI", fmt, siz, ms, mt, w, h, stride, mw, mh, tn, 0, 0, len(src)) + tl + src)
bd = os.path.join(root, "ver/us/build")
for dp, _, fs in os.walk(bd):
    for f in sorted(fs):
        if not f.endswith(".png.bin"): continue
        pb = os.path.join(dp, f); png = os.path.join(root, "assets/us", os.path.relpath(pb, bd))[:-4]
        if not os.path.exists(png): continue
        try: w, h = Image.open(png).size
        except Exception: continue
        data = open(pb, "rb").read()
        if w * h == 0 or w > 512 or h > 512: continue
        bits = len(data) * 8 // (w * h)
        if bits not in FM or len(data) * 8 != w * h * bits: continue
        palp = pb[:-8] + ".pal.bin"
        for fmt, siz in FM[bits]:
            if fmt == 2:
                n = 16 if siz == 0 else 256
                tl = open(palp, "rb").read()[:n * 2] if os.path.exists(palp) else bytes(rnd.randrange(256) for _ in range(n * 2))
                tl = tl.ljust(n * 2, b"\0"); tn = len(tl) // 2
            else: tl, tn = b"", 0
            add(fmt, siz, w, h, w, 0, 0, 0, 0, tn, tl, data)
# synthetic: random bytes, masks, mirror, stride, odd sizes
for _ in range(400):
    fmt, siz = rnd.choice([(0, 2), (0, 3), (2, 0), (2, 1), (3, 0), (3, 1), (3, 2), (4, 0), (4, 1), (1, 2), (0, 1)])
    w = rnd.choice([1, 3, 8, 13, 16, 31, 32, 48, 64, 100]); h = rnd.choice([1, 5, 8, 16, 24, 32, 63, 64])
    stride = w + rnd.choice([0, 0, 2, 8, 16])
    mw = rnd.choice([0, 0, 0, 4, 8, 16]); mh = rnd.choice([0, 0, 0, 4, 8, 16])
    mw = mw if mw < w else 0; mh = mh if mh < h else 0
    bits = {0: 4, 1: 8, 2: 16, 3: 32}[siz]
    rows = mh or h; ww = mw or w
    ln = (((rows - 1) * stride + ww) * bits + 7) // 8 + 4
    src = bytes(rnd.randrange(256) for _ in range(ln))
    tn = 0; tl = b""
    if fmt == 2:
        tn = rnd.choice([16, 256, 5]) if siz else rnd.choice([16, 7]); tl = bytes(rnd.randrange(256) for _ in range(tn * 2))
    add(fmt, siz, w, h, stride, mw, mh, rnd.randrange(2), rnd.randrange(2), tn, tl, src)
open(out, "wb").write(struct.pack("<II", 0x51544d50, len(reqs)) + b"".join(reqs))
print("requests", len(reqs))
