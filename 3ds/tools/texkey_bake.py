#!/usr/bin/env python3
"""TEXKEY2: fix the alpha-0 (cutout key) palette entries of the map texture archives at build time.

Reads the ROM (ver/us/baserom.z64 of the asset base, read only) and the texture lists (mapfs/tex/<map>_tex.json of the asset
base, read only). Finds each CI4/CI8 texture record in the ROM by its 32 byte name, checks the header against the JSON, and
applies the palette rule: each alpha-0 palette index takes the average RGB of the opaque texels that touch texels of that
index (alpha stays 0, an index with no opaque neighbour keeps its RGB). Bilinear at a cutout edge then mixes the rock colour,
not the key colour. The result is a patch list (ROM address, big endian value) that pi_read_raw callers apply in nuPiReadRom
(pm_os.c). Nothing is written to the assets or the ROM.

Usage: python3 3ds/tools/texkey_bake.py <asset_base> <out.h>
Writes the header (generated, git-ignored) and prints one summary line.
"""
import json
import os
import struct
import sys
import zlib

FMT_CI = 2
TILES_BASIC, TILES_MIPMAPS, TILES_SHARED_AUX, TILES_INDEPENDENT_AUX = 0, 1, 2, 3
HDR = 48  # 32 name bytes + 16 header bytes


