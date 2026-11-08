#!/usr/bin/env python3
"""tour_spots.py: one tour spot per map (POS1, plan_tours.md "Spots").

For every map of gAreas (src/world/world.c, no tst_ maps) the tool reads the map collision
(assets/us/mapfs/geom/<map>_hit.bin) and shape (<map>_shape.bin), and writes one line
`map:entry@x:y:z` to 3ds/tours/spots.txt (the tour list format of pm_warp.c tour_step).
A map without a spot gets `map:0` (entry 0 position). The score and the reason per map go to
3ds/tours/spots_report.txt (the tour parser reads spaces as separators, so spots.txt holds only spots).

Method:
- Candidates: points of a GRID-unit grid on the floor triangles (normal y > 0.5, not
  water/lava/spikes, not IGNORE_PLAYER), one per floor layer. Grid neighbours link when their
  floors differ by at most LINK_DY; only the floor linked to the point nearest entry 0 is used.
  (Triangle centroids miss the inside of large floor triangles and need not lie on the floor
  Mario can reach, so the grid replaces them.)
- Standable: the 4 grid neighbours have floor within STEP_DY of the candidate height, and no
  wall triangle cuts the segment to them at knee and head height.
- Camera: the camera zone under the spot (hit file zone section, CameraControlSettings) gives the view
  direction (fixed orientation A->B, look at point, between points) and the boom; no zone: toward -z.
  A zone that does not keep Mario in frame (fixed position, frozen line) rejects the spot.
- Visible: no model triangle cuts the segments from Mario (12 and 30 units up) to the camera eye;
  a spot behind a foreground tree or bush is rejected.
- Outdoors: a vertical ray up from the candidate meets no collision or model triangle between
  ROOF_GAP and ROOF_H units above it (inside a building or under a roof is rejected).
- Score: content = models (box centre) plus ENT_W times entities/NPCs (map source positions) inside
  the view box of the zone camera around the candidate; score = content / the best content of the map
  minus DIST_W times the distance to the floor centre over the floor radius. Best candidate wins.
- A map with floor but no outdoor candidate takes the best standable candidate with ROOF_MIN
  clearance (reason "indoor"). No floor or no standable candidate: entry 0 (reason given).

y is the floor height at the candidate; the tour also snaps y at run time (TOUR1).

Usage: python 3ds/tools/tour_spots.py [--maps m1,m2] [--out 3ds/tours/spots.txt]
"""
import argparse
import os
import re
import struct
import sys

import numpy as np

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
GEOM = os.path.join(ROOT, "assets", "us", "mapfs", "geom")
WORLD = os.path.join(ROOT, "src", "world")
SHAPE_BASE = 0x80210000

FLOOR_NY = 0.5
BAD_SURFACE = {1, 2, 3}          # water, spikes, lava (enums.h SURFACE_TYPE_*)
IGNORE_PLAYER = 0x00010000
ROOF_H = 600.0                   # any geometry above within this height: covered
ROOF_GAP = 40.0                  # geometry closer above the floor is ground detail, not a roof
ROOF_MIN = 80.0                  # indoor fallback: clearance above Mario
GRID = 16.0                      # floor sample grid (candidates and reachability)
LINK_DY = 12.0                   # neighbour grid floors this close in height link (the 10-unit steps of the game)
ENTRY_R = 120.0                  # entry 0 links to the floor when a grid point is this close
STEP_DY = 6.0                    # the 4 grid neighbours of a spot are this flat
# Default camera view box around Mario (camera on +z looking to -z, boom about 450, pitch about 15 deg).
VIEW_X = 220.0
VIEW_ZB = 380.0                  # toward the back (-z)
VIEW_ZF = 120.0                  # toward the camera (+z)
VIEW_YD = 80.0
VIEW_YU = 300.0
CAM_UP_BASE = 20.0               # camera target height over Mario's feet (the eye is boom length back at boom pitch)
DIST_W = 0.5                     # score = content / best content - DIST_W * distance to the floor centre / floor radius
ENT_W = 2.0


# ---------------------------------------------------------------- map list and source data

def map_list():
    src = open(os.path.join(WORLD, "world.c"), encoding="utf-8").read()
    names = re.findall(r"MAP(?:_WITH_INIT)?\(([a-z0-9_]+)\)", src)
    return [n for n in names if n != "map" and not n.startswith("tst_")]


