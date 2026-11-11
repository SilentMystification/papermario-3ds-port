#!/usr/bin/env python3
"""hotset.py: hot code bytes of the core 0 draw path (plan_hotcode.md stage HC0, measurement only).

usage (repo root; Windows: py -3 3ds/tools/hotset.py ...):
  hotset.py                  full report of the lane: M1 per class and union, M2 linker hot group, call sites of the union
  hotset.py --line           one line (build.sh prints it after the memcheck line)
  hotset.py --fncov <log>    M3: bytes of the functions that ran in each census frame, per class (log of an fncov_* scenario)
Lane: PM_LANE (folder build3ds/lane-<L>/), else build3ds/. The census lane is <L>_fncov (PM_FNCOV=1 build.sh); M3 reads its map.
Sizes come from the map (.text.<fn> input sections, -ffunction-sections). Call edges come from objdump -d of pm_3ds.elf, run in
the devkitARM image pm3ds-build:1 through docker and cached in <lane>/hotset/objdump.txt (rerun when the ELF size or mtime changes).
M1: membership of a class = its roots (hotset.json) plus their callees to "depth" inside the draw objects (a static estimate).
M2: the names of the hot group in 3ds/ld/3dsx.ld.in, summed from the map; a name the map does not have is stale.
M3: the census counts (log lines "fncov f<n> <addr>:<calls> ...") mapped to names by the census map, sized by the normal map.
Stdlib only. Metrics are printed as "hotset M1 ...", "hotset M2 ...", "hotset M3 ..." lines (bench.py log_metrics reads them).
"""
import argparse, fnmatch, json, os, re, subprocess, sys
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
CFG = json.load(open(HERE / "hotset.json", encoding="utf-8"))
IMG = "pm3ds-build:1"
CLASSES = sorted(CFG["classes"])
DRAW = [re.compile(p) for p in CFG["draw_objs"]]
EXT = set(CFG["ext_allow"])
CALLS = {k: [re.compile(p) for p in v] for k, v in CFG["calls"].items()}
LD = REPO / "3ds" / "ld" / "3dsx.ld.in"
HOT_LD = REPO / "3ds" / "ld" / "hot_order.ld"


def lane_dir(lane, fncov=False):
    if not lane:
        return REPO / "build3ds"
    return REPO / "build3ds" / ("lane-%s%s" % (lane, "_fncov" if fncov else ""))


def strip_sec(sec):
    return re.sub(r"^\.text\.(unlikely\.)?", "", sec)


def _add(out, a, sz, obj, sec):
    size = int(sz, 16)
    addr = int(a, 16)
    if size and addr not in out:
        out[addr] = {"name": strip_sec(sec), "size": size, "obj": obj, "cold": sec.startswith(".text.unlikely.")}


def parse_map(path):
    """-> {addr: {name, size, obj}} for the .text.* input sections of the .text output section."""
    out, pend, in_text = {}, None, False
    with open(path, encoding="utf-8", errors="replace") as fh:
        for raw in fh:
            l = raw.rstrip("\r\n")
            if l and not l[0].isspace():  # an output section line starts in column 0
                in_text = bool(re.match(r"^\.text(\s|$)", l))
                pend = None
                continue
            s = l.strip()
            if not in_text or not s:
                continue
            if pend is not None:
                m = re.match(r"^(0x[0-9a-f]+)\s+(0x[0-9a-f]+)\s+(\S+)$", s)
                if m:
                    _add(out, m.group(1), m.group(2), m.group(3), pend)
                pend = None
                continue
            m = re.match(r"^(\.text\.\S+)(?:\s+(0x[0-9a-f]+)\s+(0x[0-9a-f]+)\s+(\S+))?$", s)
            if m:
                if m.group(2):
                    _add(out, m.group(2), m.group(3), m.group(4), m.group(1))
                else:
                    pend = m.group(1)
    return out


