#!/usr/bin/env python3
"""NDT3 region diff (fidelity rule of docs/handoff/design_native_draw.md, plan "Fidelity rule").

  python3 -I 3ds/tools/region_diff.py DIR_A DIR_B [--tol 8] [--max-region 64] [--max-pct 2.0]

Pairs the top-screen PPM shots (name not starting with bot_) of two runs. A pixel differs when a channel
differs by more than --tol. The differing pixels form regions (8-connected). A shot passes when every
region has at most --max-region pixels (small scattered differences: one-pixel shifts, sprite edges) and the
differing pixels are at most --max-pct percent of the shot. A whole object that moved or changed texture is
one large region and fails the shot. Exit 0 when every shot passes, 1 otherwise, 2 when the shots do not pair.
"""
import os
import sys


def read_ppm(path):
    with open(path, "rb") as f:
        data = f.read()
    parts, pos = [], 0
    while len(parts) < 4:
        while data[pos:pos + 1].isspace():
            pos += 1
        if data[pos:pos + 1] == b"#":
            while data[pos:pos + 1] not in (b"\n", b""):
                pos += 1
            continue
        start = pos
        while not data[pos:pos + 1].isspace():
            pos += 1
        parts.append(data[start:pos])
    pos += 1  # one whitespace byte after maxval
    if parts[0] != b"P6":
        raise ValueError(f"{path}: not P6")
    w, h = int(parts[1]), int(parts[2])
    return w, h, data[pos:pos + w * h * 3]


def tops(d):
    return sorted(f for f in os.listdir(d) if f.endswith(".ppm") and not f.startswith("bot_"))


def regions(mask, w, h):
    """Sizes of the 8-connected regions of the set pixels (mask: bytearray of w*h)."""
    seen = bytearray(w * h)
    sizes = []
    for start in range(w * h):
        if not mask[start] or seen[start]:
            continue
        stack = [start]
        seen[start] = 1
        n = 0
        while stack:
            p = stack.pop()
            n += 1
            y, x = divmod(p, w)
            for dy in (-1, 0, 1):
                ny = y + dy
                if ny < 0 or ny >= h:
                    continue
                for dx in (-1, 0, 1):
                    nx = x + dx
                    if nx < 0 or nx >= w:
                        continue
                    q = ny * w + nx
                    if mask[q] and not seen[q]:
                        seen[q] = 1
                        stack.append(q)
        sizes.append(n)
    return sizes


def compare(pa, pb, tol, max_region, max_pct):
    wa, ha, da = read_ppm(pa)
    wb, hb, db = read_ppm(pb)
    if (wa, ha) != (wb, hb):
        return False, f"size {wa}x{ha} vs {wb}x{hb}", 0, 0
    w, h = wa, ha
    mask = bytearray(w * h)
    n = 0
    for p in range(w * h):
        i = p * 3
        if abs(da[i] - db[i]) > tol or abs(da[i + 1] - db[i + 1]) > tol or abs(da[i + 2] - db[i + 2]) > tol:
            mask[p] = 1
            n += 1
    sizes = regions(mask, w, h) if n else []
    big = max(sizes) if sizes else 0
    pct = 100.0 * n / (w * h)
    ok = big <= max_region and pct <= max_pct
    note = f"diff px {n} ({pct:.2f} %), regions {len(sizes)}, largest {big}"
    return ok, note, n, big


def main(argv):
    args = list(argv)
    tol, max_region, max_pct = 8, 64, 2.0
    for key in ("--tol", "--max-region", "--max-pct"):
        if key in args:
            i = args.index(key)
            val = args[i + 1]
            del args[i:i + 2]
            if key == "--tol":
                tol = int(val)
            elif key == "--max-region":
                max_region = int(val)
            else:
                max_pct = float(val)
    if len(args) != 2:
        print(__doc__)
        return 2
    a, b = args
    ta, tb = tops(a), tops(b)
    if ta != tb:
        print(f"NO PAIR: names differ {ta} vs {tb}")
        return 2
    if not ta:
        print("NO PAIR: no top shots")
        return 2
    fails = 0
    for name in ta:
        ok, note, _, _ = compare(os.path.join(a, name), os.path.join(b, name), tol, max_region, max_pct)
        print(f"{'PASS' if ok else 'FAIL'} {name}: {note}")
        fails += not ok
    print(f"{len(ta) - fails} of {len(ta)} shots pass (tol {tol}, region max {max_region} px, diff max {max_pct} %)")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