def map_dir(name):
    for area in sorted(os.listdir(WORLD)):
        d = os.path.join(WORLD, area, name)
        if area.startswith("area_") and os.path.isdir(d):
            return d
    return None


def read_sources(d):
    out = []
    if d is None:
        return out
    for dp, _, fs in os.walk(d):
        for f in fs:
            if f.endswith((".c", ".h")):
                out.append(open(os.path.join(dp, f), encoding="utf-8", errors="replace").read())
    return out


NUM = r"(-?[\d.]+)f?"


def entry0(srcs):
    for s in srcs:
        m = re.search(r"EntryList\s+N\(Entrances\)\s*=\s*\{(.*?)\};", s, re.S)
        if m:
            e = re.search(r"\{\s*" + NUM + r"\s*,\s*" + NUM + r"\s*,\s*" + NUM, m.group(1))
            if e:
                return tuple(float(v) for v in e.groups())
    return None


def geom_names(name, srcs):
    shape, hit = name + "_shape", name + "_hit"
    for s in srcs:
        m = re.search(r'sprintf\(wMapShapeName,\s*"([a-z0-9_]+)"', s)
        if m:
            shape = m.group(1)
        m = re.search(r'sprintf\(wMapHitName,\s*"([a-z0-9_]+)"', s)
        if m:
            hit = m.group(1)
    return shape, hit


def content_points(srcs):
    """Entity (MakeEntity) and NPC (.pos) positions from the map source."""
    pts = []
    for s in srcs:
        for m in re.finditer(r"MakeEntity,\s*Ref\(Entity_\w+\)\s*,\s*" + NUM + r"\s*,\s*" + NUM + r"\s*,\s*" + NUM, s):
            pts.append(tuple(float(v) for v in m.groups()))
        for m in re.finditer(r"\.pos\s*=\s*\{\s*" + NUM + r"\s*,\s*" + NUM + r"\s*,\s*" + NUM, s):
            pts.append(tuple(float(v) for v in m.groups()))
    return np.array(pts, dtype=np.float64).reshape(-1, 3)


# ---------------------------------------------------------------- hit file (collision.c load_hit_data)

def read_hit(path):
    """Returns (tris Nx3x3 float, flags N int) of the collider triangles, or None."""
    b = open(path, "rb").read()
    coll_off, _ = struct.unpack_from(">II", b, 0)
    if coll_off == 0:
        return None
    nc, c_off, nv, v_off, nbb, bb_off = struct.unpack_from(">h2xih2xih2xi", b, coll_off)
    verts = np.array(struct.unpack_from(">%dh" % (nv * 3), b, v_off), dtype=np.float64).reshape(nv, 3)
    tris, flags = [], []
    for i in range(nc):
        bbo, _, _, nt, t_off = struct.unpack_from(">hhhhi", b, c_off + i * 12)
        if nt <= 0:
            continue
        fl = 0
        if bbo >= 0:
            fl = struct.unpack_from(">i", b, bb_off + bbo * 4 + 0x18)[0]
        for j in range(nt):
            p = struct.unpack_from(">I", b, t_off + j * 4)[0]
            tris.append((p & 0x3FF, (p >> 10) & 0x3FF, (p >> 20) & 0x3FF))
            flags.append(fl)
    if not tris:
        return None
    idx = np.array(tris, dtype=np.int64)
    return verts[idx], np.array(flags, dtype=np.int64)


def read_zones(path):
    """Camera zones of the hit file (zone section): list of (settings, tris). For a zone the bounding box offset points
    at its CameraControlSettings (common_structs.h): type, boomLength, boomPitch, points A B (6 floats), viewPitch, flag."""
    b = open(path, "rb").read()
    zo = struct.unpack_from(">I", b, 4)[0]
    if zo == 0:
        return []
    nc, c_off, nv, v_off, nbb, bb_off = struct.unpack_from(">h2xih2xih2xi", b, zo)
    verts = np.array(struct.unpack_from(">%dh" % (nv * 3), b, v_off), dtype=np.float64).reshape(nv, 3)
    out = []
    for i in range(nc):
        bbo, _, _, nt, t_off = struct.unpack_from(">hhhhi", b, c_off + i * 12)
        if nt <= 0 or bbo < 0:
            continue
        cs = struct.unpack_from(">i2f6ffi", b, bb_off + bbo * 4)
        idx = [(p & 0x3FF, (p >> 10) & 0x3FF, (p >> 20) & 0x3FF) for p in struct.unpack_from(">%dI" % nt, b, t_off)]
        out.append((cs, verts[np.array(idx, dtype=np.int64)]))
    return out


