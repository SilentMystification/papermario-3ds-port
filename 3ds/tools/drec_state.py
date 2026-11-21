#!/usr/bin/env python3
"""drec_state: the render state of the draws in a drec.txt (plan_decal.md section 6, row DEC0). Reads text only.

  python 3ds/tools/drec_state.py census [--all] [PATH | --bench B --row R]
  python 3ds/tools/drec_state.py tdiff [BASE ROW | --bench B --row R]
  python 3ds/tools/drec_state.py selftest

drec.txt (nd_drec.c:31-58): `frame <F>` (pm_gpu.c:661, drec step > 1 only), then per draw `draw <seq> kind K n N`,
`r <reg 3 hex> <mask> <val 8 hex>`, `t <u> type <8 hex> dim <8 hex> bytes <n> h <hash|gpu>`, `v ...`, `i ...`, `m ...`, `end`.
A register missing from a draw counts as not matching (shown as `-` in census).
Registers used: 104 alpha test (bit 0 = on), 101 blend (default 01010000), 107 depth, 04e depth offset (default 0),
083 unit 0 param (bit 1 = mag linear, bits 8-9 wrap t, bits 12-13 wrap s).

census: one line per draw with r 104 bit 0 set, or r 101 not 01010000, or r 04e not 0 (every draw with --all).
  ds <seq> fr <F> at <104> bl <101> dp <107> dm <04e> p0 <083> f <mag> ws <wrap s> wt <wrap t> td <dim> ty <type> th <hash>
  Unit 0 gives td, ty, th. Last line: dstate draws N listed L atest A blend B dmoff D lin X (lin: draws with f 1).
  Second line: dstate missing at A bl B dm D p0 P (draws without that register).
tdiff: base and row blocks pair by index. A different draw count: `tdiff count base N row M`, exit 1.
  A pair with equal lines is same. A pair where only the t lines differ is tex. A tex pair whose base r 104 bit 0 is clear
  and r 101 is 01010000 is tex_opaque. Any other difference is other. Exit 0 only when other 0 and tex_opaque 0.
  The seqs of the first 10 tex_opaque and other pairs are printed.
--bench B --row R: the row file is bench.row_shots_run(B, R) shot_path/drec.txt. The base file is
  bench.store_root()/bench.base_commit(None)/B/drec.txt.
"""
import argparse
import os
import sys
from pathlib import Path

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

R_AT, R_BL, R_DP, R_DM, R_P0 = 0x104, 0x101, 0x107, 0x04E, 0x083
BL_DEFAULT = 0x01010000


def parse(text):
    """The draw blocks of a drec text: list of dicts seq, frame, lines (the block lines), regs (reg -> last value), tex (unit -> (type, dim, hash))."""
    blocks, frame, blk = [], None, None
    for line in text.splitlines():
        if line.startswith("frame "):
            frame = int(line.split()[1])
        elif line.startswith("draw "):
            parts = line.split()
            blk = {"seq": int(parts[1]), "frame": frame, "lines": [line], "regs": {}, "tex": {}}
        elif blk is not None:
            blk["lines"].append(line)
            if line == "end":
                blocks.append(blk)
                blk = None
            elif line.startswith("r "):
                parts = line.split()
                blk["regs"][int(parts[1], 16)] = int(parts[3], 16)
            elif line.startswith("t "):
                parts = line.split()
                blk["tex"][int(parts[1])] = (parts[3], parts[5], parts[9])
    return blocks


def hx(v, w=8):
    return "-" if v is None else ("%0*x" % (w, v))


def census_lines(text, show_all):
    """(lines, counts) of the census of one drec text."""
    out = []
    n = listed = atest = blend = dmoff = lin = miss_at = miss_bl = miss_dm = miss_p0 = 0
    for b in parse(text):
        g = b["regs"]
        at, bl, dp, dm, p0 = g.get(R_AT), g.get(R_BL), g.get(R_DP), g.get(R_DM), g.get(R_P0)
        n += 1
        miss_at += at is None; miss_bl += bl is None; miss_dm += dm is None; miss_p0 += p0 is None
        is_at = at is not None and (at & 1) == 1
        is_bl = bl is not None and bl != BL_DEFAULT
        is_dm = dm is not None and dm != 0
        f = None if p0 is None else (p0 >> 1) & 1
        ws = None if p0 is None else (p0 >> 12) & 3
        wt = None if p0 is None else (p0 >> 8) & 3
        atest += is_at; blend += is_bl; dmoff += is_dm; lin += (f == 1)
        hit = is_at or is_bl or is_dm
        listed += hit
        if hit or show_all:
            ty, td, th = b["tex"].get(0, ("-", "-", "-"))  # unit 0: type, dim, hash
            out.append("ds %d fr %s at %s bl %s dp %s dm %s p0 %s f %s ws %s wt %s td %s ty %s th %s" % (
                b["seq"], "-" if b["frame"] is None else b["frame"], hx(at), hx(bl), hx(dp), hx(dm), hx(p0),
                "-" if f is None else f, "-" if ws is None else ws, "-" if wt is None else wt, td, ty, th))
    out.append("dstate draws %d listed %d atest %d blend %d dmoff %d lin %d" % (n, listed, atest, blend, dmoff, lin))
    out.append("dstate missing at %d bl %d dm %d p0 %d" % (miss_at, miss_bl, miss_dm, miss_p0))
    return out


