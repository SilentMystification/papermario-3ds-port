#!/usr/bin/env python3
"""HOSTSEL: one result cache per host test unit of nd_host_test.sh (the inline block, each host.d script).

Store: <root>/<16 hex of sha1(K0 of the unit)>/<16 hex of sha1(full key)>/ with key.txt, reads.txt, out.txt, out.sha1.
K0 (nd_host_test.sh writes it, the unit name is added here): image, the script, the assets, the trace tool, the path list of
HOST_IN, the gfx table entry. Full key = K0 + "path id" of every file the unit read (3ds/tools/readtrace.c trace, kept only
when the path is in the HOST_IN list; ids from host_in.txt, the in_manifest content ids). Only a passing unit is stored.
Cache safety (docs/handoff/plan_buildspeed.md): every lookup recomputes the full key of each stored read list and needs key.txt
equal and out.sha1 equal to the sha1 of out.txt and reads.txt; a failed entry is removed. A new entry is a temp folder, one rename.
Usage:
  hostsel.py lookup <root> <k0 file> <host_in.txt> <units file> <hits out>   one line "<unit>\\t<entry dir>" per hit
  hostsel.py store <root> <k0 file> <host_in.txt> <list>   list lines: unit, trace log, out file, must read (tab separated)
  hostsel.py selftest                                                        seeded faults (host.d/hostsel.sh)
"""
import hashlib, os, shutil, sys, tempfile, time

KEEP = 4  # read states kept per unit K0


def sha1(b):
    return hashlib.sha1(b).hexdigest()


def load_ids(p):
    ids = {}
    with open(p, encoding="utf-8", newline="") as f:
        for ln in f:
            ln = ln.rstrip("\r\n")
            if "\t" in ln:
                k, v = ln.split("\t", 1)
                ids[k] = v
    return ids


def k0_of(k0, unit, ids):
    """K0 of a unit: the K0 text of nd_host_test.sh, the sha1 of the full path list of the tar (every HOST_IN file: a new, a
    removed or a created once-probed file changes it; this covers each directory listing, glob and failed open of a unit,
    which the trace does not log), and the unit name."""
    return k0.rstrip("\n") + "\npaths " + sha1("\n".join(sorted(ids)).encode("utf-8")) + "\nunit " + unit


def full_key(k0u, reads, ids):
    return k0u + "\nreads\n" + "\n".join("%s\t%s" % (p, ids.get(p, "-")) for p in reads)


def entry_ok(d, key):
    try:
        if open(os.path.join(d, "key.txt"), "rb").read().decode("utf-8") != key:
            return False
        want = open(os.path.join(d, "out.sha1"), encoding="utf-8").read().strip()
        got = sha1(open(os.path.join(d, "out.txt"), "rb").read() + b"\0" + open(os.path.join(d, "reads.txt"), "rb").read())
        return want == got
    except (OSError, UnicodeDecodeError):
        return False


def lookup_unit(root, k0u, ids):
    """The entry dir of a hit, else None. A candidate whose full key names it but fails its check is removed."""
    kd = os.path.join(root, sha1(k0u.encode("utf-8"))[:16])
    if not os.path.isdir(kd):
        return None
    for c in sorted(os.listdir(kd)):
        cd = os.path.join(kd, c)
        if ".tmp." in c or not os.path.isfile(os.path.join(cd, "reads.txt")):
            continue
        try:
            reads = [l for l in open(os.path.join(cd, "reads.txt"), encoding="utf-8").read().split("\n") if l]
        except (OSError, UnicodeDecodeError):
            continue
        key = full_key(k0u, reads, ids)
        d = os.path.join(kd, sha1(key.encode("utf-8"))[:16])
        if not os.path.isdir(d):
            continue
        if entry_ok(d, key):
            return d
        print("hostsel: entry %s failed its check (key or file hash), removed" % d, file=sys.stderr)
        shutil.rmtree(d, ignore_errors=True)
    return None


def log_paths(log):
    """The paths of a trace log (None when the log is missing). A "@<path>" path is a read below the shared /tmp/gconv."""
    try:
        txt = open(log, encoding="utf-8", errors="replace").read()
    except OSError:
        return None
    return set(l.split("\t", 1)[1] for l in txt.split("\n") if "\t" in l)


