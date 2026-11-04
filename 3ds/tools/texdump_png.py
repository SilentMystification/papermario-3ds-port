"""Decode a wall texture dump pair to PNG (pure Python, no PIL).
texsrc: N64 I4 source bytes, 64x64, row-major, high nibble first (the ROM bytes the converter read).
texgpu: PICA LA4 bytes of the uploaded texture, 64x64, 8x8 tiles, Morton order inside a tile,
        memory row y holds the logical row 63 - y (t = 0 at the last memory row).
Writes src.png (N64 decode) and gpu.png (PICA decode, mapped back to N64 rows) and prints the pixel difference.
Usage (repo root): python texdump_png.py <texsrc.bin> <texgpu.bin> <out_dir>. Dumps: texword=shape (texsrc_*.bin, texgpu_*.bin) in the Azahar sdmc folder.
"""
import struct
import sys
import zlib


def png(path, w, h, rgb):
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        raw += rgb[y * w * 3:(y + 1) * w * 3]

    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    data = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
    data += chunk(b'IDAT', zlib.compress(bytes(raw), 9)) + chunk(b'IEND', b'')
    with open(path, 'wb') as f:
        f.write(data)


def morton(x, y):
    m = 0
    for i in range(3):
        m |= ((x >> i) & 1) << (2 * i)
        m |= ((y >> i) & 1) << (2 * i + 1)
    return m


def scale(img, n, w, h):
    """img: list of (L, A) per logical texel, row 0 at top. Returns RGB bytes scaled by n, over a dark checker for alpha."""
    out = bytearray()
    W, H = w * n, h * n
    for y in range(H):
        for x in range(W):
            L, A = img[(y // n) * w + (x // n)]
            chk = 60 if ((x // 8 + y // 8) & 1) else 110
            v = (L * A + chk * (255 - A)) // 255
            out += bytes((v, v, v))
    return W, H, bytes(out)


def main():
    src_p, gpu_p, out_dir = sys.argv[1], sys.argv[2], sys.argv[3]
    src = open(src_p, 'rb').read()
    gpu = open(gpu_p, 'rb').read()
    W = H = 64
    # N64 decode: I4, intensity = alpha = nibble
    s_img = []
    for y in range(H):
        for x in range(W):
            idx = y * W + x
            b = src[idx >> 1]
            n = (b >> 4) if idx % 2 == 0 else (b & 15)
            s_img.append((n * 17, n * 17))
    # PICA decode: LA4 one byte per texel, L high nibble, A low nibble; tile order 8x8 Morton; row y -> logical 63 - y
    g_img = [None] * (W * H)
    for my in range(H):
        for mx in range(W):
            tile = (my >> 3) * (W >> 3) + (mx >> 3)
            byte = gpu[tile * 64 + morton(mx & 7, my & 7)]
            L, A = byte >> 4, byte & 15
            g_img[(H - 1 - my) * W + mx] = (L * 17, A * 17)
    diff = sum(1 for k in range(W * H) if s_img[k] != g_img[k])
    print('decoded texels differing (N64 source vs PICA dump): %d of %d' % (diff, W * H))
    print('first rows of N64 nibbles row 0 %s' % ' '.join('%x' % (src[k] >> 4) for k in range(4)))
    print('first logical row of PICA dump: %s' % ' '.join('%x' % (g_img[k][0] // 17) for k in range(8)))
    for name, img in (('src', s_img), ('gpu', g_img)):
        w, h, rgb = scale(img, 4, W, H)
        png('%s/%s.png' % (out_dir, name), w, h, rgb)
    print('wrote %s/src.png and %s/gpu.png' % (out_dir, out_dir))


if __name__ == '__main__':
    main()
