"""TEXSELF: independent texture decode check. Reads the corpus (texbake_corpus.py) and the port's linear decode
(build3ds/texbake/decode.bin, from 3ds/tools/tex_selftest.c = pm_tex.c expand). Decodes each request again here from the
N64 rules and compares texel by texel. Usage: python 3ds/tools/tex_selftest.py build3ds/texbake/corpus.req build3ds/texbake/decode.bin
Coordinates: X(x) = mask map of x when a mask is set (mask < width), else min(x, w-1). Same for Y with the height.
Mask map: r = x mod (2m or m); mirror: r >= m -> 2m-1-r. Formats: CI4/CI8 (index, palette be16, idx >= n -> 0xffff),
RGBA16 (be16), RGBA32 (4 bytes), IA16 (be16 as LE bytes). I4/I8/IA4/IA8: the port refuses (expand returns 0), refusal expected."""
import struct, sys, collections
req_path, dec_path = sys.argv[1], sys.argv[2]
names = open(req_path + ".names", encoding="utf-8").read().split("\n")
b = open(req_path, "rb").read()
magic, n = struct.unpack_from("<II", b, 0)
assert magic == 0x51544D50, "bad corpus magic"
reqs = []; off = 8
for _ in range(n):
    fmt, siz, ms, mt, w, h, stride, mw, mh, tn, _a, _b, ln = struct.unpack_from("<BBBBHHHHHHHHI", b, off); off += 24
    tl = b[off:off + tn * 2]; off += tn * 2
    src = b[off:off + ln]; off += ln
    reqs.append(dict(fmt=fmt, siz=siz, ms=ms, mt=mt, w=w, h=h, stride=stride, mw=mw, mh=mh, tn=tn, tl=tl, src=src))
d = open(dec_path, "rb").read(); p = 0
def be16(buf, k): return (buf[k] << 8) | buf[k + 1]
def pow2_8(v):
    p2 = 8
    while p2 < v: p2 *= 2
    return p2
def mmap(c, m, mirror, size):
    if mirror:
        r = c % (2 * m)
        return 2 * m - 1 - r if r >= m else r
    return c % m
def axis(c, size, mask, mirror):
    if mask and mask < size: return mmap(c, mask, mirror, size)
    return min(c, size - 1)
stat = collections.Counter(); rows = []
for i, q in enumerate(reqs):
    ri, pw, ph, bpp = struct.unpack_from("<iiii", d, p); p += 16
    assert ri == i, "index drift %d vs %d" % (ri, i)
    got = d[p:p + pw * ph * bpp] if bpp else b""; p += pw * ph * bpp
    key = (q["fmt"], q["siz"])
    supported = not ((q["fmt"] == 4 or q["fmt"] == 3) and q["siz"] in (0, 1)) and q["fmt"] in (0, 2, 3)
    if not supported:
        ok_port = bpp == 0
        stat["refused-ok" if ok_port else "UNEXPECTED decode of unsupported"] += 1
        if not ok_port: rows.append((i, names[i], q, "unsupported format decoded", None))
        continue
    if bpp == 0:
        stat["port refused supported"] += 1; rows.append((i, names[i], q, "port refused a supported format", None)); continue
    pw_e, ph_e = pow2_8(q["w"]), pow2_8(q["h"])
    if (pw_e, ph_e) != (pw, ph):
        stat["size differs"] += 1; rows.append((i, names[i], q, "size %dx%d vs %dx%d" % (pw, ph, pw_e, ph_e), None)); continue
    first = None; bad = 0
    for y in range(ph):
        Y = axis(y, q["h"], q["mh"], q["mt"])
        for x in range(pw):
            X = axis(x, q["w"], q["mw"], q["ms"])
            t = Y * q["stride"] + X
            if q["fmt"] == 2:
                if q["siz"] == 0:
                    byte = q["src"][t >> 1]; idx = (byte & 15) if (t & 1) else (byte >> 4)
                else:
                    idx = q["src"][t]
                exp = be16(q["tl"], idx * 2) if idx < q["tn"] else 0xFFFF
                exp = exp.to_bytes(2, "little")
            elif q["fmt"] == 0 and q["siz"] == 2:
                exp = be16(q["src"], t * 2).to_bytes(2, "little")
            elif q["fmt"] == 0 and q["siz"] == 3:
                exp = q["src"][t * 4:t * 4 + 4]
            elif q["fmt"] == 3 and q["siz"] == 2:
                exp = be16(q["src"], t * 2).to_bytes(2, "little")
            else:
                exp = None
            if exp is None: continue
            k = (y * pw + x) * bpp
            have = got[k:k + bpp]
            if have != exp[:bpp]:
                bad += 1
                if first is None: first = (x, y, have.hex(), exp[:bpp].hex(), X, Y)
    if bad: stat["MISMATCH"] += 1; rows.append((i, names[i], q, "texels %d, first x,y %d,%d port %s want %s (src x %d y %d)" % ((bad,) + first), first))
    else: stat["match"] += 1
print("requests", len(reqs), dict(stat))
for i, nm, q, msg, _ in rows[:400]:
    print("req %d %s fmt %d siz %d %dx%d stride %d mask %d,%d mirror %d,%d: %s" % (i, nm, q["fmt"], q["siz"], q["w"], q["h"], q["stride"], q["mw"], q["mh"], q["ms"], q["mt"], msg))
