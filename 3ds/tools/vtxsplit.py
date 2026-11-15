"""VTXSPLIT: split the gsSPVertex loads that read past their Vtx array end (DGBJOIN).

Usage: vtxsplit.py <vtx dir> <gfx file>
  vtx dir: the folder of the Vtx arrays (<dir>/*.vtx.inc.c, each `Vtx NAME[N] = {`).
  gfx file: a device source with `gsSPVertex(NAME, n, v),` or `gsSPVertex(&NAME[k], n, v),` lines.

A load with k + n > N (N = size of NAME) becomes pieces. The piece c = min(n, N - k) reads NAME from k to slot v. Then n -= c,
v += c, and the next piece reads the array at ROM address NAME + N * 16 from index 0 (ROM order, the Vtx size is 16 bytes).
The next array must exist in the vtx dir, else the tool exits 1. Other lines stay. Line endings (CRLF or LF) stay.
Prints `vtxsplit lines <split lines> loads <new loads>`: a second run prints `vtxsplit lines 0 loads 0`.
"""
import pathlib
import re
import sys

VTX_DEF = re.compile(r"^Vtx\s+(vtx_[0-9A-Fa-f]+)\s*\[\s*(\d+)\s*\]", re.M)
LOAD = re.compile(r"^(\s*)gsSPVertex\(\s*(&?)(vtx_[0-9A-Fa-f]+)(?:\[\s*(\d+)\s*\])?\s*,\s*(\d+)\s*,\s*(\d+)\s*\)\s*(,?)\s*$")
ROM_STEP = 16  # sizeof(Vtx)


def read_sizes(vdir):
    sizes = {}
    for p in sorted(vdir.glob("*.vtx.inc.c")):
        for name, n in VTX_DEF.findall(p.read_text(encoding="utf-8", errors="replace")):
            sizes[name] = int(n)
    return sizes


def split_line(m, sizes):
    """Returns the replacement lines of one load (a list of str without line end), or None when the load fits."""
    ind, amp, name, k, n, v, comma = m.groups()
    if name not in sizes:
        return None
    k = int(k) if k is not None else 0
    n = int(n)
    v = int(v)
    if k + n <= sizes[name]:
        return None
    out = []
    cur, cur_k = name, k
    while n > 0:
        size = sizes[cur]
        if cur_k + n <= size:
            c = n
        else:
            c = size - cur_k
        if c <= 0:
            raise ValueError("empty piece at %s[%d]" % (cur, cur_k))
        src = cur if cur_k == 0 else "&%s[%d]" % (cur, cur_k)
        out.append("%sgsSPVertex(%s, %d, %d)," % (ind, src, c, v))
        n -= c
        v += c
        if n > 0:
            nxt = "vtx_%X" % (int(cur[4:], 16) + size * ROM_STEP)
            if nxt not in sizes:
                raise FileNotFoundError("next array %s after %s is not in the vtx dir" % (nxt, cur))
            cur, cur_k = nxt, 0
    return out


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    vdir = pathlib.Path(sys.argv[1])
    gfx = pathlib.Path(sys.argv[2])
    sizes = read_sizes(vdir)
    raw = gfx.read_bytes().decode("utf-8")
    crlf = "\r\n" in raw
    lines = raw.split("\r\n") if crlf else raw.split("\n")
    out = []
    split_lines = 0
    new_loads = 0
    try:
        for line in lines:
            m = LOAD.match(line.rstrip("\r"))
            rep = split_line(m, sizes) if m else None
            if rep is None:
                out.append(line)
                continue
            split_lines += 1
            new_loads += len(rep)
            out.extend(rep)
    except FileNotFoundError as e:
        print("vtxsplit FAIL %s" % e)
        return 1
    if split_lines:
        text = ("\r\n" if crlf else "\n").join(out)
        gfx.write_bytes(text.encode("utf-8"))
    print("vtxsplit lines %d loads %d" % (split_lines, new_loads))
    return 0


if __name__ == "__main__":
    sys.exit(main())
