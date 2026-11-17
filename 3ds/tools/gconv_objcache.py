#!/usr/bin/env python3
"""GFXINC: the object cache of the gfx table tool inputs (3ds/tools/gen_gfx_tab.sh). Runs in the host image pm3ds-host:1.
Usage: gconv_objcache.py --flags "<gcc flags>" <src.c>...   Each object is written next to its source (<src>.o).
Cache folder: env GCONV_OBJ (the docker volume pm3ds-gconvobj-<host image id>). Unset, missing, or PM_CACHE=off: every source
compiles and nothing is stored. PM_CACHE=check: every source compiles and its object is compared with the entry (cachecheck).
Cache safety (docs/handoff/plan_buildspeed.md "Cache safety"):
- the primary key is the text: tool version, gcc and as versions, the flags, the source path and the sha1 of the source;
- GCONVSPLIT (v2): the folder <GCONV_OBJ>/<16 hex of sha1(primary key)>/ holds up to CAND entries, one per dep state:
  <16 hex of sha1(deps.sha1)>/ with key.txt, deps.sha1 (the sha1 of every file the compile read: gcc -MD gives the #include
  files, as --MD the .incbin files), out.sha1 and obj.o. A header edit adds an entry and keeps the others (no evict between
  lanes with different headers);
- every entry that a lookup reads is checked: key.txt other than the key, or obj.o other than out.sha1, removes the entry;
- a hit needs one entry whose dep files all equal their sha1; else the source compiles; a new entry is a temp folder renamed
  in one step; v1 entries (key.txt at the top) are removed at the start.
Prints "gconvobj: hit H miss M" on stderr; check mode also "cachecheck gconvobj ok N" or "CACHECHECK FAIL gconvobj <src>".
Exit 1 when a compile fails or a cachecheck differs. Host self-test: 3ds/tests/host.d/gfxinc.sh."""
import concurrent.futures
import hashlib
import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

VERSION = "gconv-obj v2"
KEEP = int(os.environ.get("GCONV_OBJ_KEEP", "3000"))
CAND = int(os.environ.get("GCONV_OBJ_CAND", "4"))  # GCONVSPLIT: entries (dep states) kept per primary key
_memo = {}