def rule(tlut, np_, texels, w, h):
    """tlut: list of u16 (np_ entries, pad with opaque white). texels: list of palette indices, row-major, stride w.
    Returns dict index -> new u16 for the changed alpha-0 entries."""
    key = [0] * 256
    pr = [0] * 256; pg = [0] * 256; pb = [0] * 256
    for i in range(np_):
        p = tlut[i] if i < len(tlut) else 0xFFFF
        key[i] = 0 if (p & 1) else 1
        pr[i] = (p >> 11) & 31; pg[i] = (p >> 6) & 31; pb[i] = (p >> 1) & 31
    sr = [0] * 256; sg = [0] * 256; sb = [0] * 256; sn = [0] * 256
    for y in range(h):
        for x in range(w):
            idx = texels[y * w + x]
            if not key[idx]:
                continue
            for k in range(4):
                nx = x + (-1 if k == 0 else 1 if k == 1 else 0)
                ny = y + (-1 if k == 2 else 1 if k == 3 else 0)
                nx = min(max(nx, 0), w - 1)
                ny = min(max(ny, 0), h - 1)
                ni = texels[ny * w + nx]
                if key[ni]:
                    continue
                sr[idx] += pr[ni]; sg[idx] += pg[ni]; sb[idx] += pb[ni]; sn[idx] += 1
    out = {}
    for i in range(min(np_, len(tlut))):
        if not key[i] or not sn[i]:
            continue
        c = ((sr[i] // sn[i]) << 11) | ((sg[i] // sn[i]) << 6) | ((sb[i] // sn[i]) << 1)
        if c != tlut[i]:
            out[i] = c
    return out


def read_png_alpha0(path):
    """Count of palette entries with alpha 0 in the PNG (tRNS zeros), and the PLTE length. None if not a palette PNG."""
    try:
        data = open(path, "rb").read()
    except OSError:
        return None
    pos, plte_n, zeros = 8, None, 0
    while pos + 8 <= len(data):
        ln, = struct.unpack(">I", data[pos:pos + 4]); typ = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + ln]
        if typ == b"PLTE":
            plte_n = ln // 3
        elif typ == b"tRNS":
            zeros = sum(1 for a in body if a == 0)
        pos += 12 + ln
    if plte_n is None:
        return None
    return zeros, plte_n


def mip_texels(w, h, depth):
    """Texel count of the mip levels tex.py writes after the main image (TILES_MIPMAPS)."""
    total = 0
    div = 2
    if w >= (32 >> depth):
        while True:
            if w // div <= 0:
                break
            total += (w // div) * (h // div)
            div *= 2
            if (w // div) < (16 >> depth):
                break
    return total


def bake(asset_base, rom_path):
    rom = open(rom_path, "rb").read()
    tex_dir = os.path.join(asset_base, "assets", "us", "mapfs", "tex")
    patches = {}  # addr -> value (u16)
    st = {"archives": 0, "found": 0, "fixed": 0, "changed": 0, "skip_nf": 0, "skip_aux": 0, "skip_hdr": 0, "skip_png": 0, "skip_mip": 0, "textures": 0}
    for jf in sorted(os.listdir(tex_dir)):
        if not jf.endswith("_tex.json"):
            continue
        st["archives"] += 1
        tex_name = jf[:-5]
        for t in json.load(open(os.path.join(tex_dir, jf), encoding="utf-8")):
            m = t.get("main", {})
            if m.get("format") not in ("CI4", "CI8"):
                continue
            st["textures"] += 1
            name = t["name"].encode("ascii")
            depth = 0 if m["format"] == "CI4" else 1
            np_ = 16 if depth == 0 else 256
            # the ROM record: the padded name, then the header
            rec = None
            start = 0
            cands = []
            while True:
                p = rom.find(name, start)
                if p < 0:
                    break
                start = p + 1
                if any(rom[p + len(name):p + 32]) or p + HDR > len(rom):
                    continue
                cands.append(p)
            if not cands:
                st["skip_nf"] += 1
                continue
            for p in cands:
                aw, mw, ah, mh, var, et, comb, fm, dp, hw, vw, flt = struct.unpack(">HHHHBBBBBBBB", rom[p + 32:p + 48])
                fmt_lo, dep_lo = fm & 15, dp & 15
                if fmt_lo != FMT_CI or dep_lo != depth or et not in (TILES_BASIC, TILES_MIPMAPS):
                    if et in (TILES_SHARED_AUX, TILES_INDEPENDENT_AUX):
                        st["skip_aux"] += 1
                    else:
                        st["skip_hdr"] += 1
                    continue
                if mw == 0 or mh == 0 or aw != 0:
                    st["skip_hdr"] += 1
                    continue
                st["found"] += 1
                texels = mw * mh
                if et == TILES_MIPMAPS:
                    texels += mip_texels(mw, mh, depth)
                rast = texels // 2 if depth == 0 else texels
                pal_off = p + HDR + rast
                if pal_off + np_ * 2 > len(rom):
                    st["skip_hdr"] += 1
                    continue
                # the PNG tRNS zeros plus the padding entries must equal the ROM alpha-0 count
                png = read_png_alpha0(os.path.join(tex_dir, tex_name, t["name"] + ".png"))
                rom_pal = [struct.unpack(">H", rom[pal_off + 2 * i:pal_off + 2 * i + 2])[0] for i in range(np_)]
                rom_a0 = sum(1 for v in rom_pal if not (v & 1))
                if png is not None and rom_a0 != png[0] + (np_ - png[1]):
                    st["skip_png"] += 1
                    continue
                # texels: N64 CI4 is two texels per byte, high nibble first; rows stride mw texels
                if depth == 0:
                    raw = rom[p + HDR:p + HDR + rast]
                    texel = [((raw[(y * mw + x) >> 1] >> (4 if ((y * mw + x) & 1) == 0 else 0)) & 15) for y in range(mh) for x in range(mw)]
                else:
                    raw = rom[p + HDR:p + HDR + rast]
                    texel = list(raw[:texels])
                ch = rule(rom_pal, np_, texel[:mw * mh], mw, mh)
                st["fixed"] += 1
                for i, v in ch.items():
                    a = pal_off + 2 * i
                    patches[a] = v
                    st["changed"] += 1
    return patches, st


def write_header(patches, out):
    recs = sorted(patches.items())
    lines = ["/* TEXKEY2: generated by 3ds/tools/texkey_bake.py (build.sh). Do not edit. Ignored by git. */",
             "#ifndef TEXKEY_PATCH_H", "#define TEXKEY_PATCH_H",
             "typedef struct { unsigned addr; unsigned short val; } TkPatch; /* ROM address, big endian value (2 bytes at addr) */",
             "#define TK_PATCH_N %du" % len(recs),
             "static const TkPatch tk_patch[] = {"]
    for a, v in recs:
        lines.append("    { 0x%08Xu, 0x%04Xu }," % (a, v))
    lines += ["};", "#endif"]
    with open(out, "w") as f:
        f.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    base, out = sys.argv[1], sys.argv[2]
    rom_path = os.path.join(base, "ver", "us", "baserom.z64")
    patches, st = bake(base, rom_path)
    write_header(patches, out)
    print("texkey_bake: archives %(archives)d textures %(textures)d found %(found)d fixed %(fixed)d "
          "entries %(changed)d skipped: not found %(skip_nf)d aux %(skip_aux)d header %(skip_hdr)d png %(skip_png)d" % st)
