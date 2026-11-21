#!/usr/bin/env python3
"""tex_fringe: count the alpha-0 texels that show a key colour under bilinear (plan_decal.md sections 3 and 6, row DEC0).

  python 3ds/tools/tex_fringe.py <asset_base> [--fill] [--list N]
  python 3ds/tools/tex_fringe.py --selftest

Reads ver/us/baserom.z64 and assets/us/mapfs/tex/*_tex.json of the asset base (read only). Applies the TEXKEY2 patches of
texkey_bake.bake() to a copy of the ROM, as the game does (pm_os.c:506). Each ROM record address is taken once.

Formats decoded (header fm low nibble, dp low nibble, et 0 or 1, aw 0): CI4, CI8, RGBA16, RGBA32, IA4, IA8, IA16.
I4 and I8 are not in the plan scope; they are counted as skipped. CI and RGBA16 keep 5-bit channels (tolerance 1),
the other formats use 8-bit channels (IA4 intensity expanded from 3 bits by bit replication; tolerance 8).

Bad texel: alpha 0, at least one opaque 8-neighbour in the image, and a channel more than the tolerance off floor(sum / c)
of those opaque neighbours (c = their count).
Seam texel: alpha 0 on an image border, the json wrap of that axis is "Repeat", and the texel on the opposite border is opaque.
--fill (simulates DEC1 on the decoded image): each alpha-0 texel of CI4, CI8, RGBA16 with an opaque 8-neighbour takes
floor(sum / c). Opaque texels never change. The counts after the fill are printed.
--list N: the N textures with the most bad texels after the run, sorted by bad (desc), archive, name.
--selftest: a 3x3 image; expect bad 8, 0 after the 8-neighbour fill, 4 after the seeded 4-neighbour fill.
"""
import argparse
import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import texkey_bake  # noqa: E402

HDR = 48
FORMATS = {(2, 0): "CI4", (2, 1): "CI8", (0, 2): "RGBA16", (0, 3): "RGBA32", (3, 0): "IA4", (3, 1): "IA8", (3, 2): "IA16"}
ORDER = ["CI4", "CI8", "RGBA16", "RGBA32", "IA4", "IA8", "IA16"]
FILL = ("CI4", "CI8", "RGBA16")
FIVE = ("CI4", "CI8", "RGBA16")
NB8 = [(-1, -1), (0, -1), (1, -1), (-1, 0), (1, 0), (-1, 1), (0, 1), (1, 1)]
NB4 = [(0, -1), (-1, 0), (1, 0), (0, 1)]


class Img:
    """w, h, px: list of (r, g, b) per texel, op: list of bool (opaque), tol: channel tolerance."""
    def __init__(self, w, h, px, op, tol):
        self.w, self.h, self.px, self.op, self.tol = w, h, px, op, tol


def opaque_nb(img, x, y, nbs):
    """The opaque neighbours of (x, y) in nbs, inside the image: list of indices."""
    out = []
    for dx, dy in nbs:
        nx, ny = x + dx, y + dy
        if 0 <= nx < img.w and 0 <= ny < img.h and img.op[ny * img.w + nx]:
            out.append(ny * img.w + nx)
    return out


