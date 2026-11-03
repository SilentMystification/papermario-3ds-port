# Dump the SMDH titles of a 3DSX file. Usage: python -I smdh_dump.py <file.3dsx>
import struct, sys

def utf16(b):
    return b.decode("utf-16-le").split("\x00")[0]

with open(sys.argv[1], "rb") as f:
    d = f.read()
assert d[0:4] == b"3DSX", "not a 3DSX file"
hdr_size = struct.unpack_from("<H", d, 4)[0]
assert hdr_size >= 0x20, "no extended header"
smdh_off, smdh_size, romfs_off = struct.unpack_from("<III", d, 0x20)
s = d[smdh_off:smdh_off + smdh_size]
assert s[0:4] == b"SMDH", "bad SMDH magic"
names = ["Japanese", "English"]
for i in range(16):
    base = 0x08 + i * 0x200
    short = utf16(s[base:base + 0x80])
    long_ = utf16(s[base + 0x80:base + 0x180])
    pub = utf16(s[base + 0x180:base + 0x200])
    if i < 2 or short:
        print(f"title {i} ({names[i] if i < 2 else 'other'}): short='{short}' long='{long_}' publisher='{pub}'")
