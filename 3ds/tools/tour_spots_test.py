#!/usr/bin/env python3
"""Host test of tour_spots.py (POS1): for mac_01, kmr_20 and iwa_10 the spot lies on a floor triangle of the map
collision (the highest floor at its x z within 1 unit of its y), no collision or model geometry is above it within
ROOF_H, the 4 neighbours 16 units away are floor at the same height, and it is reachable from entry 0.
Run: python 3ds/tools/tour_spots_test.py   (prints PASS or FAIL lines, exit code 0 on PASS)"""
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tour_spots as ts  # noqa: E402

MAPS = ("mac_01", "kmr_20", "iwa_10")


def check(name):
    errs = []
    r = ts.spot_for_map(name)
    if r["pos"] is None:
        return ["no spot: " + r["reason"]]
    x, y, z = r["pos"]
    srcs = ts.read_sources(ts.map_dir(name))
    shape_n, hit_n = ts.geom_names(name, srcs)
    tris, flags = ts.read_hit(os.path.join(ts.GEOM, hit_n + ".bin"))
    n = ts.tri_normals(tris)
    floors = tris[n[:, 1] > ts.FLOOR_NY]
    fy = ts.floor_at(x, z, y + 1.0, floors)
    if fy is None or abs(fy - y) > 1.0:
        errs.append("not on a floor triangle: y %.1f floor %s" % (y, fy))
    models = ts.read_shape(os.path.join(ts.GEOM, shape_n + ".bin"))
    above = np.concatenate([tris] + models)
    roof = ts.covered_height(np.array([x]), np.array([y]), np.array([z]), above)[0]
    if r["reason"].startswith("outdoor") and roof - y < ts.ROOF_H:
        errs.append("geometry above at %.1f" % roof)
    if not r["reason"].startswith("outdoor"):
        errs.append("not outdoor: " + r["reason"])
    for dx, dz in ((16, 0), (-16, 0), (0, 16), (0, -16)):
        ny = ts.floor_at(x + dx, z + dz, y + ts.STEP_DY, floors)
        if ny is None or abs(ny - y) > ts.STEP_DY:
            errs.append("neighbour %+d %+d not floor (%s)" % (dx, dz, ny))
    if "entry 0 off the floor" in r["reason"]:
        errs.append("not linked to entry 0")
    print("%s: spot %.0f %.1f %.0f score %.2f models %d ents %d roof %s %s" % (
        name, x, y, z, r["score"], r["models"], r["ents"], "none" if roof == np.inf else "%.0f" % (roof - y), r["reason"]))
    return errs


def main():
    bad = 0
    for m in MAPS:
        e = check(m)
        print("%s %s%s" % ("PASS" if not e else "FAIL", m, "" if not e else ": " + "; ".join(e)))
        bad += bool(e)
    print("tour_spots_test: %s" % ("PASS" if not bad else "FAIL (%d maps)" % bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
