"""NDS1 phase B step 1: check the word layout of two captured static model records.

Reads 3ds/tools/nat_fixture_toad.txt (two NDUMP blocks from the rec_done dump on census_toad, frame 200 window).
Splits each record into PICA commands and asserts that the words which differ between the two models are only
the per-draw words (unit 0 texture block 0x081, palette uniform 0x2C1, index address 0x227, index count 0x228) or
template-key words (TEV block 0x0C0 to 0x0FF and the extra TEV update commands 0x0E0, 0x0FD, 0x0E1).
Run: python -I 3ds/tools/nat_layout_check.py 3ds/tools/nat_fixture_toad.txt
"""
import difflib, re, sys

PER_DRAW = {0x081, 0x227, 0x228}
PALETTE = 0x2C1
TEV_KEY = set(range(0xC0, 0x100)) | {0xE0, 0xFD, 0xE1}


def blocks(path):
    out, cur = [], None
    for line in open(path, encoding="utf-8"):
        m = re.search(r"NDUMP slot (\d+) n (\d+)", line)
        if m:
            cur = {"slot": int(m.group(1)), "n": int(m.group(2)), "w": []}
            out.append(cur)
            continue
        m = re.search(r"NW [0-9a-f]{4} (.*)$", line.strip())
        if m and cur is not None:
            cur["w"] += [int(x, 16) for x in m.group(1).split()]
    for b in out:
        b["w"] = b["w"][: b["n"]]
    return out


def cmds(w):
    out, i = [], 0
    while i + 1 < len(w):
        hd = w[i + 1]
        reg, c, ln = hd & 0x3FF, (hd >> 20) & 0xFF, 2 + ((hd >> 20) & 0xFF) + (((hd >> 20) & 0xFF) & 1)
        assert i + ln <= len(w), "command runs past the record"
        out.append((i, reg, tuple(w[i:i + ln])))
        i += ln
    assert i == len(w), "record does not end on a command"
    return out


def main(path):
    recs = blocks(path)
    assert len(recs) == 2, "need two records"
    a, b = cmds(recs[0]["w"]), cmds(recs[1]["w"])
    ka = [(r, w) for _, r, w in a]
    kb = [(r, w) for _, r, w in b]
    diff_regs = set()
    for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, ka, kb, autojunk=False).get_opcodes():
        if tag == "equal":
            continue
        for k in range(i1, i2):
            diff_regs.add(a[k][1])
        for k in range(j1, j2):
            diff_regs.add(b[k][1])
    print("record A slot %d n %d cmds %d; record B slot %d n %d cmds %d" % (recs[0]["slot"], recs[0]["n"], len(a), recs[1]["slot"], recs[1]["n"], len(b)))
    print("registers that differ between the two models: " + " ".join("%03x" % r for r in sorted(diff_regs)))
    unknown = diff_regs - PER_DRAW - {PALETTE} - TEV_KEY
    assert not unknown, "unexpected differing registers: %s" % sorted(hex(r) for r in unknown)
    assert diff_regs & PER_DRAW, "no per-draw register differs (check the fixture)"
    print("PASS: differing words are per-draw (0x081, 0x227, 0x228), palette (0x2C1) or template key (TEV)")


if __name__ == "__main__":
    main(sys.argv[1])