def objdump_text(lane):
    """objdump -d of the lane ELF through docker, cached in <lane>/hotset/."""
    base = lane_dir(lane)
    elf = base / "pm_3ds.elf"
    if not elf.exists():
        raise SystemExit("hotset: no ELF at %s (build the lane first)" % elf)
    st = elf.stat()
    key = "%d %d" % (st.st_size, int(st.st_mtime))
    d = base / "hotset"
    d.mkdir(parents=True, exist_ok=True)
    cache, kf = d / "objdump.txt", d / "objdump.key"
    if cache.exists() and kf.exists() and kf.read_text().strip() == key:
        return cache.read_text(encoding="utf-8", errors="replace")
    root = str(REPO).replace("\\", "/")
    rel = elf.relative_to(REPO).as_posix()
    sh = 'export PATH=/opt/devkitpro/devkitARM/bin:$PATH; arm-none-eabi-objdump -d --no-show-raw-insn "%s"' % rel
    env = dict(os.environ, MSYS_NO_PATHCONV="1")
    p = subprocess.run(["docker", "run", "--rm", "-v", root + ":/w", "-w", "/w", IMG, "sh", "-c", sh],
                       capture_output=True, env=env)
    if p.returncode:
        raise SystemExit("hotset: objdump failed in %s (exit %d): %s" % (IMG, p.returncode, p.stderr[-300:]))
    text = p.stdout.decode("utf-8", errors="replace")
    cache.write_text(text, encoding="utf-8")
    kf.write_text(key)
    return text


HDR = re.compile(r"^([0-9a-f]+) <([^>]+)>:$")
INS = re.compile(r"^\s*([0-9a-f]+):\t(\S+)\s*(.*)$")
TGT = re.compile(r"^([0-9a-f]+) <([^>]+)>")
BR = re.compile(r"^b(l|lx)?(eq|ne|cs|hs|cc|lo|mi|pl|vs|vc|hi|ls|ge|lt|gt|le)?$")


def parse_calls(text):
    """-> {caller addr: [(callee addr, callee name)]} for the branch instructions of each function. A jump inside a
    function has a '+offset' target and is skipped."""
    calls, cur = {}, None
    for l in text.split("\n"):
        m = HDR.match(l)
        if m:
            cur = int(m.group(1), 16)
            calls.setdefault(cur, [])
            continue
        if cur is None:
            continue
        m = INS.match(l)
        if not m or not BR.match(m.group(2)):
            continue
        t = TGT.match(m.group(3))
        if t and "+" not in t.group(2):
            calls[cur].append((int(t.group(1), 16), t.group(2)))
    return calls


def model(lane):
    fns = parse_map(lane_dir(lane) / "pm_3ds.map")
    if not fns:
        raise SystemExit("hotset: no .text input sections in the map of lane %r" % lane)
    calls = parse_calls(objdump_text(lane))
    names = {}
    for a, e in fns.items():
        names.setdefault(e["name"], []).append(a)
    cg = {a: set(t for t, _ in calls.get(a, ()) if t in fns) for a in fns}
    return fns, names, calls, cg


def allowed(a, fns):
    e = fns[a]
    return e["name"] in EXT or any(r.search(e["obj"]) for r in DRAW)


def closure(roots, fns, cg, depth):
    mem, front = set(roots), set(roots)
    for _ in range(depth):
        nxt = set()
        for a in front:
            for c in cg.get(a, ()):
                if c not in mem and allowed(c, fns):
                    nxt.add(c)
        mem |= nxt
        front = nxt
    return mem


def classes(fns, names, cg):
    mem, missing = {}, []
    for c in CLASSES:
        roots = []
        for n in CFG["classes"][c]["roots"]:
            if n in names:
                roots += names[n]
            else:
                missing.append(n)
        mem[c] = closure(roots, fns, cg, CFG["depth"])
    return mem, missing


def nbytes(fns, s):
    return sum(fns[a]["size"] for a in s)


