"""TEXSELF2: independent check of the 3DS video memory image (TX1 path, pm_tex_tx1.h) against the N64 rules.
Usage: python 3ds/tools/tex_selftest_mem.py build3ds/texbake/corpus.req build3ds/texbake/mem.bin
Memory rule: element (X, Y) of a pw x ph image sits in 8x8 tile (Y>>3)*(pw>>3)+(X>>3), position morton(X&7, Y&7) (x bits at even
positions, y bits at odd positions). Its content is the source texel (axis(X), axis(ph-1-Y)): the image is stored flipped in Y.
axis() is the tile mask (wrap or mirror, mask < size) or the edge clamp. Element size: RGBA8 4 bytes (A,B,G,R order in memory),
LA4 and L8 1 byte, RGBA5551 and LA8 2 bytes (little endian of the 16-bit value).
Known and excluded: nibble parity (CI4, I4, IA4) is taken from x, not from the texel index (pm_tex_tx1.h tx1_n4); reported apart."""
import struct, sys, collections
req_path, mem_path = sys.argv[1], sys.argv[2]
names = open(req_path + ".names", encoding="utf-8").read().split("\n")
b = open(req_path, "rb").read()
magic, n = struct.unpack_from("<II", b, 0); off = 8
reqs = []
for _ in range(n):
    fmt, siz, ms, mt, w, h, stride, mw, mh, tn, _a, _b, ln = struct.unpack_from("<BBBBHHHHHHHHI", b, off); off += 24
    tl = b[off:off + tn * 2]; off += tn * 2
    src = b[off:off + ln]; off += ln
    reqs.append(dict(fmt=fmt, siz=siz, ms=ms, mt=mt, w=w, h=h, stride=stride, mw=mw, mh=mh, tn=tn, tl=tl, src=src))
d = open(mem_path, "rb").read(); p = 0
def pot8(v):
    q = 8
    while q < v: q *= 2
    return q
def axis(c, size, mask, mirror):
    if mask and mask < size:
        m = mask
        r = c % (2 * m) if mirror else c % m
        return 2 * m - 1 - r if (mirror and r >= m) else r
    return min(c, size - 1)
def morton(x, y):
    v = 0
    for k in range(3):
        v |= ((x >> k) & 1) << (2 * k)
        v |= ((y >> k) & 1) << (2 * k + 1)
    return v
def be16(buf, k): return (buf[k] << 8) | buf[k + 1]
stat = collections.Counter(); rows = []
for i, q in enumerate(reqs):
    ri, pw, ph, bpp = struct.unpack_from("<iiii", d, p); p += 16
    assert ri == i
    mem = d[p:p + pw * ph * bpp] if bpp else b""; p += pw * ph * bpp
    fmt, siz = q["fmt"], q["siz"]
    if fmt not in (0, 2, 3, 4) or (fmt == 4 and siz == 2) or (fmt == 3 and siz in (0, 1) and False):
        stat["skipped (not an N64 format in the list)"] += 1; continue
    if bpp == 0:
        stat["port built nothing"] += 1; rows.append((i, names[i], q, "port built nothing")); continue
    if (pw, ph) != (pot8(q["w"]), pot8(q["h"])):
        stat["size differs"] += 1; rows.append((i, names[i], q, "size")); continue
    if fmt == 0 and siz == 3: gbpp = 4
    elif (fmt == 4 and siz in (0, 1)) or (fmt == 3 and siz in (0, 1)): gbpp = 1
    else: gbpp = 2
    if gbpp != bpp:
        stat["element size differs"] += 1; rows.append((i, names[i], q, "element size %d vs %d" % (bpp, gbpp))); continue
    bad = 0; first = None; rowbad = set()
    for Y in range(ph):
        Ys = axis(ph - 1 - Y, q["h"], q["mh"], q["mt"])
        for X in range(pw):
            Xs = axis(X, q["w"], q["mw"], q["ms"])
            t = Ys * q["stride"] + Xs
            tile = (Y >> 3) * (pw >> 3) + (X >> 3)
            e = tile * 64 + morton(X & 7, Y & 7)
            k = e * bpp
            if fmt == 2 or (fmt == 4 and siz == 0) or (fmt == 3 and siz == 0) or (fmt == 4 and siz == 1 and False):
                pass
            if fmt == 2 and siz == 0:
                byte = q["src"][t >> 1]; idx = (byte & 15) if (t & 1) else (byte >> 4)  
                want = (be16(q["tl"], idx * 2) if idx < q["tn"] else 0xFFFF).to_bytes(2, "little")
            elif fmt == 2 and siz == 1:
                idx = q["src"][t]
                want = (be16(q["tl"], idx * 2) if idx < q["tn"] else 0xFFFF).to_bytes(2, "little")
            elif fmt == 0 and siz == 2:
                want = be16(q["src"], t * 2).to_bytes(2, "little")
            elif fmt == 0 and siz == 3:
                s = q["src"][t * 4:t * 4 + 4]; want = bytes([s[3], s[2], s[1], s[0]])
            elif fmt == 3 and siz == 2:
                want = be16(q["src"], t * 2).to_bytes(2, "little")
            elif fmt == 3 and siz == 1:
                want = bytes([q["src"][t]])
            elif fmt == 3 and siz == 0:
                nib = (q["src"][t >> 1] & 15) if (t & 1) else (q["src"][t >> 1] >> 4)
                I3 = nib >> 1; L4 = (I3 << 1) | (I3 >> 2); A4 = 15 if (nib & 1) else 0
                want = bytes([(L4 << 4) | A4])
            elif fmt == 4 and siz == 1:
                want = bytes([q["src"][t]])
            elif fmt == 4 and siz == 0:
                nib = (q["src"][t >> 1] & 15) if (t & 1) else (q["src"][t >> 1] >> 4)
                want = bytes([(nib << 4) | nib])
            else:
                continue
            have = mem[k:k + bpp]
            if have != want:
                bad += 1; rowbad.add(Y)
                if first is None: first = (X, Y, have.hex(), want.hex(), Xs, Ys)
    if bad:
        cls = "MISMATCH"
        if fmt in (2, 4, 3) and siz == 0 and q["w"] % 2 == 1: cls = "known CI4/I4/IA4 odd-width nibble parity"
        stat[cls] += 1
        rows.append((i, names[i], q, "%s: texels %d, mem rows %d..%d, first X,Y %d,%d port %s want %s (src X %d Y %d)" % ((cls, bad, min(rowbad), max(rowbad)) + first)))
    else:
        stat["match"] += 1
print("requests", len(reqs), dict(stat))
for i, nm, q, msg in rows[:300]:
    print("req %d %s fmt %d siz %d %dx%d stride %d mask %d,%d mirror %d,%d: %s" % (i, nm, q["fmt"], q["siz"], q["w"], q["h"], q["stride"], q["mw"], q["mh"], q["ms"], q["mt"], msg))
