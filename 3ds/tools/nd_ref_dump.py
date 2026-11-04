#!/usr/bin/env python3
"""nd_ref_dump.py: read an ndref.bin reference draw stream (format: 3ds/tests/ndref/FORMAT.md).

  python 3ds/tools/nd_ref_dump.py <file.bin> [...]     one line per draw and per tag
  python 3ds/tools/nd_ref_dump.py --check <file.bin> [...]   validate sizes and counts, exit 1 on any fail
"""
import struct, sys

MAGIC = 0x3152444E  # "NDR1" little endian
TEVKEY = 76          # 19 x u32: TevKey (pm_gpu.c)
NDDRAW = 84          # NdrefDraw: 21 x i32/u32 + 2 x 4 x u32 texture words (pm_gpu.c)
UNI = 56             # uvso[4], cmul[4], cadd[4], zmix[2] floats
PMFX = 304           # PmFx: 76 floats (pm_gpu.h)
PAL = 68             # u32 game matrix index + C3D_Mtx (64 bytes)
VTX_RING = 16        # ring vertex (raw path)
VTX_CPU = 40         # Vert: x y z w u v r g b a floats (CPU path)
KIND_DRAW, KIND_PROD, KIND_MDL, KIND_ST, KIND_PAL = 1, 2, 3, 4, 5
ST_WORDS = 15 + 13 + 34  # kind 4 body: 15 header words, 13 effect words, 34 texenv words (FORMAT.md)


def parse(path):
    """Return (header dict, records list). Each record is a dict. Raises ValueError with the reason."""
    data = open(path, "rb").read()
    if len(data) < 20:
        raise ValueError("file shorter than the 20-byte header")
    magic, ver, frame, ndraws, plen = struct.unpack_from("<5I", data, 0)
    if magic != MAGIC:
        raise ValueError("bad magic %08x" % magic)
    if ver != 3:
        raise ValueError("version %d, expected 3" % ver)
    if len(data) != 20 + plen:
        raise ValueError("payload %d bytes, header says %d" % (len(data) - 20, plen))
    hdr = {"frame": frame, "draws": ndraws, "bytes": plen}
    recs, p, end, draws = [], 20, 20 + plen, 0
    while p < end:
        if p + 8 > end:
            raise ValueError("record header past the end at byte %d" % p)
        kind, size = struct.unpack_from("<2I", data, p)
        body = p + 8
        if body + size > end:
            raise ValueError("record at %d: size %d runs past the end" % (p, size))
        if kind in (KIND_PROD, KIND_MDL):
            if size != 8:
                raise ValueError("tag record at %d: size %d, expected 8" % (p, size))
            tid, extra = struct.unpack_from("<2I", data, body)
            recs.append({"kind": kind, "id": tid, "extra": extra})
        elif kind == KIND_DRAW:
            recs.append(parse_draw(data, body, body + size))
            draws += 1
        elif kind == KIND_ST:
            if size != 4 * ST_WORDS:
                raise ValueError("state record at %d: size %d, expected %d" % (p, size, 4 * ST_WORDS))
            w = struct.unpack_from("<%dI" % ST_WORDS, data, body)
            recs.append({"kind": KIND_ST, "hdr": w[:15], "eff": w[15:28], "tev": w[28:]})
        elif kind == KIND_PAL:
            np_ = struct.unpack_from("<I", data, body + 64)[0]
            if size != 68 + np_ * 68:
                raise ValueError("palette record at %d: size %d, expected %d" % (p, size, 68 + np_ * 68))
            recs.append({"kind": KIND_PAL, "post": struct.unpack_from("<16f", data, body), "np": np_})
        else:
            raise ValueError("unknown record kind %d at %d" % (kind, p))
        p = body + size
    if draws != ndraws:
        raise ValueError("header says %d draws, records hold %d" % (ndraws, draws))
    return hdr, recs


def parse_draw(data, p, end):
    d = {"kind": KIND_DRAW}
    if p + TEVKEY + NDDRAW + UNI + PMFX + 4 > end:
        raise ValueError("draw record too short for the fixed part")
    d["key"] = struct.unpack_from("<19I", data, p)
    p += TEVKEY
    f = struct.unpack_from("<4i" + "i" * 7 + "ii" + "4I" + "4I", data, p)
    d.update(z=f[0], graw=f[1], prog=f[2], cull=f[3], decal=f[4], blend=f[5], atest=f[6],
             sci=f[7:11], sa=f[11], sb=f[12], tw0=f[13:17], tw1=f[17:21])
    p += NDDRAW + UNI + PMFX
    np_ = struct.unpack_from("<I", data, p)[0]
    p += 4
    if p + np_ * PAL + 4 > end:
        raise ValueError("palette of %d entries runs past the record" % np_)
    d["pal"] = [struct.unpack_from("<I", data, p + i * PAL)[0] for i in range(np_)]
    p += np_ * PAL
    nv = struct.unpack_from("<I", data, p)[0]
    p += 4
    stride = VTX_RING if d["graw"] else VTX_CPU
    if p + nv * stride != end:
        raise ValueError("vertex block: %d x %d bytes, record has %d" % (nv, stride, end - p))
    d["nv"] = nv
    d["vtx"] = data[p:end]
    if nv % 3:
        raise ValueError("%d vertices is not a whole number of triangles" % nv)
    if d["graw"] == 0 and d["pal"]:
        raise ValueError("CPU draw with a palette")
    return d


def key_hash(key):
    h = 2166136261
    for w in key:
        h = ((h ^ w) * 16777619) & 0xFFFFFFFF
    return h


def show(path, hdr, recs):
    print("%s frame %d draws %d bytes %d" % (path, hdr["frame"], hdr["draws"], hdr["bytes"]))
    n = 0
    for r in recs:
        if r["kind"] == KIND_DRAW:
            print("draw %3d key %08x z %d graw %d cull %d decal %d blend %d atest %d tex %d %d tris %d pal %s" % (
                n, key_hash(r["key"]), r["z"], r["graw"], r["cull"], r["decal"], r["blend"], r["atest"],
                r["sa"], r["sb"], r["nv"] // 3, r["pal"]))
            n += 1
        elif r["kind"] == KIND_ST:
            print("state pass %d z %d zmark %d zcmp %d zupd %d nocol %d noise %d raw %d key %08x %08x %d %d eff %s" % (
                (*r["hdr"][:12], " ".join("%08x" % x for x in r["eff"]))))
        elif r["kind"] == KIND_PROD:
            print("tag prod %d" % r["id"])
        else:
            print("tag mdl %d pan %d" % (r["id"], r["extra"]))


def main(argv):
    check = "--check" in argv
    files = [a for a in argv if a != "--check"]
    if not files:
        print(__doc__)
        return 2
    bad = 0
    for path in files:
        try:
            hdr, recs = parse(path)
        except (OSError, ValueError, struct.error) as e:
            print("FAIL %s: %s" % (path, e))
            bad += 1
            continue
        if check:
            tris = sum(r["nv"] // 3 for r in recs if r["kind"] == KIND_DRAW)
            print("OK %s frame %d draws %d tris %d bytes %d" % (path, hdr["frame"], hdr["draws"], tris, hdr["bytes"]))
        else:
            show(path, hdr, recs)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