def mean_of(img, idxs):
    c = len(idxs)
    return tuple(sum(img.px[i][k] for i in idxs) // c for k in range(3))


def bad_flags(img):
    """One bool per texel: alpha 0, opaque 8-neighbour, a channel more than img.tol off the neighbour mean."""
    flags = [False] * (img.w * img.h)
    for y in range(img.h):
        for x in range(img.w):
            i = y * img.w + x
            if img.op[i]:
                continue
            nb = opaque_nb(img, x, y, NB8)
            if not nb:
                continue
            m = mean_of(img, nb)
            flags[i] = any(abs(img.px[i][k] - m[k]) > img.tol for k in range(3))
    return flags


def fill(img, nbs):
    """A copy of img: each alpha-0 texel with an opaque neighbour in nbs takes the floor mean of those neighbours."""
    px = list(img.px)
    for y in range(img.h):
        for x in range(img.w):
            i = y * img.w + x
            if img.op[i]:
                continue
            nb = opaque_nb(img, x, y, nbs)
            if nb:
                px[i] = mean_of(img, nb)
    return Img(img.w, img.h, px, img.op, img.tol)


def seam_count(img, wrap_x, wrap_y):
    """Alpha-0 border texels whose opposite border texel (same row or column, wrap axis Repeat) is opaque."""
    n = 0
    for y in range(img.h):
        for x in range(img.w):
            i = y * img.w + x
            if img.op[i]:
                continue
            hit = False
            if wrap_x and x == 0 and img.op[y * img.w + img.w - 1]:
                hit = True
            if wrap_x and x == img.w - 1 and img.op[y * img.w]:
                hit = True
            if wrap_y and y == 0 and img.op[(img.h - 1) * img.w + x]:
                hit = True
            if wrap_y and y == img.h - 1 and img.op[x]:
                hit = True
            if hit:
                n += 1
    return n


def decode(rom, p, key, mw, mh, et):
    """The main image of the record at p as an Img, or None when the ROM is too short."""
    n = mw * mh
    if key[0] == 2:  # CI: palette after the raster (mip texels count in the raster, as texkey_bake)
        texels_all = n
        if et == 1:
            texels_all += texkey_bake.mip_texels(mw, mh, key[1])
        rast = texels_all // 2 if key[1] == 0 else texels_all
        np_ = 16 if key[1] == 0 else 256
        pal_off = p + HDR + rast
        if pal_off + np_ * 2 > len(rom):
            return None
        pal = [struct.unpack(">H", rom[pal_off + 2 * i:pal_off + 2 * i + 2])[0] for i in range(np_)]
        raw = rom[p + HDR:p + HDR + rast]
        if key[1] == 0:
            idx = [((raw[(y * mw + x) >> 1] >> (4 if ((y * mw + x) & 1) == 0 else 0)) & 15) for y in range(mh) for x in range(mw)]
        else:
            idx = list(raw[:n])
        px = [((pal[i] >> 11) & 31, (pal[i] >> 6) & 31, (pal[i] >> 1) & 31) for i in idx]
        op = [bool(pal[i] & 1) for i in idx]
        return Img(mw, mh, px, op, 1)
    fmt = FORMATS[key]
    bits = {"RGBA16": 16, "RGBA32": 32, "IA4": 4, "IA8": 8, "IA16": 16}[fmt]
    need = (n * bits + 7) // 8
    if p + HDR + need > len(rom):
        return None
    raw = rom[p + HDR:p + HDR + need]
    px, op = [], []
    if fmt == "RGBA16":
        for j in range(n):
            v = (raw[2 * j] << 8) | raw[2 * j + 1]
            px.append(((v >> 11) & 31, (v >> 6) & 31, (v >> 1) & 31)); op.append(bool(v & 1))
        return Img(mw, mh, px, op, 1)
    if fmt == "RGBA32":
        for j in range(n):
            r, g, b, a = raw[4 * j:4 * j + 4]
            px.append((r, g, b)); op.append(a > 0)
        return Img(mw, mh, px, op, 8)
    if fmt == "IA4":
        for j in range(n):
            nib = (raw[j >> 1] >> (4 if (j & 1) == 0 else 0)) & 15
            i3 = nib >> 1
            v8 = (i3 << 5) | (i3 << 2) | (i3 >> 1)
            px.append((v8, v8, v8)); op.append((nib & 1) == 1)
        return Img(mw, mh, px, op, 8)
    if fmt == "IA8":
        for j in range(n):
            b = raw[j]
            v8 = (b >> 4) * 17
            px.append((v8, v8, v8)); op.append((b & 15) > 0)
        return Img(mw, mh, px, op, 8)
    for j in range(n):  # IA16: intensity byte, alpha byte
        v8 = raw[2 * j]
        px.append((v8, v8, v8)); op.append(raw[2 * j + 1] > 0)
    return Img(mw, mh, px, op, 8)


def find_records(rom, name):
    """The candidate record addresses of a texture name (texkey_bake.bake: name, zero padding to 32 bytes)."""
    out, start = [], 0
    while True:
        p = rom.find(name, start)
        if p < 0:
            break
        start = p + 1
        if any(rom[p + len(name):p + 32]) or p + HDR > len(rom):
            continue
        out.append(p)
    return out


def walk(base):
    """The decoded records: list of dicts (arch, name, fmt, w, h, img, wrap_x, wrap_y), and the skipped counts."""
    rom_path = os.path.join(base, "ver", "us", "baserom.z64")
    rom = bytearray(open(rom_path, "rb").read())
    patches, _ = texkey_bake.bake(base, rom_path)
    for a, v in patches.items():
        rom[a] = (v >> 8) & 255
        rom[a + 1] = v & 255
    tex_dir = os.path.join(base, "assets", "us", "mapfs", "tex")
    seen, recs = set(), []
    skipped = {"I4": 0, "I8": 0, "other": 0}
    for jf in sorted(os.listdir(tex_dir)):
        if not jf.endswith("_tex.json"):
            continue
        arch = jf[:-5]
        for t in json.load(open(os.path.join(tex_dir, jf), encoding="utf-8")):
            m = t.get("main", {})
            name = t["name"].encode("ascii")
            for p in find_records(rom, name):
                aw, mw, ah, mh, var, et, comb, fm, dp, hw, vw, flt = struct.unpack(">HHHHBBBBBBBB", rom[p + 32:p + 48])
                key = (fm & 15, dp & 15)
                if et not in (0, 1) or aw != 0 or mw == 0 or mh == 0:
                    continue
                if key not in FORMATS:
                    if key == (4, 0):
                        skipped["I4"] += 1
                    elif key == (4, 1):
                        skipped["I8"] += 1
                    else:
                        skipped["other"] += 1
                    continue
                if p in seen:
                    continue
                img = decode(rom, p, key, mw, mh, et)
                if img is None:
                    skipped["other"] += 1
                    continue
                seen.add(p)
                recs.append({"arch": arch, "name": t["name"], "fmt": FORMATS[key], "w": mw, "h": mh, "img": img,
                             "wrap_x": m.get("hwrap") == "Repeat", "wrap_y": m.get("vwrap") == "Repeat"})
    return recs, skipped


def scan(recs, do_fill):
    """Per texture bad counts (after the fill when do_fill), totals per format, seam total and flagged textures."""
    per = {f: 0 for f in ORDER}
    bad_tex, seam_total, flagged = [], 0, 0
    for r in recs:
        img = r["img"]
        if do_fill and r["fmt"] in FILL:
            img = fill(img, NB8)
        bad = sum(1 for b in bad_flags(img) if b)
        seam = seam_count(r["img"], r["wrap_x"], r["wrap_y"])
        per[r["fmt"]] += bad
        seam_total += seam
        if bad or seam:
            flagged += 1
        bad_tex.append((bad, r["arch"], r["name"], r["fmt"], r["w"], r["h"]))
    return per, seam_total, flagged, bad_tex


def selftest():
    px = [(0, 0, 0)] * 9
    op = [False] * 9
    px[4] = (31, 0, 0)
    op[4] = True
    img = Img(3, 3, px, op, 1)
    n0 = sum(bad_flags(img))
    n8 = sum(bad_flags(fill(img, NB8)))
    n4 = sum(bad_flags(fill(img, NB4)))
    ok = n0 == 8 and n8 == 0 and n4 == 4
    print("tex_fringe selftest bad %d after 8-nb fill %d after seeded 4-nb fill %d" % (n0, n8, n4))
    print("tex_fringe selftest PASS" if ok else "tex_fringe selftest FAIL")
    return 0 if ok else 1


def main(argv):
    ap = argparse.ArgumentParser(description="alpha-0 key colour texels of the map textures (DEC0)")
    ap.add_argument("base", nargs="?", help="asset base: holds ver/us/baserom.z64 and assets/us/mapfs/tex")
    ap.add_argument("--fill", action="store_true", help="simulate the DEC1 fill of CI4, CI8, RGBA16 before the count")
    ap.add_argument("--list", type=int, default=0, metavar="N", help="list the N textures with the most bad texels")
    ap.add_argument("--selftest", action="store_true")
    a = ap.parse_args(argv)
    if a.selftest:
        return selftest()
    if not a.base:
        ap.error("asset base needed (or --selftest)")
    recs, skipped = walk(a.base)
    per, seam, flagged, bad_tex = scan(recs, a.fill)
    print("fringe fill %s skipped I4 %d I8 %d other %d" % ("on" if a.fill else "off", skipped["I4"], skipped["I8"], skipped["other"]))
    print("fringe: textures %d flagged %d bad %d seam %d " % (len(recs), flagged, sum(per.values()), seam)
          + " ".join("%s %d" % (f, per[f]) for f in ORDER))
    if a.list > 0:
        for bad, arch, name, fmt, w, h in sorted(bad_tex, key=lambda x: (-x[0], x[1], x[2]))[:a.list]:
            if bad > 0:
                print("ft %s %s %s %dx%d bad %d" % (arch, name, fmt, w, h, bad))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
