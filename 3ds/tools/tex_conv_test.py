#!/usr/bin/env python3
"""Host test of the texture converter (TX1 path, 3ds/src/pm_tex_tx1.h, driver 3ds/tools/tex_conv_drive.c).

Reference, written from the N64 and PICA formats and not from the port code:
  1. read the N64 texel field (I4, I8, IA4, IA8, IA16, CI4, CI8, RGBA16, RGBA32) from the random source bytes,
  2. map the PICA texel (x, y) to the N64 texel: PICA memory row y holds the logical row ph-1-y, a window mask
     (mask smaller than the image) folds the coordinate (repeat or mirror), else the edge texel repeats,
  3. encode the texel in the PICA layout: RGBA8 bytes A B G R; RGBA5551 R5 G5 B5 A1 (bit 15 down to bit 0);
     LA8 bytes A then L; LA4 one byte, L high nibble, A low nibble; L8 one byte,
  4. place the texel in 8x8 tiles, texels inside a tile in Morton order (x0, y0, x1, y1, x2, y2 from bit 0 up).
Exit 1 when any texel differs. Usage (repo root): python3 -I 3ds/tools/tex_conv_test.py <driver exe>
"""
import os
import random
import struct
import subprocess
import sys
import tempfile
from collections import defaultdict

FMT_RGBA, FMT_CI, FMT_IA, FMT_I = 'RGBA', 'CI', 'IA', 'I'
GPU_NAME = {0: 'RGBA8', 2: 'RGBA5551', 5: 'LA8', 7: 'L8', 9: 'LA4'}
GPU_BPP = {0: 4, 2: 2, 5: 2, 7: 1, 9: 1}
FMT_CODE = {FMT_RGBA: 0, FMT_CI: 2, FMT_IA: 3, FMT_I: 4}
SIZ_CODE = {4: 0, 8: 1, 16: 2, 32: 3}


def pot8(n):
    p = 8
    while p < n:
        p *= 2
    return p


def expected_gf(fmt, siz):
    """The PICA format each N64 format maps to (the port mapping, stated as a spec)."""
    if fmt == FMT_RGBA and siz == 32:
        return 0
    if fmt == FMT_RGBA and siz == 16:
        return 2
    if fmt == FMT_CI:
        return 2
    if fmt == FMT_IA and siz == 16:
        return 5
    if fmt == FMT_I and siz == 8:
        return 7
    if (fmt == FMT_I and siz == 4) or (fmt == FMT_IA and siz in (4, 8)):
        return 9
    raise ValueError((fmt, siz))


def morton(x, y):
    m = 0
    for i in range(3):
        m |= ((x >> i) & 1) << (2 * i)
        m |= ((y >> i) & 1) << (2 * i + 1)
    return m


def pica_addr(x, y, pw, bpp):
    """Byte offset of PICA texel (x, y) in the tiled image."""
    tile = (y >> 3) * (pw >> 3) + (x >> 3)
    return (tile * 64 + morton(x & 7, y & 7)) * bpp


def map_coord(v, n, win, mirror):
    """Logical coordinate to source coordinate. win = window width (0 = no window, or window not smaller than the image)."""
    if win:
        period = 2 * win if mirror else win
        r = v % period
        if mirror and r >= win:
            r = 2 * win - 1 - r
        return r
    return min(v, n - 1)


def n64_field(fmt, siz, data, stride, x, y, native):
    """Raw N64 texel field at (x, y). Returns an int (4, 8, 16 bit) or a 4-tuple (R, G, B, A) for RGBA32."""
    idx = y * stride + x
    if siz == 4:
        b = data[idx >> 1]
        return (b >> 4) if idx % 2 == 0 else (b & 15)
    if siz == 8:
        return data[idx]
    if siz == 16:
        p = idx * 2
        return (data[p] | (data[p + 1] << 8)) if native else ((data[p] << 8) | data[p + 1])
    p = idx * 4
    return (data[p], data[p + 1], data[p + 2], data[p + 3])


def rgba16_to_5551(c):
    """N64 RGBA16 field: R5 bits 15-11, G5 10-6, B5 5-1, A1 bit 0. PICA RGBA5551 uses the same field order."""
    r, g, b, a = c >> 11, (c >> 6) & 31, (c >> 1) & 31, c & 1
    return (r << 11) | (g << 6) | (b << 1) | a


