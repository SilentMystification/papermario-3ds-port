#!/usr/bin/env python3
"""BASEBIN: content-keyed store entries for bench.py (cache safety rules, docs/handoff/plan_buildspeed.md "Cache safety").
An entry is the folder <root>/<16 hex of sha1(key)>/ with key.txt (the full key text), out.sha1 ("<sha1>  <name>" per stored
file) and the files. get() checks key.txt and every file hash on each use; a failed entry is removed (a miss). put() writes a
temp folder and renames it, so no half entry exists. PM_CACHE=off: get() misses and put() stores nothing.
Host self-test: 3ds/tests/basebin/test_basebin.py (run by 3ds/tests/host.d/basebin.sh)."""
import hashlib
import os
import shutil
import subprocess
from pathlib import Path


def sha1_bytes(b):
    return hashlib.sha1(b).hexdigest()


def sha1_file(p):
    h = hashlib.sha1()
    with open(p, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def cache_off():
    return os.environ.get("PM_CACHE", "on") == "off"


def entry_dir(root, key):
    return Path(root) / sha1_bytes(key.encode("utf-8"))[:16]


def check(d, key):
    """True when the entry folder d holds key and every file matches out.sha1. A failed entry is removed."""
    if not d.is_dir():
        return False
    try:
        ok = (d / "key.txt").read_bytes().decode("utf-8") == key
        lines = [ln for ln in (d / "out.sha1").read_text(encoding="utf-8").splitlines() if ln.strip()]
        ok = ok and bool(lines)
        for ln in lines if ok else []:
            h, name = ln.split("  ", 1)
            if sha1_file(d / name) != h:
                ok = False
                break
    except (OSError, ValueError, UnicodeDecodeError):
        ok = False
    if not ok:
        print("cache %s: entry %s failed its check (key or file hash), removed" % (Path(d).parent.name, d.name))
        shutil.rmtree(d, ignore_errors=True)
    return ok


def get(root, key):
    """The entry folder of key when it passes check(), else None."""
    if cache_off():
        return None
    d = entry_dir(root, key)
    return d if check(d, key) else None


def put(root, key, files, keep):
    """Stores files (dict: name -> source path) under key and keeps the newest `keep` entries. Returns the entry folder, or
    None (PM_CACHE=off, or a source file is missing: nothing stored)."""
    if cache_off():
        return None
    if not all(Path(p).is_file() for p in files.values()):
        print("cache %s: a file to store is missing, nothing stored" % Path(root).name)
        return None
    d = entry_dir(root, key)
    if check(d, key):
        return d
    tmp = d.with_name(d.name + ".tmp.%d" % os.getpid())
    shutil.rmtree(tmp, ignore_errors=True)
    tmp.mkdir(parents=True)
    lines = []
    for name, src in sorted(files.items()):
        shutil.copy2(src, tmp / name)
        lines.append("%s  %s" % (sha1_file(tmp / name), name))
    (tmp / "key.txt").write_bytes(key.encode("utf-8"))
    (tmp / "out.sha1").write_text("\n".join(lines) + "\n", encoding="utf-8")
    try:
        os.rename(tmp, d)
    except OSError:
        shutil.rmtree(tmp, ignore_errors=True)
    trim(root, keep)
    return d if d.is_dir() else None


def trim(root, keep):
    ents = sorted((p for p in Path(root).iterdir() if p.is_dir() and ".tmp." not in p.name), key=lambda p: p.stat().st_mtime, reverse=True)
    for p in ents[keep:]:
        shutil.rmtree(p, ignore_errors=True)


def asset_hash(main):
    """sha1 of the 'path<TAB>blob' lines of every file of assets/us and ver/us/build of the main folder (git hash-object
    --no-filters: content ids, never size or time)."""
    paths = []
    for rel in ("assets/us", "ver/us/build"):
        for root, _dirs, names in os.walk(Path(main) / rel):
            for n in names:
                paths.append((Path(root) / n).relative_to(main).as_posix())
    paths.sort()
    r = subprocess.run(["git", "hash-object", "--no-filters", "--stdin-paths"], cwd=main, input="\n".join(paths) + "\n",
                       capture_output=True, text=True, check=True)
    ids = r.stdout.split()
    if len(ids) != len(paths):
        raise RuntimeError("asset_hash: %d ids for %d files" % (len(ids), len(paths)))
    return sha1_bytes("".join("%s\t%s\n" % (p, i) for p, i in zip(paths, ids)).encode("utf-8"))


def image_id(name):
    """Stable id of a docker image: sha1 of its layer digests. The .Id changes on every docker build -q (BuildKit
    attestation), so a key on .Id would miss after every build.sh run (coordinator fix, 2026-11-16)."""
    r = subprocess.run(["docker", "image", "inspect", "-f", "{{json .RootFS.Layers}}", name], capture_output=True, text=True)
    return hashlib.sha1(r.stdout.strip().encode()).hexdigest() if r.returncode == 0 and r.stdout.strip() else "none"


def build_key(repo, main, commit, build_no, assets):
    """The key of a base build of commit (build.sh with PM_BUILD_REV): every input by content. The tree of the commit (blob
    ids of every tracked file, build.sh and the Dockerfiles too), the commit id and build number (both are written into
    pm_build.h), every asset file, the ROM, both docker image ids, PM_FNCOV."""
    tree = subprocess.run(["git", "ls-tree", "-r", commit], cwd=repo, capture_output=True, check=True).stdout
    return "\n".join([
        "basebin v1",
        "rev %s" % commit,
        "bn %s" % build_no,
        "tree %s" % sha1_bytes(tree),
        "assets %s" % assets,
        "rom %s" % sha1_file(Path(main) / "ver" / "us" / "baserom.z64"),
        "img-build %s" % image_id("pm3ds-build:1"),
        "img-host %s" % image_id("pm3ds-host:1"),
        "fncov %s" % os.environ.get("PM_FNCOV", "0"),
    ])
