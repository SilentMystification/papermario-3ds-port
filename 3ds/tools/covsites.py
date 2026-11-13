#!/usr/bin/env python3
"""covsites.py: site coverage of game files from a census log (fncov lane, COVMAP).

usage (repo root; Windows: py -3 3ds/tools/covsites.py ...):
  covsites.py <fncov log> --lane <L> <src file>...

<L> is the lane name of the normal build (PM_LANE); the census map is read from build3ds/lane-<L>_fncov/pm_3ds.map.
Calls per address = sum over all "fncov f<n> <addr>:<calls>" tokens of the log (all frames).

A site is a line of an active branch (listcen.py TARGET_3DS rule) that matches
  \\bg(DP|SP)\\w+\\(|\\bnd_(rec_dl|scomp_rec|draw_list|val_\\w+|xf_\\w+)\\(
A function is a column-0 definition, its body runs to the first column-0 "}". A function is hit when its map entry
has calls > HIT_MIN (the sites of a hit function count as covered). Only functions with sites enter the funcs count.
Output:
  cov <file> sites <hit>/<total> funcs <hit>/<n>
  cov miss <file> <func> sites <k>        (function with sites and 0 calls)
"""
import argparse, re, sys
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import hotset as H  # noqa: E402
import listcen as L  # noqa: E402

SITE_RX = re.compile(r"\bg(DP|SP)\w+\(|\bnd_(rec_dl|scomp_rec|draw_list|val_\w+|xf_\w+)\(")
FN_RX = re.compile(r"^[A-Za-z_][^(;=]*?\b(\w+)\s*\(")
HIT_MIN = 0  # seed check (COVMAP gate 3) sets this to 1000000 for one run, then back to 0


def census_calls(log):
    tot, seen = Counter(), False
    with open(log, encoding="utf-8", errors="replace") as fh:
        for l in fh:
            m = re.search(r"fncov f(\d+) ((?:[0-9a-f]{8}:\d+ ?)+)", l)
            if m:
                seen = True
                for tok in m.group(2).split():
                    a, n = tok.split(":")
                    tot[int(a, 16) & ~1] += int(n)
    if not seen:
        raise SystemExit("covsites: no fncov frame in %s" % log)
    return tot


def functions(path):
    """-> [(name, sites, active_sites)] for the column-0 definitions of one file."""
    text = L.read_stripped(path)
    flags = L.line_flags(text)
    lines = text.split("\n")
    out = []
    i = 0
    while i < len(lines):
        ln = lines[i]
        m = FN_RX.match(ln)
        if m and ln and not ln[0].isspace() and not ln.rstrip().endswith(";") and not ln.startswith("#") and ln[0] != "}":
            name = m.group(1)
            j = i
            while j < len(lines) and not (j > i and lines[j].startswith("}")):
                j += 1
            sites = 0
            for k in range(i, min(j + 1, len(lines))):
                if flags[k] is True and SITE_RX.search(lines[k]):
                    sites += 1
            out.append((name, sites))
            i = j + 1
            continue
        i += 1
    return out


def map_addrs(mp, name, stem):
    return [a for a, e in mp.items() if (e["name"] == name or e["name"].endswith("_" + name)) and stem in e["obj"]]


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("log")
    ap.add_argument("--lane", required=True)
    ap.add_argument("files", nargs="+")
    a = ap.parse_args(argv)
    calls = census_calls(a.log)
    mp = H.parse_map(H.lane_dir(a.lane, True) / "pm_3ds.map")
    for f in a.files:
        stem = Path(f).stem
        hit_sites = tot_sites = hit_funcs = n_funcs = 0
        misses = []
        for name, sites in functions(H.REPO / f):
            if sites == 0:
                continue
            cands = map_addrs(mp, name, stem)
            c = max((calls.get(x, 0) for x in cands), default=0)
            tot_sites += sites
            n_funcs += 1
            if c > HIT_MIN:
                hit_sites += sites
                hit_funcs += 1
            else:
                misses.append((name, sites))
        print("cov %s sites %d/%d funcs %d/%d" % (f, hit_sites, tot_sites, hit_funcs, n_funcs))
        for name, sites in misses:
            print("cov miss %s %s sites %d" % (f, name, sites))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