def ref_texel(fmt, siz, data, tlut, tlut_n, stride, x, y, native):
    """PICA texel value (bytes, little endian) of the N64 source texel (x, y)."""
    if fmt == FMT_RGBA and siz == 32:
        r, g, b, a = n64_field(fmt, siz, data, stride, x, y, native)
        return bytes([a, b, g, r])                              # RGBA8 in PICA memory: A B G R
    if fmt == FMT_RGBA and siz == 16:
        return struct.pack('<H', rgba16_to_5551(n64_field(fmt, siz, data, stride, x, y, native)))
    if fmt == FMT_CI:
        idx = n64_field(fmt, siz, data, stride, x, y, False)
        assert idx < tlut_n, 'request uses an index outside the palette'
        c = (tlut[2 * idx] << 8) | tlut[2 * idx + 1]           # TLUT entries are RGBA16, big endian in the ROM
        return struct.pack('<H', rgba16_to_5551(c))
    if fmt == FMT_IA and siz == 16:
        c = n64_field(fmt, siz, data, stride, x, y, False)
        i8, a8 = c >> 8, c & 255
        return bytes([a8, i8])                                  # LA8 in PICA memory: alpha, then luminance
    if fmt == FMT_I and siz == 8:
        return bytes([n64_field(fmt, siz, data, stride, x, y, False)])
    if fmt == FMT_I and siz == 4:
        i4 = n64_field(fmt, siz, data, stride, x, y, False)
        return bytes([(i4 << 4) | i4])                          # intensity is luminance and alpha
    if fmt == FMT_IA and siz == 8:
        b = n64_field(fmt, siz, data, stride, x, y, False)
        return bytes([((b >> 4) << 4) | (b & 15)])              # I4 A4 in one byte, L high nibble
    if fmt == FMT_IA and siz == 4:
        nib = n64_field(fmt, siz, data, stride, x, y, False)
        i3, a1 = nib >> 1, nib & 1
        l4 = (i3 * 30 + 7) // 14                                # round(I3 * 15 / 7), I3 in 0..7
        a4 = 15 if a1 else 0
        return bytes([(l4 << 4) | a4])
    raise ValueError((fmt, siz))


def build_case(case):
    """Return (expected bytes, expected gf) for one request."""
    fmt, siz, native, w, h, stride = case['fmt'], case['siz'], case['native'], case['w'], case['h'], case['stride']
    pw, ph = pot8(w), pot8(h)
    gf = expected_gf(fmt, siz)
    bpp = GPU_BPP[gf]
    mw = case['mw'] if (case['mw'] and case['mw'] < w) else 0
    mh = case['mh'] if (case['mh'] and case['mh'] < h) else 0
    ms, mt = case['ms'], case['mt']
    tlut = case['tlut']
    tlut_n = len(tlut) // 2
    out = bytearray(pw * ph * bpp)
    for my in range(ph):
        ly = ph - 1 - my
        sy = map_coord(ly, h, mh, mt)
        for mx in range(pw):
            sx = map_coord(mx, w, mw, ms)
            val = ref_texel(fmt, siz, case['src'], tlut, tlut_n, stride, sx, sy, native)
            a = pica_addr(mx, my, pw, bpp)
            out[a:a + bpp] = val
    return bytes(out), gf, pw, ph


