"""SEAMCEN (CRK2): seam census of the real device draws in a drec.txt written with the drecvx switch.

Usage:
  seamcen.py <bench> --row <id>     the newest run of bench under row (its shot folder drec.txt)
  seamcen.py --base <bench>         the stored base drec.txt of bench (bench.py base)
  seamcen.py --file <path>          any drec.txt
  seamcen.py --selftest             the built-in check (exit 0 only when both cases are right)

Prints one line: `seamcen <name> draws A verts B clusters C shared D split E snap_rn F snap_rz G maxd16 H`.
Points: the vertices the ix lines name, per draw (one point per unique index). A point reaches the cluster stage in double
(reference: c = P[4r] + P[4r+1] z + P[4r+2] y + P[4r+3] x for rows 0, 1, 3; points with w < 0.05 skipped). Cluster: a grid cell of
1/64 screen units, joined when |dsx| < 1/256, |dsy| < 1/256 and |dw| < 1e-5 w to a cluster of the 9 cells around.
shared: clusters of 2 or more draws. split: shared clusters with 2 or more member keys (x, y, z and the 16 palette words).
snap_rn, snap_rz: split clusters whose (X16, Y16) differ, with the f24 model of nd_seam_test.c (round to nearest, round to zero).
maxd16: 16 times the largest screen spread (x or y) of a shared cluster, f24 round to nearest.
"""
import math
import os
import struct
import sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))


def f32(x):
    try:
        return struct.unpack("<f", struct.pack("<f", x))[0]
    except OverflowError:
        return math.copysign(math.inf, x)


def bits_of(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]


def from_bits(u):
    return struct.unpack("<f", struct.pack("<I", u & 0xFFFFFFFF))[0]


def rn(x):
    u = bits_of(x)
    if u & 0x7FFFFFFF:
        u = (u + 0x3F + ((u >> 7) & 1)) & 0xFFFFFF80
    return from_bits(u)


def rz(x):
    return from_bits(bits_of(x) & 0xFFFFFF80)


def dp4(row, v, f24):
    acc = 0.0
    for k in range(4):
        p = f24(f32(f24(row[k]) * f24(v[k])))
        acc = f24(f32(acc + p))
    return acc


def model_s(cn, cd, scale):
    # sx = f32(f32(f32(f32(cx / cw) * 0.5) + 0.5) * scale), the f24 model of nd_seam_test.c
    return f32(f32(f32(f32(cn / cd) * 0.5) + 0.5) * scale)


def x16(s):
    return int(f32(f32(s * 16) + (-0.5 if s < 0 else 0.5)))


def parse(lines):
    """Draws of a drec.txt: dict per draw with the palette words (reg -> bits) and the vx and ix bytes."""
    draws = []
    cur = None
    for ln in lines:
        if ln.startswith("draw "):
            cur = {"pal": {}, "vx": None, "ix": None}
            draws.append(cur)
        elif cur is None:
            continue
        elif ln == "end":
            cur = None
        elif ln.startswith("r "):
            w = ln.split(" ")
            reg = int(w[1], 16)
            if 0x1000 <= reg < 0x1200:
                cur["pal"].setdefault((reg - 0x1000) >> 4, {})[reg & 15] = int(w[3], 16)
        elif ln.startswith("vx"):
            cur["vx"] = bytes.fromhex("".join(ln.split(" ")[1:]))
        elif ln.startswith("ix"):
            cur["ix"] = bytes.fromhex("".join(ln.split(" ")[1:]))
    return draws


