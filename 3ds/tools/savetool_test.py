#!/usr/bin/env python3
"""Round-trip self-test for savetool.py: make, dump, checksum validates, edit, flash. Run: python 3ds/tools/savetool_test.py"""
import json
import os
import struct
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import savetool as st  # noqa: E402

JSON = {
    "boots": 1, "hammer": 2, "hp": 12, "maxhp": 15, "fp": 4, "maxfp": 9, "bp": 3, "level": 5,
    "coins": 321, "partner": 2, "story": 7, "area": 1, "map": 4, "entry": 2, "pos": [-264, 20, -326],
    "items": [5, 9], "badges": [33, 34, 35], "flags": {"3": 1}, "bytes": {"9": 77},
    "areaFlags": {"1": 4}, "areaBytes": {"2": 6},
}

with tempfile.TemporaryDirectory() as tmp:
    j = os.path.join(tmp, "in.json")
    sav = os.path.join(tmp, "a.sav")
    img = os.path.join(tmp, "save.flash")
    with open(j, "w") as f:
        json.dump(JSON, f)

    assert st.main(["new", j, sav]) == 0
    buf = open(sav, "rb").read()
    assert len(buf) == st.SAVE_SIZE == 0x1380
    assert st.valid(buf), "new file must pass the checksum"
    assert buf[0:15] == b"Mario Story 006" and buf[15] == 0

    assert st.main(["dump", sav]) == 0
    p = 0x40
    assert buf[p + 2] == 12 and buf[p + 3] == 15 and buf[p + 9] == 5
    assert struct.unpack_from("<h", buf, p + 0x0C)[0] == 321
    assert buf[0x10B0] == 7                                   # story = globalBytes[0]
    assert struct.unpack_from("<h", buf, 0x12E6) == (-264,)   # savePos x
    assert struct.unpack_from("<h", buf, p + 0xB4) == (33,)   # badges[0]
    assert struct.unpack_from("<i", buf, 0x0FB0 + 3 * 4) == (1,)
    assert struct.unpack_from("<i", buf, 0x12B0 + 4) == (4,)

    # a single changed byte must break the checksum
    bad = bytearray(buf)
    bad[0x200] ^= 1
    assert not st.valid(bytes(bad))

    # edit in place keeps the checksum valid
    e = os.path.join(tmp, "e.json")
    with open(e, "w") as f:
        json.dump({"coins": 999, "story": 8}, f)
    assert st.main(["edit", e, sav]) == 0
    buf2 = open(sav, "rb").read()
    assert st.valid(buf2)
    assert struct.unpack_from("<h", buf2, p + 0x0C)[0] == 999 and buf2[0x10B0] == 8

    # flash writes the save at slot * 0x4000 and keeps the rest of the image erased
    assert st.main(["flash", sav, "--slot", "2", "--image", img]) == 0
    image = open(img, "rb").read()
    assert len(image) == st.FLASH_SIZE == 131072
    assert image[2 * 0x4000:2 * 0x4000 + st.SAVE_SIZE] == buf2
    assert image[0:0x4000] == b"\xff" * 0x4000
    assert image[2 * 0x4000 + st.SAVE_SIZE:3 * 0x4000] == b"\xff" * (0x4000 - st.SAVE_SIZE)
    assert st.valid(image[2 * 0x4000:2 * 0x4000 + st.SAVE_SIZE])

    # a file that fails the checksum is refused
    open(sav, "wb").write(bytes(bad))
    assert st.main(["flash", sav, "--image", img]) == 1

print("savetool_test: all checks passed")