def linker_group():
    """The names of the hot group in 3ds/ld/3dsx.ld.in: the lines of its INCLUDE of hot_order.ld (HC2, generated), else the
    '*(.text.' lines up to '*(.text.unlikely.*)'."""
    toks, on = [], False
    for l in open(LD, encoding="utf-8", errors="replace"):
        s = l.strip()
        if s.startswith("*(.text.unlikely") and on:
            break
        if s.startswith("INCLUDE") and "hot_order.ld" in s:
            for h in open(HOT_LD, encoding="utf-8", errors="replace"):
                toks += re.findall(r"\.text\.([A-Za-z0-9_.*]+)", h.split("/*")[0])
            break
        if s.startswith("*(.text.") or s.startswith("*libcitro3d"):
            on = True
            toks += re.findall(r"\.text\.([A-Za-z0-9_.*]+)", s)
    seen = []
    for t in toks:
        if t not in seen:
            seen.append(t)
    return seen


def m2(fns, names, toks):
    hit, stale = set(), []
    for t in toks:
        if "*" in t:
            got = [a for a, e in fns.items() if fnmatch.fnmatchcase(e["name"], t)]
        else:
            got = names.get(t, [])
        got = [a for a in got if not fns[a]["cold"]]  # a .text.unlikely section is not in the hot group
        if not got:
            stale.append(t)
        hit |= set(got)
    return hit, stale


def cat_of(name):
    for k, rs in CALLS.items():
        if any(r.search(name) for r in rs):
            return k
    return None


def kb(b):
    return "%.1f KB" % (b / 1024.0)


def run_report(lane, line):
    fns, names, calls, cg = model(lane)
    mem, missing = classes(fns, names, cg)
    union = set().union(*mem.values())
    hit, stale = m2(fns, names, linker_group())
    cb = {c: nbytes(fns, mem[c]) for c in CLASSES}
    ch = {c: nbytes(fns, set(a for a in mem[c] if not fns[a]["cold"])) for c in CLASSES}  # TOOLHOT: per class without .text.unlikely
    ub = nbytes(fns, union)
    hot = set(a for a in union if not fns[a]["cold"])  # M1 hot: the union without .text.unlikely (cold split, HC1)
    hb = nbytes(fns, hot)
    if line:
        print("hotset: M1 union %s, M1 hot %s (%s) | M2 linker %s, stale %d (%s)" % (
            kb(ub), kb(hb), " ".join("%s %.1f" % (c, cb[c] / 1024.0) for c in CLASSES),
            kb(nbytes(fns, hit)), len(stale), ", ".join(stale) or "none"))
        return 0
    print("hotset lane %s, map %s, %d functions in the map" % (lane or "main", lane_dir(lane) / "pm_3ds.map", len(fns)))
    for c in CLASSES:
        print("hotset M1 class %s %d B (%d fns) %s" % (c, cb[c], len(mem[c]), CFG["classes"][c]["name"]))
    for c in CLASSES:
        print("hotset M1hot class %s %d B %s" % (c, ch[c], CFG["classes"][c]["name"]))
    print("hotset M1 union %d B (%d fns, depth %d)" % (ub, len(union), CFG["depth"]))
    print("hotset M1 hot %d B (%d fns, union without .text.unlikely)" % (hb, len(hot)))
    print("hotset M2 linker %d B (%d fns)" % (nbytes(fns, hit), len(hit)))
    print("hotset M2 stale %d names: %s" % (len(stale), " ".join(stale)))
    if missing:
        print("hotset M1 roots missing from the map: %s" % " ".join(missing))
    if stale:
        print("hotset FAIL: %d hot group names are not in the map (regenerate ld/hot_order.ld or fix 3dsx.ld.in)" % len(stale))
        return 1
    tot = Counter()
    for a in sorted(union, key=lambda x: -fns[x]["size"]):
        cnt = Counter()
        for _, n in calls.get(a, ()):
            k = cat_of(n)
            if k:
                cnt[k] += 1
        if cnt:
            tot += cnt
            print("hotcall %s %d B %s" % (fns[a]["name"], fns[a]["size"],
                                         " ".join("%s %d" % (k, cnt[k]) for k in CALLS if cnt[k])))
    print("hotset calls union %s" % " ".join("%s %d" % (k, tot[k]) for k in CALLS))
    return 0