def store_unit(root, k0u, ids, log, out, must=None, extra=(), prep=None):
    """Stores a passing unit. (None, 0) when its trace is missing or lacks <must> (the unit's own script): a broken trace.
    HSDEP: the reads of the units it needs (extra logs) join its key; a unit (or a needed unit) that opened a file below the
    shared /tmp/gconv ("@" paths) also gets the reads of the prep step that made it (prep log; missing prep log: not stored)."""
    own = log_paths(log)
    if own is None:
        return None, 0
    if must and must not in own:
        return None, 0
    allp = set(own)
    for x in extra:
        xp = log_paths(x)
        if xp is None:
            return None, 0
        allp |= xp
    if any(q.startswith("@") for q in allp):
        pp = log_paths(prep) if prep else None
        if pp is None:
            return None, 0
        allp |= pp
    reads = sorted(allp & set(ids))
    key = full_key(k0u, reads, ids)
    kd = os.path.join(root, sha1(k0u.encode("utf-8"))[:16])
    d = os.path.join(kd, sha1(key.encode("utf-8"))[:16])
    os.makedirs(kd, exist_ok=True)
    t = tempfile.mkdtemp(prefix=".tmp.", dir=kd)
    rd = "\n".join(reads).encode("utf-8")
    ob = open(out, "rb").read()
    open(os.path.join(t, "reads.txt"), "wb").write(rd)
    open(os.path.join(t, "out.txt"), "wb").write(ob)
    open(os.path.join(t, "key.txt"), "wb").write(key.encode("utf-8"))
    open(os.path.join(t, "out.sha1"), "w").write(sha1(ob + b"\0" + rd))
    for attempt in range(8):  # Windows: the rename fails when another run holds d open or stores the same unit at the same time
        shutil.rmtree(d, ignore_errors=True)
        try:
            os.rename(t, d)
            break
        except OSError:
            try:
                same = open(os.path.join(d, "key.txt"), "rb").read() == key.encode("utf-8")
            except OSError:
                same = False
            if same:  # another run stored the same unit with the same key: keep it
                shutil.rmtree(t, ignore_errors=True)
                break
            if attempt == 7:
                raise
            time.sleep(0.25)
    olds = sorted((e for e in os.listdir(kd) if ".tmp." not in e), key=lambda e: os.path.getmtime(os.path.join(kd, e)), reverse=True)
    for e in olds[KEEP:]:
        shutil.rmtree(os.path.join(kd, e), ignore_errors=True)
    return d, len(reads)


