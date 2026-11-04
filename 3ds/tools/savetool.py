#!/usr/bin/env python3
"""Make, edit, dump and flash a Paper Mario SaveData file (0x1380 bytes, little-endian).

Layout: include/common_structs.h (SaveData at line 2111, PlayerData at line 310, PartnerData at line 155).
Checksum: src/fio.c fio_save_game (crc1 = 32-bit sum of all words with crc1 = 0, crc2 = ~0 during the sum; crc2 = ~crc1)
and fio_validate_file_checksum (magic, crc1 == ~crc2, sum of all stored words == crc1).
Flash: src/fio.c reads 128-byte pages; physical save slot n starts at byte n * 0x4000 (one 16 KB sector) of save.flash.
3ds/src/pm_os.c keeps save.flash as a 128 KB image (8 sectors). Slot 0..5 are saves, 6 and 7 are the globals.

Commands:
  savetool.py new  [in.json|-] out.sav     blank file plus the JSON overlay
  savetool.py edit in.json|-  file.sav     change an existing file in place (overlay, then checksum)
  savetool.py dump file.sav                print the fields and the checksum state
  savetool.py flash file.sav [--slot N] [--image save.flash]   write one save into the flash image

JSON keys (all optional, numbers are integers):
  boots, hammer, hp, maxhp, fp, maxfp, bp, level, coins, partner, story,
  area, map, entry, pos [x, y, z], saveSlot, saveCount,
  items [ids, up to 10], badges [ids, up to 128],
  flags {"index": value} (globalFlags, s32), bytes {"index": value} (globalBytes, s8),
  areaFlags {"index": value} (s32), areaBytes {"index": value} (s8).
  story is globalBytes[0] (story progress). maxhp, maxfp default to hp, fp.
  partner enables that partner (PartnerData.enabled) and sets curPartner.
"""
import argparse
import json
import os
import struct
import sys

SAVE_SIZE = 0x1380
NWORDS = SAVE_SIZE // 4
MAGIC = b"Mario Story 006"
SECTOR = 0x4000
FLASH_SIZE = 8 * SECTOR
PARTNER_BASE = 0x40 + 0x14  # PlayerData.partners, 8 bytes each


def u32(x):
    return x & 0xFFFFFFFF


def words(buf):
    return list(struct.unpack("<%di" % NWORDS, buf))


def pack(ws):
    return struct.pack("<%di" % NWORDS, *ws)


def checksum_sum(ws):
    s = 0
    for w in ws:
        s = (s + w) & 0xFFFFFFFF
    return s