def fncov_report(log, lane, fn=False):
    fmap = parse_map(lane_dir(lane, True) / "pm_3ds.map")  # census lane: addr -> name (its sizes include the hooks)
    fns, names, calls, cg = model(lane)                       # normal lane: sizes and class membership by name
    mem, _ = classes(fns, names, cg)
    size = {}
    for a, e in fns.items():
        size.setdefault(e["name"], e["size"])
    cls = {}
    for c in CLASSES:
        for a in mem[c]:
            cls.setdefault(fns[a]["name"], set()).add(c)
    frames, done = {}, set()
    pat_c = re.compile(r"fncov f(\d+) ((?:[0-9a-f]{8}:\d+ ?)+)")
    pat_d = re.compile(r"fncov f(\d+) done")
    with open(log, encoding="utf-8", errors="replace") as fh:
        for l in fh:
            m = pat_d.search(l)
            if m:
                done.add(int(m.group(1)))
                continue
            m = pat_c.search(l)
            if m:
                cnt = frames.setdefault(int(m.group(1)), Counter())
                for tok in m.group(2).split():
                    a, n = tok.split(":")
                    cnt[int(a, 16)] += int(n)
    if not done:
        print("hotset M3: no 'fncov f<n> done' line in %s (the census did not finish)" % log)
        return 1
    for f in sorted(done):
        cnt = frames.get(f, Counter())
        run_b, unm, calls_n, outside = 0, 0, 0, 0
        cb, cc = Counter(), Counter()
        for a, n in cnt.items():
            calls_n += n
            e = fmap.get(a)
            if not e or e["name"] not in size:
                unm += 1
                continue
            b = size[e["name"]]
            run_b += b
            if e["name"] in cls:
                for c in cls[e["name"]]:
                    cb[c] += b; cc[c] += n
            else:
                outside += b
        print("hotset M3 f%d run %d B (%d fns, %d calls, unmatched %d)" % (f, run_b, len(cnt), calls_n, unm))
        print("hotset M3 f%d class %s" % (f, " ".join("%s %d" % (c, cb[c]) for c in CLASSES)))
        print("hotset M3 f%d calls %s" % (f, " ".join("%s %d" % (c, cc[c]) for c in CLASSES)))
        print("hotset M3 f%d outside classes %d B" % (f, outside))
        if fn:  # --fncovfn: calls per function of the frame (census addresses summed by name), calls >= 20, largest first
            fc = Counter()
            for a, n in cnt.items():
                e = fmap.get(a)
                fc[e["name"] if e else "0x%08x" % a] += n
            for name, n in sorted(fc.items(), key=lambda t: -t[1]):
                if n >= 20:
                    print("hotset fn %s %d" % (name, n))
    return 0


def main():
    ap = argparse.ArgumentParser(description="hot code bytes of the core 0 draw path (plan_hotcode HC0)")
    ap.add_argument("--line", action="store_true", help="one line for build.sh")
    ap.add_argument("--fncov", metavar="LOG", help="M3 from a census log")
    ap.add_argument("--fncovfn", action="store_true", help="with --fncov: calls per function of each census frame")
    ap.add_argument("--lane", default=os.environ.get("PM_LANE", ""), help="lane name (default PM_LANE)")
    a = ap.parse_args()
    if a.fncov:
        return fncov_report(a.fncov, a.lane, a.fncovfn)
    return run_report(a.lane, a.line)


if __name__ == "__main__":
    sys.exit(main())