def draw_points(d):
    """{unique index: (x, y, z, k)} of one draw, or None when the draw has no palette words or no vx or ix line."""
    if not d["pal"] or d["vx"] is None or d["ix"] is None:
        return None
    ix = d["ix"]
    idx = struct.unpack("<%dH" % (len(ix) // 2), ix[: len(ix) // 2 * 2])
    vx = d["vx"]
    pts = {}
    for i in set(idx):
        o = 16 * i
        if o + 8 > len(vx):
            continue
        x, y, z, k = struct.unpack("<4h", vx[o : o + 8])
        if k in d["pal"] and len(d["pal"][k]) == 16:
            pts[i] = (x, y, z, k)
    return pts


def census(lines):
    """Counts of one drec.txt (see the module doc)."""
    draws = parse(lines)
    cells = defaultdict(list)
    clusters = []
    used_draws = 0
    verts = 0
    for di, d in enumerate(draws):
        pts = draw_points(d)
        if not pts:
            continue
        used_draws += 1
        pal_f = {}
        pal_b = {}
        for k, p in d["pal"].items():
            if len(p) == 16:
                pal_b[k] = tuple(p[j] for j in range(16))
                pal_f[k] = [from_bits(p[j]) for j in range(16)]
        for i in sorted(pts):
            x, y, z, k = pts[i]
            P = pal_f[k]
            cx = P[0] + P[1] * z + P[2] * y + P[3] * x
            cy = P[4] + P[5] * z + P[6] * y + P[7] * x
            cw = P[12] + P[13] * z + P[14] * y + P[15] * x
            if cw < 0.05:
                continue
            sx = (cx / cw * 0.5 + 0.5) * 240.0
            sy = (cy / cw * 0.5 + 0.5) * 400.0
            key = (x, y, z, pal_b[k])
            member = {"draw": di, "key": key, "rn": None, "rz": None}
            v = (1.0, float(z), float(y), float(x))
            for mode, f24 in (("rn", rn), ("rz", rz)):
                rows = [[f24(P[4 * r + j]) for j in range(4)] for r in range(4)]
                mx = dp4(rows[0], v, f24)
                my = dp4(rows[1], v, f24)
                mw = dp4(rows[3], v, f24)
                if mw != 0.0:
                    msx = model_s(mx, mw, 240.0)
                    msy = model_s(my, mw, 400.0)
                    member[mode] = (msx, msy, x16(msx), x16(msy))
            cxk, cyk = math.floor(sx * 64), math.floor(sy * 64)
            found = None
            for ddx in (-1, 0, 1):
                for ddy in (-1, 0, 1):
                    for cid in cells.get((cxk + ddx, cyk + ddy), ()):
                        c = clusters[cid]
                        if abs(sx - c["sx"]) < 1 / 256 and abs(sy - c["sy"]) < 1 / 256 and abs(cw - c["w"]) < 1e-5 * cw:
                            found = cid
                            break
                    if found is not None:
                        break
                if found is not None:
                    break
            if found is None:
                found = len(clusters)
                clusters.append({"sx": sx, "sy": sy, "w": cw, "mem": []})
                cells[(cxk, cyk)].append(found)
            clusters[found]["mem"].append(member)
            verts += 1
    shared = [c for c in clusters if len({m["draw"] for m in c["mem"]}) >= 2]
    split = [c for c in shared if len({m["key"] for m in c["mem"]}) >= 2]
    snap = {}
    for mode in ("rn", "rz"):
        snap[mode] = sum(1 for c in split if len({(m[mode][2], m[mode][3]) for m in c["mem"] if m[mode]}) >= 2)
    maxd = 0.0
    for c in shared:
        ms = [m["rn"] for m in c["mem"] if m["rn"]]
        if ms:
            spread = max(max(m[0] for m in ms) - min(m[0] for m in ms), max(m[1] for m in ms) - min(m[1] for m in ms))
            maxd = max(maxd, spread * 16.0)
    return {
        "draws": used_draws,
        "verts": verts,
        "clusters": len(clusters),
        "shared": len(shared),
        "split": len(split),
        "snap_rn": snap["rn"],
        "snap_rz": snap["rz"],
        "maxd16": maxd,
    }


def line_of(name, c):
    return "seamcen %s draws %d verts %d clusters %d shared %d split %d snap_rn %d snap_rz %d maxd16 %.3f" % (
        name, c["draws"], c["verts"], c["clusters"], c["shared"], c["split"], c["snap_rn"], c["snap_rz"], c["maxd16"])


def hexbytes(b):
    h = b.hex()
    return " ".join(h[i : i + 32] for i in range(0, len(h), 32))


def draw_text(seq, rows, verts, idx):
    """A draw block in the drec.txt format: palette 0 words, the vx and ix lines (no m lines are read)."""
    words = [w for r in rows for w in r]
    out = ["draw %d kind 1 n %d" % (seq, len(idx))]
    for j, w in enumerate(words):
        out.append("r %03x f %08x" % (0x1000 + j, bits_of(float(w))))
    vb = b"".join(struct.pack("<4h", x, y, z, k) + b"\x00" * 8 for (x, y, z, k) in verts)
    out.append("vx " + hexbytes(vb))
    out.append("ix " + hexbytes(b"".join(struct.pack("<H", i) for i in idx)))
    out.append("end")
    return out


def selftest():
    rows = [[0, 0, 0, 1], [0, 0, 1, 0], [0, 1, 0, 0], [300, 0.5, 0, 0]]
    verts = [(10, 20, 30, 0), (40, -20, 30, 0), (-30, 5, 60, 0)]
    idx = [0, 1, 2]
    rows_b = [[100, 0, 0, 1], [0, 0, 1, 0], [0, 1, 0, 0], [300, 0.5, 0, 0]]
    verts_b = [(x - 100, y, z, k) for (x, y, z, k) in verts]
    a = census(draw_text(0, rows, verts, idx) + draw_text(1, rows, verts, idx))
    b = census(draw_text(0, rows, verts, idx) + draw_text(1, rows_b, verts_b, idx))
    ok_a = a["shared"] == 3 and a["split"] == 0
    ok_b = b["shared"] == 3 and b["split"] == 3
    print(line_of("selftest A", a))
    print(line_of("selftest B", b))
    if ok_a and ok_b:
        print("seamcen selftest ok")
        return 0
    print("seamcen selftest FAIL")
    return 1


def main(argv):
    if "--selftest" in argv:
        return selftest()
    if "--file" in argv:
        path, name = Path(argv[argv.index("--file") + 1]), "file"
    elif "--base" in argv:
        import bench  # noqa: E402

        name = argv[argv.index("--base") + 1]
        commit = bench.base_commit(None)
        if commit is None:
            print("seamcen: no merge-base with main")
            return 2
        path = bench.store_root() / commit / name / "drec.txt"
    elif argv and not argv[0].startswith("-") and "--row" in argv:
        import bench  # noqa: E402

        name = argv[0]
        row = argv[argv.index("--row") + 1]
        rr = bench.row_shots_run(name, row)
        if rr is None:
            print("seamcen: no run of %s under row %s with shots" % (name, row))
            return 2
        path = Path(rr["shot_path"]) / "drec.txt"
    else:
        print("usage: seamcen.py <bench> --row <id> | --base <bench> | --file <path> | --selftest")
        return 2
    if not path.is_file():
        print("seamcen: no drec.txt at %s" % path)
        return 2
    lines = path.read_text(encoding="utf-8").splitlines()
    print(line_of(name, census(lines)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