def make_cases(rng):
    combos = [(FMT_RGBA, 16, 0), (FMT_RGBA, 16, 1), (FMT_RGBA, 32, 0), (FMT_IA, 16, 0), (FMT_CI, 4, 0), (FMT_CI, 8, 0),
              (FMT_I, 4, 0), (FMT_I, 8, 0), (FMT_IA, 4, 0), (FMT_IA, 8, 0)]
    sizes = [(8, 8), (16, 16), (32, 32), (64, 64), (64, 32), (32, 64), (16, 64), (64, 16), (24, 16), (40, 8)]
    cases = []
    for fmt, siz, native in combos:
        for (w, h) in sizes:
            for stride_mode in ('w', 'w3', '2w'):
                stride = {'w': w, 'w3': w + 3, '2w': 2 * w}[stride_mode]
                for wrap in ('none', 'repeat', 'mirror'):
                    mw = mh = ms = mt = 0
                    if wrap != 'none':
                        if w >= 4:
                            mw = 1 << rng.randint(1, (w).bit_length() - 2)
                            ms = 1 if wrap == 'mirror' else 0
                        if h >= 4 and rng.random() < 0.7:
                            mh = 1 << rng.randint(1, (h).bit_length() - 2)
                            mt = ms
                    texels = (h - 1) * stride + w
                    if siz == 4:
                        nbytes = (texels + 1) // 2 + 1
                    elif siz == 8:
                        nbytes = texels + 1
                    elif siz == 16:
                        nbytes = texels * 2 + 2
                    else:
                        nbytes = texels * 4 + 4
                    src = bytes(rng.getrandbits(8) for _ in range(nbytes))
                    tlut = b''
                    if fmt == FMT_CI:
                        n = 16 if siz == 4 else 256
                        tlut = bytes(rng.getrandbits(8) for _ in range(2 * n))
                        if siz == 4:
                            src = bytes(rng.getrandbits(8) for _ in range((texels + 1) // 2 + 1))
                        # CI indices must stay inside the palette (the port draws out-of-range texels white)
                        if siz == 8:
                            src = bytes(v % n for v in src)
                    cases.append({'fmt': fmt, 'siz': siz, 'native': native, 'w': w, 'h': h, 'stride': stride,
                                  'mw': mw, 'mh': mh, 'ms': ms, 'mt': mt, 'src': src, 'tlut': tlut,
                                  'wrap': wrap, 'stride_mode': stride_mode})
    return cases


def write_request(path, cases):
    with open(path, 'wb') as f:
        for c in cases:
            hdr = [FMT_CODE[c['fmt']], SIZ_CODE[c['siz']], c['native'], c['w'], c['h'], c['stride'], c['mw'], c['mh'],
                   c['ms'], c['mt'], len(c['tlut']) // 2, len(c['src']), len(c['tlut'])]
            f.write(struct.pack('<13i', *hdr))
            f.write(c['src'])
            f.write(c['tlut'])


def read_output(path, n):
    res = []
    with open(path, 'rb') as f:
        for _ in range(n):
            gf, ln = struct.unpack('<ii', f.read(8))
            res.append((gf, f.read(ln)))
    return res


def main():
    if len(sys.argv) != 2:
        print('usage: tex_conv_test.py <driver exe>')
        return 2
    rng = random.Random(20261104)
    cases = make_cases(rng)
    tmp = tempfile.mkdtemp(prefix='texconv_')
    req = os.path.join(tmp, 'req.bin')
    outp = os.path.join(tmp, 'out.bin')
    write_request(req, cases)
    exe = sys.argv[1]
    if not os.path.exists(exe) and os.path.exists(exe + '.exe'):
        exe += '.exe'                                   # Windows gcc appends .exe
    subprocess.run([exe, req, outp], check=True)
    got = read_output(outp, len(cases))

    fails = []
    summary = defaultdict(lambda: [0, 0])          # (format, siz) -> [cases, failing cases]
    by_stride = defaultdict(lambda: [0, 0])        # stride mode -> [cases, failing cases]
    by_size = defaultdict(lambda: [0, 0])
    by_wrap = defaultdict(lambda: [0, 0])
    for i, c in enumerate(cases):
        exp, egf, pw, ph = build_case(c)
        ggf, gout = got[i]
        key = (c['fmt'] + str(c['siz']) + ('n' if c['native'] else ''))
        bad = None
        if ggf != egf:
            bad = 'format got %s want %s' % (GPU_NAME.get(ggf, ggf), GPU_NAME[egf])
        elif gout != exp:
            n = sum(1 for k in range(len(exp)) if exp[k] != gout[k])
            first = next(k for k in range(len(exp)) if exp[k] != gout[k])
            bpp = GPU_BPP[egf]
            addr = first // bpp
            bad = 'bytes differ %d of %d, first at tile addr %d (x %d y %d)' % (
                n, len(exp), addr, addr % pw, addr // pw)
        for table, k in ((summary, key), (by_stride, c['stride_mode']), (by_size, '%dx%d' % (c['w'], c['h'])),
                         (by_wrap, c['wrap'])):
            table[k][0] += 1
        if bad:
            for table, k in ((summary, key), (by_stride, c['stride_mode']), (by_size, '%dx%d' % (c['w'], c['h'])),
                             (by_wrap, c['wrap'])):
                table[k][1] += 1
            fails.append((c, bad))

    print('tex_conv_test: %d requests, %d failing' % (len(cases), len(fails)))
    print('per N64 format (fmt+size, n = native RGBA16):')
    for k in sorted(summary):
        print('  %-8s cases %4d  fail %4d  %s' % (k, summary[k][0], summary[k][1], 'FAIL' if summary[k][1] else 'ok'))
    print('per stride mode:', ', '.join('%s %d/%d' % (k, v[1], v[0]) for k, v in sorted(by_stride.items())))
    print('per wrap mode:', ', '.join('%s %d/%d' % (k, v[1], v[0]) for k, v in sorted(by_wrap.items())))
    print('per size (failing/cases):', ', '.join('%s %d/%d' % (k, v[1], v[0]) for k, v in sorted(by_size.items()) if v[1]) or 'none fail')
    for c, bad in fails[:12]:
        print('  FAIL %s%d native %d %dx%d stride %d mask w %d h %d ms %d mt %d: %s' % (
            c['fmt'], c['siz'], c['native'], c['w'], c['h'], c['stride'], c['mw'], c['mh'], c['ms'], c['mt'], bad))
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