def camera_at(p, zones):
    """Camera of the zone under p (highest zone floor at or below p y + 50, as test_ray_zones casts down):
    (forward unit vector in x z, boom length, boom pitch in degrees, follows). Follows is False for the zone types
    that do not keep Mario in frame (fixed position, frozen line)."""
    best = None
    for cs, tr in zones:
        y = floor_at(p[0], p[2], p[1] + 50.0, tr)
        if y is not None and (best is None or y > best[0]):
            best = (y, cs)
    fwd = np.array([0.0, -1.0])
    if best is None:
        return fwd, 450.0, 15.0, True
    typ, boom, pitch, ax, ay, az, bx, by, bz, vpitch, flag = best[1]
    follows = True
    if typ == 0:                      # CAM_CONTROL_FIXED_ORIENTATION: yaw along A -> B
        d = np.array([bx - ax, bz - az])
        if np.hypot(*d) > 1e-3:
            fwd = d / np.hypot(*d)
        follows = flag == 0
    elif typ == 1:                    # LOOK_AT_POINT: toward point A (away with a negative boom)
        d = np.array([ax - p[0], az - p[2]]) * (1.0 if boom >= 0 else -1.0)
        if np.hypot(*d) > 1e-3:
            fwd = d / np.hypot(*d)
    elif typ in (2, 4, 5):            # constrained to a line or fixed: Mario need not be in frame
        follows = typ != 4 and flag == 0
    elif typ == 6:                    # between points: yaw perpendicular to A B, facing -z side
        d = np.array([bz - az, -(bx - ax)])
        if np.hypot(*d) > 1e-3:
            fwd = d / np.hypot(*d)
            if fwd[1] > 0:
                fwd = -fwd
        follows = flag == 0
    return fwd, abs(boom) if boom else 450.0, pitch, follows


def tri_normals(t):
    """Normals as load_hit_data computes them: e13 x e21, normalised."""
    e13 = t[:, 2] - t[:, 0]
    e21 = t[:, 0] - t[:, 1]
    n = np.cross(e13, e21)
    ln = np.linalg.norm(n, axis=1)
    ln[ln == 0] = 1.0
    return n / ln[:, None]


# ---------------------------------------------------------------- shape file (model tree, F3DEX2 display lists)