def seal(buf):
    """Set crc1 and crc2 the way fio_save_game does."""
    ws = words(buf)
    ws[0x30 // 4] = 0            # crc1
    ws[0x34 // 4] = -1           # crc2 = ~0
    crc1 = checksum_sum(ws)
    ws[0x30 // 4] = crc1 - (1 << 32) if crc1 & 0x80000000 else crc1
    crc2 = u32(~crc1)
    ws[0x34 // 4] = crc2 - (1 << 32) if crc2 & 0x80000000 else crc2
    return pack(ws)


def valid(buf):
    """Mirror of fio_validate_file_checksum."""
    if len(buf) != SAVE_SIZE or buf[0:15] != MAGIC[:15] or buf[15] != 0:
        return False
    ws = words(buf)
    crc1 = u32(ws[0x30 // 4])
    crc2 = u32(ws[0x34 // 4])
    if crc1 != u32(~crc2):
        return False
    return checksum_sum(ws) == crc1


def blank():
    buf = bytearray(SAVE_SIZE)
    buf[0:len(MAGIC) + 1] = MAGIC + b"\0"
    buf[0x40 + 0x02] = 10  # curHP
    buf[0x40 + 0x03] = 10  # curMaxHP
    buf[0x40 + 0x04] = 10  # hardMaxHP
    buf[0x40 + 0x09] = 1   # level
    buf[0x12F2] = 1        # metadata.level
    return buf


def put8(buf, off, v):
    buf[off] = v & 0xFF


def put16(buf, off, v):
    struct.pack_into("<h", buf, off, v)


def put32(buf, off, v):
    struct.pack_into("<i", buf, off, v)


def overlay(buf, d):
    p = 0x40
    if "boots" in d: put8(buf, p + 0x00, d["boots"])
    if "hammer" in d: put8(buf, p + 0x01, d["hammer"])
    if "hp" in d:
        put8(buf, p + 0x02, d["hp"])
        put8(buf, p + 0x03, d.get("maxhp", d["hp"]))
        put8(buf, p + 0x04, d.get("maxhp", d["hp"]))
    elif "maxhp" in d:
        put8(buf, p + 0x03, d["maxhp"])
        put8(buf, p + 0x04, d["maxhp"])
    if "fp" in d:
        put8(buf, p + 0x05, d["fp"])
        put8(buf, p + 0x06, d.get("maxfp", d["fp"]))
        put8(buf, p + 0x07, d.get("maxfp", d["fp"]))
    elif "maxfp" in d:
        put8(buf, p + 0x06, d["maxfp"])
        put8(buf, p + 0x07, d["maxfp"])
    if "bp" in d: put8(buf, p + 0x08, d["bp"])
    if "level" in d:
        put8(buf, p + 0x09, d["level"])
        put8(buf, 0x12F2, d["level"])  # metadata.level
    if "coins" in d: put16(buf, p + 0x0C, d["coins"])
    if "partner" in d:
        pid = int(d["partner"])
        put8(buf, p + 0x12, pid)  # curPartner
        if 0 < pid < 12:
            put8(buf, PARTNER_BASE + pid * 8, 1)  # partners[pid].enabled
    if "items" in d:
        if len(d["items"]) > 10: raise ValueError("items: at most 10")
        for i in range(10):
            put16(buf, p + 0x1B4 + i * 2, d["items"][i] if i < len(d["items"]) else 0)
    if "badges" in d:
        if len(d["badges"]) > 128: raise ValueError("badges: at most 128")
        for i in range(128):
            put16(buf, p + 0xB4 + i * 2, d["badges"][i] if i < len(d["badges"]) else 0)
    if "area" in d: put16(buf, 0x468, d["area"])
    if "map" in d: put16(buf, 0x46A, d["map"])
    if "entry" in d: put16(buf, 0x46C, d["entry"])
    if "pos" in d:
        x, y, z = d["pos"]
        put16(buf, 0x12E6, x); put16(buf, 0x12E8, y); put16(buf, 0x12EA, z)
    if "story" in d: put8(buf, 0x10B0, d["story"])
    if "saveSlot" in d: put32(buf, 0x38, d["saveSlot"])
    if "saveCount" in d: put32(buf, 0x3C, d["saveCount"])
    for k, i in (d.get("flags") or {}).items():
        put32(buf, 0x0FB0 + int(k) * 4, i)
    for k, v in (d.get("bytes") or {}).items():
        put8(buf, 0x10B0 + int(k), v)
    for k, v in (d.get("areaFlags") or {}).items():
        put32(buf, 0x12B0 + int(k) * 4, v)
    for k, v in (d.get("areaBytes") or {}).items():
        put8(buf, 0x12D0 + int(k), v)
    return buf


def read_json(path):
    if path == "-":
        return {}
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def read_save(path):
    buf = bytearray(open(path, "rb").read())
    if len(buf) != SAVE_SIZE:
        raise ValueError("%s: %d bytes, need %d" % (path, len(buf), SAVE_SIZE))
    return buf


def cmd_new(a):
    buf = overlay(blank(), read_json(a.json))
    out = seal(bytes(buf))
    with open(a.out, "wb") as f:
        f.write(out)
    print("wrote %s valid=%s" % (a.out, valid(out)))
    return 0


def cmd_edit(a):
    buf = overlay(read_save(a.file), read_json(a.json))
    out = seal(bytes(buf))
    with open(a.file, "wb") as f:
        f.write(out)
    print("edited %s valid=%s" % (a.file, valid(out)))
    return 0


def s16(buf, off):
    return struct.unpack_from("<h", buf, off)[0]


def cmd_dump(a):
    buf = bytes(read_save(a.file))
    ws = words(buf)
    p = 0x40
    print("magic      %r" % buf[0:16].split(b"\0")[0].decode("latin-1"))
    print("crc1 %d crc2 %d saveSlot %d saveCount %d" % (ws[0x30 // 4], ws[0x34 // 4], ws[0x38 // 4], ws[0x3C // 4]))
    print("boots %d hammer %d" % (buf[p], buf[p + 1]))
    print("hp %d/%d hardMax %d fp %d/%d bp %d level %d" % (buf[p + 2], buf[p + 3], buf[p + 4], buf[p + 5], buf[p + 6], buf[p + 8], buf[p + 9]))
    print("coins %d curPartner %d" % (s16(buf, p + 0x0C), buf[p + 0x12]))
    items = [s16(buf, p + 0x1B4 + i * 2) for i in range(10)]
    badges = [s16(buf, p + 0xB4 + i * 2) for i in range(128)]
    print("items %s" % [i for i in items if i])
    print("badges %s" % [b for b in badges if b])
    print("area %d map %d entry %d" % (s16(buf, 0x468), s16(buf, 0x46A), s16(buf, 0x46C)))
    print("pos %d %d %d" % (s16(buf, 0x12E6), s16(buf, 0x12E8), s16(buf, 0x12EA)))
    print("story %d" % buf[0x10B0])
    print("metadata level %d timePlayed %d" % (buf[0x12F2], struct.unpack_from("<i", buf, 0x12EC)[0]))
    print("valid %s" % valid(buf))
    return 0


def cmd_flash(a):
    buf = bytes(read_save(a.file))
    if not valid(buf):
        print("refused: %s fails the checksum" % a.file)
        return 1
    off = a.slot * SECTOR
    if off + SECTOR > FLASH_SIZE:
        print("refused: slot %d outside the 6 save sectors" % a.slot)
        return 1
    if os.path.exists(a.image):
        img = bytearray(open(a.image, "rb").read())
        if len(img) != FLASH_SIZE:
            raise ValueError("%s: %d bytes, need %d" % (a.image, len(img), FLASH_SIZE))
    else:
        img = bytearray(b"\xff" * FLASH_SIZE)
    img[off:off + SECTOR] = b"\xff" * SECTOR  # erase the sector, as fio_save_game does
    img[off:off + SAVE_SIZE] = buf
    with open(a.image, "wb") as f:
        f.write(img)
    print("flashed %s slot %d at 0x%x valid=%s" % (a.file, a.slot, off, valid(bytes(img[off:off + SAVE_SIZE]))))
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    n = sub.add_parser("new"); n.add_argument("json"); n.add_argument("out"); n.set_defaults(fn=cmd_new)
    e = sub.add_parser("edit"); e.add_argument("json"); e.add_argument("file"); e.set_defaults(fn=cmd_edit)
    d = sub.add_parser("dump"); d.add_argument("file"); d.set_defaults(fn=cmd_dump)
    f = sub.add_parser("flash"); f.add_argument("file"); f.add_argument("--slot", type=int, default=0)
    f.add_argument("--image", default="save.flash"); f.set_defaults(fn=cmd_flash)
    a = ap.parse_args(argv)
    return a.fn(a)


if __name__ == "__main__":
    sys.exit(main())