def selftest():
    r = tempfile.mkdtemp()
    try:
        ids = {"a.c": "b:1", "b.h": "b:2", "u.c": "b:3"}
        log = os.path.join(r, "log"); out = os.path.join(r, "out")
        open(log, "w").write("cc1\ta.c\nsh\tb.h\nsh\tmade.o\n")
        open(out, "w").write("unit ok\n")
        root = os.path.join(r, "hs"); k0u = k0_of("K v1", "t1", ids)
        look = lambda i: lookup_unit(root, k0_of("K v1", "t1", i), i)
        def chk(c, msg):
            if not c:
                raise SystemExit("hostsel FAIL: " + msg)
        chk(lookup_unit(root, k0u, ids) is None, "hit on an empty store")
        d, n = store_unit(root, k0u, ids, log, out)
        chk(n == 2, "reads %d, want 2 (made.o is no input)" % n)
        chk(lookup_unit(root, k0u, ids) == d, "no hit after store")
        chk(lookup_unit(root, k0_of("K v1", "t2", ids), ids) is None, "another unit hit")
        chk(lookup_unit(root, k0_of("K v2", "t1", ids), ids) is None, "another K0 hit")
        chk(look(dict(ids, **{"u.c": "b:9"})) == d, "an unread file edit missed")
        chk(look(dict(ids, **{"a.c": "b:9"})) is None, "a read file edit hit")
        chk(look({"b.h": "b:2", "u.c": "b:3"}) is None, "a removed read file hit")
        chk(look(dict(ids, **{"n.c": "b:4"})) is None, "a new file in a scanned folder hit")
        chk(look({"a.c": "b:1", "b.h": "b:2"}) is None, "a deleted unread file hit")
        chk(look(dict(ids, **{"inc/probe.h": "b:5"})) is None, "a created probed-missing file hit")
        chk(look(ids) == d, "no hit after the list cases")
        open(os.path.join(d, "out.txt"), "a").write("X")
        chk(lookup_unit(root, k0u, ids) is None, "a changed stored byte hit")
        chk(not os.path.isdir(d), "the failed entry stayed")
        chk(store_unit(root, k0u, ids, log, out, "u.c")[0] is None, "a trace without its must file was stored")
        chk(store_unit(root, k0u, ids, log + ".none", out)[0] is None, "a missing trace was stored")
        # HSDEP: a needed unit's reads and the prep reads (for "@" paths) join the key
        xlog = os.path.join(r, "xlog"); plog = os.path.join(r, "plog"); glog = os.path.join(r, "glog")
        open(xlog, "w").write("cc1\tu.c\n"); open(plog, "w").write("cc1\tp.h\n"); open(glog, "w").write("cc1\ta.c\nsh\t@nd_gfx_tab.o\n")
        ids2 = dict(ids, **{"p.h": "b:7"}); k2 = k0_of("K v1", "t1", ids2)
        d2, n2 = store_unit(root, k2, ids2, log, out, None, [xlog], plog)
        chk(n2 == 3, "needs: reads %d, want 3 (a.c b.h + u.c of the needed unit)" % n2)
        chk(lookup_unit(root, k2, dict(ids2, **{"u.c": "b:9"})) is None, "an edit read only by the needed unit hit")
        k3 = k0_of("K v1", "t3", ids2)
        d3, n3 = store_unit(root, k3, ids2, glog, out, None, [], plog)
        chk(n3 == 2, "prep: reads %d, want 2 (a.c + p.h of the prep)" % n3)
        chk(lookup_unit(root, k3, dict(ids2, **{"p.h": "b:9"})) is None, "an edit read only by the /tmp/gconv prep hit")
        chk(store_unit(root, k3, ids2, glog, out, None, [], plog + ".none")[0] is None, "a unit with @ reads was stored without the prep log")
        chk(store_unit(root, k2, ids2, log, out, None, [xlog + ".none"], plog)[0] is None, "a unit was stored without its needed unit's log")
        d, n = store_unit(root, k0u, ids, log, out)
        open(os.path.join(d, "reads.txt"), "a").write("\nu.c")
        chk(lookup_unit(root, k0u, ids) is None, "a changed read list hit")
    finally:
        shutil.rmtree(r, ignore_errors=True)
    print("hostsel ok (empty, store, hit, unit, K0, unread hit, read miss, removed read, new file, deleted file, probe, byte, must, no trace, needs, prep, read list)")


def main(a):
    if len(a) >= 2 and a[1] == "selftest":
        selftest(); return 0
    if len(a) == 7 and a[1] == "lookup":
        root, k0, ids, units, hits = a[2], open(a[3], encoding="utf-8").read(), load_ids(a[4]), a[5], a[6]
        with open(hits, "w", encoding="utf-8", newline="\n") as f:
            for u in [l.strip() for l in open(units, encoding="utf-8") if l.strip()]:
                d = lookup_unit(root, k0_of(k0, u, ids), ids)
                if d:
                    f.write("%s\t%s\n" % (u, d))
        return 0
    if len(a) in (6, 7) and a[1] == "store":
        k0, ids = open(a[3], encoding="utf-8").read(), load_ids(a[4])
        prep = a[6] if len(a) == 7 else None
        bad = 0
        for l in open(a[5], encoding="utf-8"):
            f = l.rstrip("\r\n").split("\t")
            u, log, out, must = f[:4]
            extra = [x for x in (f[4].split(",") if len(f) > 4 else []) if x]
            d, n = store_unit(a[2], k0_of(k0, u, ids), ids, log, out, must, extra, prep)
            if d is None:
                print("HOSTSEL FAULT: the read trace of %s (or of a unit it needs, or of the /tmp/gconv prep) is missing or lacks %s. Nothing stored for it." % (u, must))
                bad = 1
        return bad
    print(__doc__, file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
