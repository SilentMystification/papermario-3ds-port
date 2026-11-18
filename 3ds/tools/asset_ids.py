#!/usr/bin/env python3
"""LOOP2: asset_ids.py <base> <paths out> <now out> <dir>...: the asset manifest of src_sync.sh asset_manifest, in one process.
<paths out>: every regular file (no symlink) under the dirs, relative to <base>, sorted by byte value (LC_ALL=C sort).
<now out>: "path<TAB>id" per file, id = the git blob id of the raw bytes (git hash-object --no-filters): sha1("blob <n>\\0" + bytes).
The same bytes as the old find | sort | git hash-object | paste pipeline (self-test: --check <base> <dir>... compares both).
FACT (LOOP2 probe, 18426 files, 73 MB): the files are hashed by 8 threads; the per-file open cost of Windows dominates.
BUILDO: env PM_ASSET_STAT=<file> is a stat cache, the rule of the git index: a file whose size and mtime equal its cache line
reuses the stored id, unless its mtime is within 2 s of the scan that wrote the line (racy: hashed again). The ids stay content
ids; only the reread is skipped. PM_CACHE=off hashes every file and writes no cache."""
import concurrent.futures, hashlib, os, subprocess, sys, time

RACY_NS = 2 * 10**9


def walk(base, dirs, st=None):
    out = []
    for d in dirs:
        stack = [d]
        while stack:
            cur = stack.pop()
            with os.scandir(os.path.join(base, cur)) as it:
                for e in it:
                    rel = cur + "/" + e.name
                    if e.is_dir(follow_symlinks=False):
                        stack.append(rel)
                    elif e.is_file(follow_symlinks=False):
                        out.append(rel)
                        if st is not None:
                            x = e.stat(follow_symlinks=False)
                            st[rel] = (x.st_size, x.st_mtime_ns)
    return sorted(out, key=lambda p: p.encode("utf-8", "surrogateescape"))


def blob_id(path):
    with open(path, "rb") as f:
        b = f.read()
    return hashlib.sha1(b"blob %d\0" % len(b) + b).hexdigest()


def stat_load(f):
    """(scan start ns, {path: (size, mtime_ns, id)}) of a stat cache file; empty when missing or of another version."""
    try:
        with open(f, encoding="utf-8") as h:
            head = h.readline().split()
            if len(head) != 3 or head[:2] != ["astat", "v1"]:
                return 0, {}
            c = {}
            for ln in h:
                p, sz, mt, i = ln.rstrip("\n").split("\t")
                c[p] = (int(sz), int(mt), i)
            return int(head[2]), c
    except (OSError, ValueError):
        return 0, {}


def manifest(base, dirs, cache=None):
    t0 = time.time_ns()
    st = {} if cache else None
    paths = walk(base, dirs, st)
    ids = [None] * len(paths)
    if cache:
        scan, old = stat_load(cache)
        for k, p in enumerate(paths):
            o = old.get(p)
            if o and (o[0], o[1]) == st[p] and o[1] < scan - RACY_NS:
                ids[k] = o[2]
    todo = [k for k in range(len(paths)) if ids[k] is None]
    with concurrent.futures.ThreadPoolExecutor(8) as ex:
        for k, i in zip(todo, ex.map(lambda k: blob_id(os.path.join(base, paths[k])), todo)):
            ids[k] = i
    if cache and todo:
        tmp = "%s.tmp.%d" % (cache, os.getpid())
        with open(tmp, "w", encoding="utf-8", newline="\n") as h:
            h.write("astat v1 %d\n" % t0)
            h.write("".join("%s\t%d\t%d\t%s\n" % (p, st[p][0], st[p][1], i) for p, i in zip(paths, ids)))
        os.replace(tmp, cache)
    return paths, ids


def main(a):
    if len(a) >= 3 and a[1] == "--check":
        base, dirs = a[2], a[3:]
        paths, ids = manifest(base, dirs)
        r = subprocess.run(["sh", "-c", "find %s -type f | LC_ALL=C sort" % " ".join(dirs)], cwd=base, capture_output=True)
        old = r.stdout.decode("utf-8", "surrogateescape").split("\n")[:-1]
        g = subprocess.run(["git", "hash-object", "--no-filters", "--stdin-paths"], cwd=base, input=r.stdout, capture_output=True)
        gids = g.stdout.decode().split()
        ok = old == paths and gids == ids
        print("asset_ids check %s (%d files)" % ("ok" if ok else "FAIL", len(paths)))
        return 0 if ok else 1
    if len(a) < 5:
        print(__doc__, file=sys.stderr)
        return 2
    base, po, no, dirs = a[1], a[2], a[3], a[4:]
    cache = os.environ.get("PM_ASSET_STAT") if os.environ.get("PM_CACHE", "on") != "off" else None
    paths, ids = manifest(base, dirs, cache)
    with open(po + ".tmp", "w", encoding="utf-8", newline="\n") as f:
        f.write("".join(p + "\n" for p in paths))
    with open(no + ".tmp", "w", encoding="utf-8", newline="\n") as f:
        f.write("".join("%s\t%s\n" % (p, i) for p, i in zip(paths, ids)))
    os.replace(po + ".tmp", po)
    os.replace(no + ".tmp", no)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