def tdiff_counts(base_text, row_text):
    """The counts of tdiff: dict with count_ok, draws, same, tex, tex_opaque, other, and the seq lists."""
    bb, rb = parse(base_text), parse(row_text)
    res = {"count_ok": len(bb) == len(rb), "nb": len(bb), "nr": len(rb), "draws": 0, "same": 0, "tex": 0,
           "tex_opaque": 0, "other": 0, "opaque_seqs": [], "other_seqs": []}
    if not res["count_ok"]:
        return res
    for a, b in zip(bb, rb):
        res["draws"] += 1
        if a["lines"] == b["lines"]:
            res["same"] += 1
            continue
        na = [x for x in a["lines"] if not x.startswith("t ")]
        nb = [x for x in b["lines"] if not x.startswith("t ")]
        if na != nb:
            res["other"] += 1
            res["other_seqs"].append(a["seq"])
            continue
        res["tex"] += 1
        at, bl = a["regs"].get(R_AT), a["regs"].get(R_BL)
        if at is not None and (at & 1) == 0 and bl == BL_DEFAULT:
            res["tex_opaque"] += 1
            res["opaque_seqs"].append(a["seq"])
    return res


def tdiff_report(res):
    if not res["count_ok"]:
        print("tdiff count base %d row %d" % (res["nb"], res["nr"]))
        return 1
    print("tdiff draws %d same %d tex %d tex_opaque %d other %d" % (res["draws"], res["same"], res["tex"], res["tex_opaque"], res["other"]))
    print("tex_opaque seqs %s" % (res["opaque_seqs"][:10],))
    print("other seqs %s" % (res["other_seqs"][:10],))
    return 0 if res["other"] == 0 and res["tex_opaque"] == 0 else 1


def bench_files(bench, row):
    """(base path, row path) for --bench and --row."""
    import bench as bm
    rr = bm.row_shots_run(bench, row)
    if rr is None:
        sys.exit("drec_state: no run of %s under row %s with shots" % (bench, row))
    base = bm.store_root() / (bm.base_commit(None) or "none") / bench / "drec.txt"
    return base, Path(rr["shot_path"]) / "drec.txt"


def read(p):
    return open(p, encoding="utf-8", errors="replace").read()


def selftest():
    head = "frame 221\n"
    def block(seq, at, bl, tex_hash):
        return ("draw %d kind 1 n 6\nr 104 ff %08x\nr 101 ff %08x\nt 0 type 00000002 dim 00100020 bytes 512 h %s\n"
                "v bytes 4 h 0000000000000001\ni bytes 2 h 0000000000000002\nend\n") % (seq, at, bl, tex_hash)
    base = head + block(0, 0x3061, 0x01010000, "00000000000000aa") + block(1, 0x0000, 0x01010000, "00000000000000bb")
    t_alpha = head + block(0, 0x3061, 0x01010000, "00000000000000cc") + block(1, 0x0000, 0x01010000, "00000000000000bb")
    r104 = (head + block(0, 0x3061, 0x01010000, "00000000000000aa") + block(1, 0x0000, 0x01010000, "00000000000000bb")).replace("r 104 ff 00000000", "r 104 ff 00000001")
    t_opaque = head + block(0, 0x3061, 0x01010000, "00000000000000aa") + block(1, 0x0000, 0x01010000, "00000000000000dd")
    ok_a = tdiff_counts(base, t_alpha)
    ok_b = tdiff_counts(base, r104)
    ok_c = tdiff_counts(base, t_opaque)
    good = (ok_a["tex"] == 1 and ok_a["tex_opaque"] == 0 and ok_a["other"] == 0 and ok_a["count_ok"]) and \
           (ok_b["other"] == 1 and ok_b["tex_opaque"] == 0) and \
           (ok_c["tex_opaque"] == 1 and ok_c["other"] == 0 and ok_c["tex"] == 1)
    print("drec_state selftest a: tex %d tex_opaque %d other %d; b: other %d; c: tex_opaque %d" % (
        ok_a["tex"], ok_a["tex_opaque"], ok_a["other"], ok_b["other"], ok_c["tex_opaque"]))
    cen = census_lines(base, False)
    good = good and len(cen) == 3 and cen[0].startswith("ds 0 fr 221 at 00003061") and cen[-2].startswith("dstate draws 2 listed 1 ")
    print("drec_state selftest PASS" if good else "drec_state selftest FAIL")
    return 0 if good else 1


def main(argv):
    ap = argparse.ArgumentParser(description="drec.txt render state census and tex diff (DEC0)")
    ap.add_argument("cmd", choices=["census", "tdiff", "selftest"])
    ap.add_argument("paths", nargs="*")
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--bench")
    ap.add_argument("--row")
    a = ap.parse_args(argv)
    if a.cmd == "selftest":
        return selftest()
    if a.cmd == "census":
        if a.bench and a.row:
            path = bench_files(a.bench, a.row)[1]
        elif len(a.paths) == 1:
            path = a.paths[0]
        else:
            ap.error("census: one PATH, or --bench and --row")
        for line in census_lines(read(path), a.all):
            print(line)
        return 0
    if a.bench and a.row:
        base, row = bench_files(a.bench, a.row)
    elif len(a.paths) == 2:
        base, row = a.paths
    else:
        ap.error("tdiff: BASE ROW, or --bench and --row")
    return tdiff_report(tdiff_counts(read(base), read(row)))


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