def read_shape(path):
    """Returns a list of models as Nx3x3 triangle arrays (world space, group matrices applied)."""
    b = open(path, "rb").read()

    def u32(a):
        return struct.unpack_from(">I", b, a - SHAPE_BASE)[0]

    def ok(a):
        return SHAPE_BASE <= a < SHAPE_BASE + len(b) - 3

    def mtx(a):
        hi = struct.unpack_from(">16h", b, a - SHAPE_BASE)
        lo = struct.unpack_from(">16H", b, a - SHAPE_BASE + 32)
        return np.array([hi[i] + lo[i] / 65536.0 for i in range(16)]).reshape(4, 4)

    def dl_tris(dl):
        vb = [None] * 64
        out = []
        stack, a, guard = [], dl, 0
        while ok(a) and guard < 200000:
            guard += 1
            w0, w1 = struct.unpack_from(">II", b, a - SHAPE_BASE)
            op = w0 >> 24
            a += 8
            if op == 0x01:  # G_VTX
                n = (w0 >> 12) & 0xFF
                v0 = ((w0 >> 1) & 0x7F) - n
                for k in range(n):
                    va = w1 + k * 16
                    if ok(va) and 0 <= v0 + k < 64:
                        vb[v0 + k] = struct.unpack_from(">3h", b, va - SHAPE_BASE)
            elif op in (0x05, 0x06, 0x07):  # G_TRI1, G_TRI2, G_QUAD
                sets = [((w0 >> 16) & 0xFF, (w0 >> 8) & 0xFF, w0 & 0xFF)]
                if op == 0x06:
                    sets.append(((w1 >> 16) & 0xFF, (w1 >> 8) & 0xFF, w1 & 0xFF))
                elif op == 0x07:
                    sets = [((w0 >> 16) & 0xFF, (w0 >> 8) & 0xFF, w0 & 0xFF),
                            ((w0 >> 16) & 0xFF, w0 & 0xFF, (w1) & 0xFF)]
                for s in sets:
                    vs = [vb[i // 2] if i // 2 < 64 else None for i in s]
                    if all(v is not None for v in vs):
                        out.append(vs)
            elif op == 0xDE:  # G_DL
                if ((w0 >> 16) & 0xFF) == 0:
                    stack.append(a)
                a = w1
            elif op == 0xDF:  # G_ENDDL
                if not stack:
                    break
                a = stack.pop()
        return out

    models = []

    def walk(node, m, depth):
        if not ok(node) or depth > 64:
            return
        typ, disp, nprop, props, group = struct.unpack_from(">iIiII", b, node - SHAPE_BASE)
        if typ == 2:
            if ok(disp) and ok(u32(disp)):
                t = dl_tris(u32(disp))
                if t:
                    arr = np.array(t, dtype=np.float64)
                    if m is not None:
                        h = np.concatenate([arr, np.ones(arr.shape[:2] + (1,))], axis=2)
                        arr = (h @ m)[:, :, :3]
                    models.append(arr)
        if ok(group):
            tm, _, _, nch, chl = struct.unpack_from(">IIiiI", b, group - SHAPE_BASE)
            mm = m
            if ok(tm):
                g = mtx(tm)
                mm = g if m is None else g @ m
            if ok(chl):
                for i in range(max(0, min(nch, 4096))):
                    walk(u32(chl + i * 4), mm, depth + 1)

    walk(u32(SHAPE_BASE), None, 0)
    return models


# ---------------------------------------------------------------- geometry helpers

def covered_height(px, py, pz, tris):
    """For each point, the lowest y of triangles (any facing) above py + ROOF_GAP at its x z (inf if none).
    Ground decals and path models sit up to about 30 units over the collision floor; Mario is about 38 tall."""
    if len(tris) == 0:
        return np.full(len(px), np.inf)
    a, b_, c = tris[:, 0], tris[:, 1], tris[:, 2]
    x0, z0 = a[:, 0], a[:, 2]
    x1, z1 = b_[:, 0] - x0, b_[:, 2] - z0
    x2, z2 = c[:, 0] - x0, c[:, 2] - z0
    det = x1 * z2 - x2 * z1
    good = np.abs(det) > 1e-6
    det = np.where(good, det, 1.0)
    out = np.full(len(px), np.inf)
    for i in range(len(px)):
        dx, dz = px[i] - x0, pz[i] - z0
        u = (dx * z2 - x2 * dz) / det
        v = (x1 * dz - dx * z1) / det
        inside = good & (u >= -1e-4) & (v >= -1e-4) & (u + v <= 1.0001)
        if not inside.any():
            continue
        y = a[inside, 1] + u[inside] * (b_[inside, 1] - a[inside, 1]) + v[inside] * (c[inside, 1] - a[inside, 1])
        y = y[y > py[i] + ROOF_GAP]
        if len(y):
            out[i] = y.min()
    return out


def floor_at(x, z, ytop, floors, index=False):
    """Highest floor triangle y at x z not above ytop (None if none); with index, its triangle index."""
    a, b_, c = floors[:, 0], floors[:, 1], floors[:, 2]
    x1, z1 = b_[:, 0] - a[:, 0], b_[:, 2] - a[:, 2]
    x2, z2 = c[:, 0] - a[:, 0], c[:, 2] - a[:, 2]
    det = x1 * z2 - x2 * z1
    good = np.abs(det) > 1e-6
    det = np.where(good, det, 1.0)
    dx, dz = x - a[:, 0], z - a[:, 2]
    u = (dx * z2 - x2 * dz) / det
    v = (x1 * dz - dx * z1) / det
    inside = good & (u >= -1e-4) & (v >= -1e-4) & (u + v <= 1.0001)
    if not inside.any():
        return None
    y = a[inside, 1] + u[inside] * (b_[inside, 1] - a[inside, 1]) + v[inside] * (c[inside, 1] - a[inside, 1])
    ids = np.nonzero(inside)[0][y <= ytop]
    y = y[y <= ytop]
    if not len(y):
        return None
    return int(ids[y.argmax()]) if index else float(y.max())


def seg_hits(p, q, tris):
    """True when segment p->q crosses any triangle (Moller-Trumbore, both faces)."""
    if len(tris) == 0:
        return False
    d = q - p
    e1 = tris[:, 1] - tris[:, 0]
    e2 = tris[:, 2] - tris[:, 0]
    h = np.cross(d, e2)
    a = np.einsum("ij,ij->i", e1, h)
    good = np.abs(a) > 1e-9
    f = np.where(good, 1.0 / np.where(good, a, 1.0), 0.0)
    s = p - tris[:, 0]
    u = f * np.einsum("ij,ij->i", s, h)
    qv = np.cross(s, e1)
    v = f * (qv @ d)
    t = f * np.einsum("ij,ij->i", e2, qv)
    return bool((good & (u >= 0) & (v >= 0) & (u + v <= 1) & (t > 0) & (t < 1)).any())


def floor_grid(floors):
    """Floor points on a GRID-unit grid: {(ix, iz): [y, ...]}, one y per floor layer of a cell (layers closer than 1 unit merge)."""
    cells = {}
    for t in floors:
        a, b_, c = t
        ix = np.arange(np.ceil(t[:, 0].min() / GRID), np.floor(t[:, 0].max() / GRID) + 1)
        iz = np.arange(np.ceil(t[:, 2].min() / GRID), np.floor(t[:, 2].max() / GRID) + 1)
        if not len(ix) or not len(iz):
            continue
        e1x, e1z, e2x, e2z = b_[0] - a[0], b_[2] - a[2], c[0] - a[0], c[2] - a[2]
        det = e1x * e2z - e2x * e1z
        if abs(det) < 1e-6:
            continue
        gx, gz = np.meshgrid(ix, iz)
        gx, gz = gx.ravel(), gz.ravel()
        dx, dz = gx * GRID - a[0], gz * GRID - a[2]
        u = (dx * e2z - e2x * dz) / det
        v = (e1x * dz - dx * e1z) / det
        ins = (u >= -1e-4) & (v >= -1e-4) & (u + v <= 1.0001)
        yy = a[1] + u * (b_[1] - a[1]) + v * (c[1] - a[1])
        for i in np.nonzero(ins)[0]:
            lay = cells.setdefault((int(gx[i]), int(gz[i])), [])
            y = float(yy[i])
            if all(abs(y - l) > 1.0 for l in lay):
                lay.append(y)
    return cells


def grid_components(cells):
    """Nodes (N x 3: ix, iz, y) and a component label per node. Neighbour cells (4-way) link when their floors
    differ by at most LINK_DY (the 10-unit steps of the game link, ledges do not)."""
    nodes, key = [], {}
    for (ix, iz), lay in cells.items():
        for k, y in enumerate(lay):
            key[(ix, iz, k)] = len(nodes)
            nodes.append((ix, iz, y))
    par = list(range(len(nodes)))

    def find(i):
        while par[i] != i:
            par[i] = par[par[i]]
            i = par[i]
        return i
    for (ix, iz, k), i in key.items():
        y = nodes[i][2]
        for dx, dz in ((1, 0), (0, 1)):
            lay = cells.get((ix + dx, iz + dz))
            if not lay:
                continue
            for k2, y2 in enumerate(lay):
                if abs(y2 - y) <= LINK_DY:
                    a, b_ = find(i), find(key[(ix + dx, iz + dz, k2)])
                    if a != b_:
                        par[a] = b_
    comp = np.array([find(i) for i in range(len(nodes))], dtype=np.int64)
    return np.array(nodes, dtype=np.float64).reshape(-1, 3), comp


# ---------------------------------------------------------------- per map

def spot_for_map(name):
    """Returns dict: name, entry, pos (x,y,z) or None, score, reason."""
    d = map_dir(name)
    srcs = read_sources(d)
    e0 = entry0(srcs)
    shape_n, hit_n = geom_names(name, srcs)
    res = {"name": name, "entry": 0, "pos": None, "score": 0.0, "reason": "", "e0": e0}
    hp = os.path.join(GEOM, hit_n + ".bin")
    sp = os.path.join(GEOM, shape_n + ".bin")
    if not os.path.exists(hp):
        res["reason"] = "no hit file " + hit_n
        return res
    hit = read_hit(hp)
    if hit is None:
        res["reason"] = "no collision"
        return res
    tris, flags = hit
    solid = (flags & IGNORE_PLAYER) == 0
    tris, flags = tris[solid], flags[solid]
    n = tri_normals(tris)
    isfloor = (n[:, 1] > FLOOR_NY) & ~np.isin(flags & 0xFF, list(BAD_SURFACE))
    floors = tris[isfloor]
    walls = tris[np.abs(n[:, 1]) <= FLOOR_NY]
    if len(floors) == 0:
        res["reason"] = "no floor"
        return res
    cells = floor_grid(floors)
    if not cells:
        res["reason"] = "floor smaller than the grid"
        return res
    nodes, comp = grid_components(cells)
    # the floor reachable from entry 0: the component of the grid point nearest entry 0 (at or below entry y + 30)
    reach_note = ""
    sel = np.ones(len(nodes), bool)
    if e0 is not None:
        dh = np.hypot(nodes[:, 0] * GRID - e0[0], nodes[:, 1] * GRID - e0[2])
        dh = np.where(nodes[:, 2] <= e0[1] + 30.0, dh, np.inf) + 0.01 * np.abs(nodes[:, 2] - e0[1])
        j = int(dh.argmin())
        if dh[j] <= ENTRY_R:
            sel = comp == comp[j]
        else:
            reach_note = " (entry 0 off the floor: all floor)"
    else:
        reach_note = " (no entry list: all floor)"
    idx = np.nonzero(sel)[0]
    P = np.stack([nodes[idx, 0] * GRID, nodes[idx, 2], nodes[idx, 1] * GRID], axis=1)
    fc = P.mean(axis=0)
    radius = max(float(np.hypot(P[:, 0] - fc[0], P[:, 2] - fc[2]).max()), 1.0)

    models = read_shape(sp) if os.path.exists(sp) else []
    mtris = np.concatenate(models) if models else np.zeros((0, 3, 3))
    above = np.concatenate([tris, mtris]) if len(mtris) else tris
    if models:
        mc = np.array([(m.reshape(-1, 3).min(axis=0) + m.reshape(-1, 3).max(axis=0)) / 2 for m in models])
    else:
        mc = np.zeros((0, 3))
    ents = content_points(srcs)

    zones = read_zones(hp)
    cams = {}

    def cam(k):
        if k not in cams:
            cams[k] = camera_at(P[k], zones)
        return cams[k]

    def view_count(p, pts, fwd):
        """Points inside the view box of the camera looking along fwd (x z) at p."""
        if len(pts) == 0:
            return 0
        dx, dy, dz = pts[:, 0] - p[0], pts[:, 1] - p[1], pts[:, 2] - p[2]
        f = dx * fwd[0] + dz * fwd[1]
        r = dx * fwd[1] - dz * fwd[0]
        return int(((np.abs(r) <= VIEW_X) & (f <= VIEW_ZB) & (f >= -VIEW_ZF) & (dy >= -VIEW_YD) & (dy <= VIEW_YU)).sum())

    def standable(i):
        ix, iz, y = nodes[i]
        for dx, dz in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            lay = cells.get((int(ix) + dx, int(iz) + dz), [])
            if not any(abs(y2 - y) <= STEP_DY for y2 in lay):
                return False
            for h in (10.0, 30.0):
                p = np.array([ix * GRID, y + h, iz * GRID])
                if seg_hits(p, p + np.array([dx * GRID, 0.0, dz * GRID]), walls):
                    return False
        return True

    content = np.array([view_count(P[k], mc, cam(k)[0]) + ENT_W * view_count(P[k], ents, cam(k)[0]) for k in range(len(P))], dtype=np.float64)
    dist = np.hypot(P[:, 0] - fc[0], P[:, 2] - fc[2]) / radius
    sc_all = content / max(content.max(), 1.0) - DIST_W * dist
    scored = [(float(sc_all[k]), k) for k in range(len(idx))]
    scored.sort(key=lambda t: -t[0])

    def visible(k):
        """The zone camera keeps Mario in frame, and no model triangle cuts the segments from Mario (12 and 30 units up)
        to the camera eye (foreground trees and bushes)."""
        fwd, boom, pitch, follows = cam(k)
        if not follows:
            return False
        p = P[k]
        a = np.radians(pitch)
        eye = p + np.array([-fwd[0] * boom * np.cos(a), CAM_UP_BASE + boom * np.sin(a), -fwd[1] * boom * np.cos(a)])
        return not any(seg_hits(p + np.array([0.0, h, 0.0]), eye, mtris) for h in (12.0, 30.0))

    memo = {}

    def check(k, what):
        key = (k, what)
        if key not in memo:
            p = P[k]
            if what == "roof":
                memo[key] = covered_height(p[0:1], p[1:2], p[2:3], above)[0] - p[1]
            elif what == "stand":
                memo[key] = standable(idx[k])
            else:
                memo[key] = visible(k)
        return memo[key]

    def pick(clear, need_vis):
        for sc, k in scored:
            if check(k, "roof") >= clear and check(k, "stand") and (not need_vis or check(k, "vis")):
                return sc, k, check(k, "roof")
        return None

    # Order: outdoors and seen by the camera, then indoors and seen, then the same without the camera test.
    r = None
    for clear, need_vis, why in ((ROOF_H, True, "outdoor"), (ROOF_MIN, True, "indoor (no spot without geometry above)"),
                                 (ROOF_H, False, "outdoor, camera view blocked or not following"),
                                 (ROOF_MIN, False, "indoor, camera view blocked or not following")):
        r = pick(clear, need_vis)
        if r is not None:
            reason = why
            break
    if r is None:
        res["reason"] = "no standable spot"
        return res
    sc, k, roof = r
    p = P[k]
    res.update(pos=(float(p[0]), float(p[1]), float(p[2])), score=float(sc), reason=reason + reach_note,
               roof=float(roof), models=view_count(p, mc, cam(k)[0]), ents=view_count(p, ents, cam(k)[0]), cam=cam(k))
    return res


def fmt(v):
    return ("%.1f" % v).rstrip("0").rstrip(".") if v != int(v) else "%d" % int(v)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--maps", help="comma list (default: every non-tst map of world.c)")
    ap.add_argument("--out", default=os.path.join(ROOT, "3ds", "tours", "spots.txt"))
    ap.add_argument("--report", default=None)
    a = ap.parse_args()
    names = a.maps.split(",") if a.maps else map_list()
    rep = a.report or os.path.join(os.path.dirname(a.out), "spots_report.txt")
    os.makedirs(os.path.dirname(a.out), exist_ok=True)
    lines, rlines = [], ["# map entry x y z score models ents roof reason (tour_spots.py)"]
    for nm in names:
        r = spot_for_map(nm)
        if r["pos"] is not None:
            x, y, z = r["pos"]
            lines.append("%s:0@%s:%s:%s" % (nm, fmt(round(x)), fmt(round(y, 1)), fmt(round(z))))
            rlines.append("%s 0 %d %.1f %d %.2f %d %d %s %s" % (nm, round(x), y, round(z), r["score"], r["models"], r["ents"],
                                                              "inf" if r["roof"] == np.inf else "%d" % r["roof"], r["reason"].replace(" ", "_")))
        else:
            lines.append("%s:0" % nm)
            rlines.append("%s 0 - - - - - - - entry0:%s" % (nm, r["reason"].replace(" ", "_")))
        print(rlines[-1], flush=True)
    with open(a.out, "w", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    with open(rep, "w", newline="\n") as f:
        f.write("\n".join(rlines) + "\n")
    print("tour_spots: %d maps, %d spots, %d entry 0 -> %s" % (len(names), sum(":0@" in l for l in lines),
                                                              sum(":0@" not in l for l in lines), a.out))


if __name__ == "__main__":
    sys.exit(main())
