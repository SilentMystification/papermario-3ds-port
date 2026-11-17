#!/usr/bin/env python3
"""LOOP2: asset_ids.py <base> <paths out> <now out> <dir>...: the asset manifest of src_sync.sh asset_manifest, in one process.
<paths out>: every regular file (no symlink) under the dirs, relative to <base>, sorted by byte value (LC_ALL=C sort).
<now out>: "path<TAB>id" per file, id = the git blob id of the raw bytes (git hash-object --no-filters): sha1("blob <n>\\0" + bytes).
The same bytes as the old find | sort | git hash-object | paste pipeline (self-test: --check <base> <dir>... compares both).
FACT (LOOP2 probe, 18426 files, 73 MB): the files are hashed by 8 threads; the per-file open cost of Windows dominates."""
import concurrent.futures, hashlib, os, subprocess, sys


def walk(base, dirs):
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
    return sorted(out, key=lambda p: p.encode("utf-8", "surrogateescape"))


def blob_id(path):
    with open(path, "rb") as f:
        b = f.read()
    return hashlib.sha1(b"blob %d\0" % len(b) + b).hexdigest()


def manifest(base, dirs):
    paths = walk(base, dirs)
    with concurrent.futures.ThreadPoolExecutor(8) as ex:
        ids = list(ex.map(lambda p: blob_id(os.path.join(base, p)), paths))
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
    paths, ids = manifest(base, dirs)
    with open(po + ".tmp", "w", encoding="utf-8", newline="\n") as f:
        f.write("".join(p + "\n" for p in paths))
    with open(no + ".tmp", "w", encoding="utf-8", newline="\n") as f:
        f.write("".join("%s\t%s\n" % (p, i) for p, i in zip(paths, ids)))
    os.replace(po + ".tmp", po)
    os.replace(no + ".tmp", no)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
