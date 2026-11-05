#!/usr/bin/env python3
"""nd_cmd_dump.py: decode the PICA command words of the kind 6 records of an ndref v4 file.

  python 3ds/tools/nd_cmd_dump.py <file.bin> [draw index ...]     one line per register write of the draws (all draws without an index)
  python 3ds/tools/nd_cmd_dump.py --stats <file.bin> ...          words per draw, registers per draw, the 20 most common registers

A command is [param, header, extra params..., pad]: header bits 0-15 register, 16-19 byte mask, 20-27 extra parameter count, bit 31 consecutive registers.
Its length in words is 2 + extra + (extra & 1) (the list is 8 byte aligned).
"""
import sys, struct, collections
sys.path.insert(0, __file__.rsplit("tools", 1)[0] + "tools")
import nd_ref_dump as rd


def decode(words):
    out, i = [], 0
    while i + 1 < len(words):
        p0, hd = words[i], words[i + 1]
        reg, mask, extra, consec = hd & 0xFFFF, (hd >> 16) & 0xF, (hd >> 20) & 0xFF, (hd >> 31) & 1
        n = 2 + extra + (extra & 1)
        if i + n > len(words):
            out.append((reg, mask, consec, [p0], "truncated"))
            break
        out.append((reg, mask, consec, [p0] + list(words[i + 2:i + 2 + extra]), ""))
        i += n
    return out


def draws(path):
    hdr, recs = rd.parse(path)
    return [r["words"] for r in recs if r["kind"] == rd.KIND_CMD]


def main(argv):
    stats = "--stats" in argv
    args = [a for a in argv if a != "--stats"]
    if not args:
        print(__doc__)
        return 2
    if stats:
        regs = collections.Counter(); nd = 0; nw = 0
        for p in args:
            for w in draws(p):
                nd += 1; nw += len(w)
                for reg, mask, consec, vals, bad in decode(w):
                    regs[reg] += 1
        print("draws %d words %d avg %.0f" % (nd, nw, nw / max(nd, 1)))
        for reg, n in regs.most_common(20):
            print("reg %03x %d" % (reg, n))
        return 0
    path, idx = args[0], [int(a) for a in args[1:]]
    ds = draws(path)
    for k, w in enumerate(ds):
        if idx and k not in idx:
            continue
        print("draw %d words %d" % (k, len(w)))
        for reg, mask, consec, vals, bad in decode(w):
            print("  reg %03x mask %x %s n %d %s%s" % (reg, mask, "inc" if consec else "one", len(vals), " ".join("%08x" % v for v in vals[:8]), (" ..." if len(vals) > 8 else "") + bad))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