def sha1_file(p):
    h = hashlib.sha1()
    with open(p, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def dep_hash(p):
    """sha1 of a dep file, read once per run ("missing" when it is gone)."""
    if p not in _memo:
        try:
            _memo[p] = sha1_file(p)
        except OSError:
            _memo[p] = "missing"
    return _memo[p]


def tool_id():
    out = []
    for cmd in (["gcc", "--version"], ["as", "--version"]):
        r = subprocess.run(cmd, capture_output=True, text=True)
        out.append(r.stdout.splitlines()[0] if r.stdout else "none")
    return "tools " + " | ".join(out)


def primary(src, flags, tool):
    return "\n".join([VERSION, tool, "flags " + flags, "src " + src, "content " + sha1_file(src)])


def entry_dir(root, key):
    return Path(root) / hashlib.sha1(key.encode("utf-8")).hexdigest()[:16]


def cand_state(c, key):
    """GCONVSPLIT: "key" (key.txt differs), "out" (obj.o differs from out.sha1), "hit" (every dep file equals its sha1) or
    "other" (another dep state)."""
    try:
        if (c / "key.txt").read_bytes().decode("utf-8") != key:
            return "key"
        if (c / "out.sha1").read_text(encoding="utf-8").strip() != sha1_file(c / "obj.o"):
            return "out"
        for ln in (c / "deps.sha1").read_text(encoding="utf-8").splitlines():
            h, path = ln.split("  ", 1)
            if dep_hash(path) != h:
                return "other"
        return "hit"
    except (OSError, ValueError, UnicodeDecodeError):
        return "key"


def cands(d):
    """The entries of a primary folder, newest first (no temp folder)."""
    try:
        cs = [c for c in d.iterdir() if c.is_dir() and ".tmp." not in c.name]
    except OSError:
        return []
    cs.sort(key=mtime, reverse=True)
    return cs


def mtime(c):
    try:
        return c.stat().st_mtime
    except OSError:
        return 0


def lookup(root, key, src):
    """(hit entry or None, entries of other dep states, entries removed for a changed object). Every entry read is checked."""
    d = entry_dir(root, key)
    hit, other, outbad = None, 0, 0
    for c in cands(d):
        st = cand_state(c, key)
        if st in ("key", "out"):
            print("gconvobj: entry %s/%s of %s failed its check (%s), removed" % (d.name, c.name, src, "key" if st == "key" else "object hash"), file=sys.stderr)
            shutil.rmtree(c, ignore_errors=True)
            outbad += st == "out"
        elif st == "hit" and hit is None:
            hit = c
        else:
            other += 1
    return hit, other, outbad


def parse_deps(text):
    deps = []
    for line in text.replace("\\\n", " ").splitlines():
        if ":" in line:
            deps += line.split(":", 1)[1].split()
    return [p for p in deps if not p.startswith("/tmp/cc")]


def compile_one(src, flags):
    obj = src[:-2] + ".o"
    d, ad = obj + ".d", obj + ".ad"
    cmd = ["gcc"] + shlex.split(flags) + ["-MD", "-MF", d, "-Wa,--MD," + ad, "-c", "-o", obj, src]
    r = subprocess.run(cmd)
    deps = []
    if r.returncode == 0:
        for f in (d, ad):
            deps += parse_deps(Path(f).read_text(encoding="utf-8", errors="replace"))
    for f in (d, ad):
        try:
            os.remove(f)
        except OSError:
            pass
    return r.returncode, sorted(set(deps))


def store(root, key, obj, deps, tag):
    d = entry_dir(root, key)
    dtext = "".join("%s  %s\n" % (dep_hash(p), p) for p in deps)
    c = d / hashlib.sha1(dtext.encode("utf-8")).hexdigest()[:16]
    tmp = d / (c.name + ".tmp.%d.%d" % (os.getpid(), tag))
    shutil.rmtree(tmp, ignore_errors=True)
    tmp.mkdir(parents=True)
    shutil.copyfile(obj, tmp / "obj.o")
    (tmp / "key.txt").write_bytes(key.encode("utf-8"))
    (tmp / "deps.sha1").write_text(dtext, encoding="utf-8")
    (tmp / "out.sha1").write_text(sha1_file(tmp / "obj.o") + "\n", encoding="utf-8")
    shutil.rmtree(c, ignore_errors=True)
    try:
        os.rename(tmp, c)
    except OSError:
        shutil.rmtree(tmp, ignore_errors=True)
    os.utime(d)
    for old in cands(d)[CAND:]:
        shutil.rmtree(old, ignore_errors=True)


def weight(s):
    """GCONVSPLIT: bytes of the sources a TU file includes (the largest compile starts first)."""
    n = 0
    try:
        for ln in Path(s).read_text(encoding="utf-8", errors="replace").splitlines():
            if ln.startswith('#include "') and not ln.startswith('#include "nd_gconv.h"'):
                try:
                    n += os.path.getsize(ln.split('"')[1])
                except OSError:
                    pass
    except OSError:
        pass
    return n


def trim(root):
    ents = [p for p in Path(root).iterdir() if p.is_dir() and ".tmp." not in p.name]
    if len(ents) > KEEP:
        ents.sort(key=lambda p: p.stat().st_mtime, reverse=True)
        for p in ents[KEEP:]:
            shutil.rmtree(p, ignore_errors=True)


def main():
    a = sys.argv[1:]
    if len(a) < 2 or a[0] != "--flags":
        sys.exit("usage: gconv_objcache.py --flags \"<gcc flags>\" <src.c>...")
    flags, srcs = a[1], a[2:]
    root = os.environ.get("GCONV_OBJ", "")
    mode = os.environ.get("PM_CACHE", "on")
    use = bool(root) and os.path.isdir(root) and mode != "off"
    tool = tool_id()
    keys = {s: primary(s, flags, tool) for s in srcs} if use else {}
    if use:
        # GCONVSPLIT: a v1 entry (key.txt at the top of the folder) is never read by v2: remove it.
        v1 = [p for p in Path(root).iterdir() if (p / "key.txt").is_file()]
        for p in v1:
            shutil.rmtree(p, ignore_errors=True)
        if v1:
            print("gconvobj: removed %d v1 entries" % len(v1), file=sys.stderr)
    todo, hits = [], 0
    for s in srcs:
        if use and mode != "check":
            hit, _, _ = lookup(root, keys[s], s)
            if hit is not None:
                try:  # another run can replace the entry at this moment: then compile
                    shutil.copyfile(hit / "obj.o", s[:-2] + ".o")
                    os.utime(hit)
                    os.utime(hit.parent)
                    hits += 1
                    continue
                except OSError:
                    pass
        todo.append(s)
    jobs = int(os.environ.get("GCONV_JOBS", "16"))  # GCONVSPLIT: 16 at once fit the 4 GB container (FACT oracle probe 2026-11-16)
    order = sorted(todo, key=weight, reverse=True)
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as ex:
        res = dict(zip(order, ex.map(lambda s: compile_one(s, flags), order)))
    bad = [s for s in todo if res[s][0] != 0]
    chk_ok, chk_fail = 0, 0
    chk_stale, chk_new = 0, 0
    if use:
        for i, s in enumerate(todo):
            if res[s][0] != 0:
                continue
            obj = s[:-2] + ".o"
            if mode == "check":
                hit, other, outbad = lookup(root, keys[s], s)
                if hit is not None:
                    if sha1_file(hit / "obj.o") == sha1_file(obj):
                        chk_ok += 1
                    else:
                        chk_fail += 1
                        print("CACHECHECK FAIL gconvobj %s: the entry differs from a cold compile, replaced" % s, file=sys.stderr)
                elif outbad:
                    chk_fail += 1
                    print("CACHECHECK FAIL gconvobj %s: the stored object does not match its out.sha1, replaced" % s, file=sys.stderr)
                elif other:
                    chk_stale += 1
                else:
                    chk_new += 1
            store(root, keys[s], obj, res[s][1], i)
        trim(root)
    print("gconvobj: hit %d miss %d" % (hits, len(todo)), file=sys.stderr)
    if mode == "check" and use:
        print("cachecheck gconvobj ok %d stale %d new %d" % (chk_ok, chk_stale, chk_new) if not chk_fail else "CACHECHECK FAIL gconvobj %d" % chk_fail, file=sys.stderr)
    if bad:
        print("gconvobj: compile failed: %s" % " ".join(bad), file=sys.stderr)
    sys.exit(1 if bad or chk_fail else 0)


if __name__ == "__main__":
    main()
