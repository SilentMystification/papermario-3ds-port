#!/usr/bin/env python3
"""GFXINC: the object cache of the gfx table tool inputs (3ds/tools/gen_gfx_tab.sh). Runs in the host image pm3ds-host:1.
Usage: gconv_objcache.py --flags "<gcc flags>" <src.c>...   Each object is written next to its source (<src>.o).
Cache folder: env GCONV_OBJ (the docker volume pm3ds-gconvobj-<host image id>). Unset, missing, or PM_CACHE=off: every source
compiles and nothing is stored. PM_CACHE=check: every source compiles and its object is compared with the entry (cachecheck).
Cache safety (docs/handoff/plan_buildspeed.md "Cache safety"):
- the primary key is the text: tool version, gcc and as versions, the flags, the source path and the sha1 of the source;
- the entry <GCONV_OBJ>/<16 hex of sha1(primary key)>/ holds key.txt, deps.sha1 (the sha1 of every file the compile read:
  gcc -MD gives the #include files, as --MD the .incbin files), out.sha1 and obj.o;
- a hit needs key.txt equal, every dep file equal to its sha1 and obj.o equal to out.sha1; anything else removes the entry and
  the source compiles; a new entry is a temp folder renamed in one step.
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

VERSION = "gconv-obj v1"
KEEP = int(os.environ.get("GCONV_OBJ_KEEP", "3000"))
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


def entry_ok(d, key):
    try:
        if (d / "key.txt").read_bytes().decode("utf-8") != key:
            return False
        for ln in (d / "deps.sha1").read_text(encoding="utf-8").splitlines():
            h, path = ln.split("  ", 1)
            if dep_hash(path) != h:
                return False
        return (d / "out.sha1").read_text(encoding="utf-8").strip() == sha1_file(d / "obj.o")
    except (OSError, ValueError, UnicodeDecodeError):
        return False


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
    tmp = d.with_name(d.name + ".tmp.%d.%d" % (os.getpid(), tag))
    shutil.rmtree(tmp, ignore_errors=True)
    tmp.mkdir(parents=True)
    shutil.copyfile(obj, tmp / "obj.o")
    (tmp / "key.txt").write_bytes(key.encode("utf-8"))
    (tmp / "deps.sha1").write_text("".join("%s  %s\n" % (dep_hash(p), p) for p in deps), encoding="utf-8")
    (tmp / "out.sha1").write_text(sha1_file(tmp / "obj.o") + "\n", encoding="utf-8")
    shutil.rmtree(d, ignore_errors=True)
    try:
        os.rename(tmp, d)
    except OSError:
        shutil.rmtree(tmp, ignore_errors=True)


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
    todo, hits = [], 0
    for s in srcs:
        if use and mode != "check":
            d = entry_dir(root, keys[s])
            if d.is_dir():
                if entry_ok(d, keys[s]):
                    shutil.copyfile(d / "obj.o", s[:-2] + ".o")
                    os.utime(d)
                    hits += 1
                    continue
                print("gconvobj: entry %s of %s failed its check (key, dep or object hash), removed" % (d.name, s), file=sys.stderr)
                shutil.rmtree(d, ignore_errors=True)
        todo.append(s)
    jobs = int(os.environ.get("GCONV_JOBS", "4"))
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as ex:
        res = dict(zip(todo, ex.map(lambda s: compile_one(s, flags), todo)))
    bad = [s for s in todo if res[s][0] != 0]
    chk_ok, chk_fail = 0, 0
    if use:
        for i, s in enumerate(todo):
            if res[s][0] != 0:
                continue
            obj = s[:-2] + ".o"
            if mode == "check":
                d = entry_dir(root, keys[s])
                if d.is_dir() and entry_ok(d, keys[s]):
                    if sha1_file(d / "obj.o") == sha1_file(obj):
                        chk_ok += 1
                    else:
                        chk_fail += 1
                        print("CACHECHECK FAIL gconvobj %s: the entry differs from a cold compile, replaced" % s, file=sys.stderr)
            store(root, keys[s], obj, res[s][1], i)
        trim(root)
    print("gconvobj: hit %d miss %d" % (hits, len(todo)), file=sys.stderr)
    if mode == "check" and use:
        print("cachecheck gconvobj ok %d" % chk_ok if not chk_fail else "CACHECHECK FAIL gconvobj %d" % chk_fail, file=sys.stderr)
    if bad:
        print("gconvobj: compile failed: %s" % " ".join(bad), file=sys.stderr)
    sys.exit(1 if bad or chk_fail else 0)


if __name__ == "__main__":
    main()
