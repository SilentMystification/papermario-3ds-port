#!/usr/bin/env python3
"""BUILDO: the key phase of 3ds/build.sh in one process (a shell step costs a fork, about 20-150 ms in Git Bash).
  build_key.py pre      map_restore files, code ids (in_now.txt), image ids, asset ids, ROM sha, the texkey, gfx_tab and pso_tab
                        cache lookups (a hit copies the outputs), the shape key, the build key and the skip decision.
                        Writes $OUT/key_env.sh (sourced by build.sh). Exit 10: the build image stamp differs (build.sh runs
                        docker build and calls pre again). Exit 11: docker image inspect failed (engine or image).
  build_key.py post     after generators ran: the pso_tab lookup (when gfx_tab ran) or the build key; writes $OUT/key_env.sh.
  build_key.py src_now  $OUT/src_now.txt: the sync list (code ids, generator outputs and pm_build.h by sha1).
The texts of the keys are the texts of the shell functions of 3ds/tools/gencache.sh and readkey.sh, byte for byte (gc_put and
rk_full of build.sh store entries that this file finds). Every hit checks key.txt and out.sha1 (cache safety rule 2).
Inputs: the environment that build.sh exports (OUT, MAIN_ROOT, ASSET_BASE, PM_CACHE, PM_BUILD_REV, CODE_DIRS, GEN_FILES,
GT_CMD, PT_CMD, SH_CMD, SH_GROUPS, GT_IN, PT_IN, GT_OUT). Host self-test: 3ds/tests/buildkey/run.sh."""
import hashlib
import io
import os
import re
import runpy
import shutil
import subprocess
import sys
from contextlib import redirect_stdout
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import asset_ids  # noqa: E402
import inc_scan  # noqa: E402

E = os.environ
OUT = E.get("OUT", "build3ds")
CACHE = E.get("PM_CACHE", "on") or "on"
REV = E.get("PM_BUILD_REV", "")
GC_ROOT = E.get("GENCACHE_DIR") or (E.get("MAIN_ROOT", ".") + "/build3ds/gencache")
GC_EXCL = ["3ds/include/pm_build.h", "3ds/src/nd_pso_tab.c", "3ds/src/nd_gfx_tab.c", "3ds/src/nd_ntab_descs.c"]
SKIP_RE = re.compile(r"(^|/)__pycache__/|\.pyc$|\.tmp\.")


def sha1b(b):
    return hashlib.sha1(b).hexdigest()


def sha1f(p):
    h = hashlib.sha1()
    with open(p, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def git(*a, inp=None):
    r = subprocess.run(["git", "-c", "core.quotepath=off"] + list(a), input=inp, capture_output=True)
    if r.returncode != 0:
        raise SystemExit("build_key: git %s failed: %s" % (" ".join(a[:3]), r.stderr.decode(errors="replace")))
    return r.stdout.decode("utf-8", "surrogateescape")


def csort(lines):
    return sorted(set(lines), key=lambda s: s.encode("utf-8", "surrogateescape"))


def under(p, paths):
    return any(p == a or p.startswith(a + "/") for a in paths)


# ---- in_manifest (src_sync.sh), same lines -------------------------------------------------------------------------------
def in_manifest(paths):
    ids, dels = {}, set()
    hl = []
    if REV:
        allf = []
        for a in paths:
            if os.path.isfile(a) and not os.path.islink(a):
                allf.append(a)
            for root, dirs, files in os.walk(a):
                for n in files:
                    p = (root + "/" + n).replace("\\", "/")
                    if not os.path.islink(p):
                        allf.append(p)
        allf = [p for p in allf if not SKIP_RE.search(p)]
        have = set(allf)
        for ln in git("ls-tree", "-r", REV, "--", *paths).splitlines():
            meta, p = ln.split("\t", 1)
            m = meta.split()
            if m[1] == "blob" and p in have:
                ids[p] = "b:" + m[2]
        hl = csort(p for p in allf if p not in ids)
    else:
        for ln in git("ls-files", "-s", "--", *paths).splitlines():
            meta, p = ln.split("\t", 1)
            ids[p] = "b:" + meta.split()[1]
        mo = csort(p for p in git("ls-files", "-m", "-o", "--", *paths).splitlines() if p and not SKIP_RE.search(p))
        for p in mo:
            (hl.append(p) if os.path.isfile(p) else dels.add(p))
    if hl:
        out = git("hash-object", "--no-filters", "--stdin-paths", inp=("\n".join(hl) + "\n").encode("utf-8", "surrogateescape")).split()
        if len(out) != len(hl):
            raise SystemExit("build_key: hash-object gave %d ids for %d files" % (len(out), len(hl)))
        for p, i in zip(hl, out):
            ids[p] = "h:" + i
    return csort("%s\t%s" % (p, i) for p, i in ids.items() if p not in dels)


def idmap(lines):
    return dict(ln.split("\t", 1) for ln in lines)


def in_pick(lines, paths, excl):
    return [ln for ln in lines if ln.split("\t", 1)[0] not in excl and under(ln.split("\t", 1)[0], paths)]


def gen_ids(files):
    return ["%s\ts:%s" % (f, sha1f(f)) for f in files if os.path.isfile(f)]


# ---- gencache.sh / readkey.sh ----------------------------------------------------------------------------------------------
def gc_dir(gen, key):
    return Path(GC_ROOT) / gen / sha1b(key.encode("utf-8", "surrogateescape"))[:16]


def sums_ok(d):
    """sha1sum -c --status out.sha1 in d (lines "<sha1> *name" or "<sha1>  name")."""
    try:
        lines = [ln for ln in (d / "out.sha1").read_text(encoding="utf-8").splitlines() if ln.strip()]
        if not lines:
            return False
        for ln in lines:
            h, name = ln.split(" ", 1)
            name = name[1:] if name[:1] in ("*", " ") else name
            if sha1f(d / name) != h:
                return False
        return True
    except (OSError, ValueError):
        return False


def gc_ok(d, key):
    if not d.is_dir():
        return False
    try:
        kt = (d / "key.txt").read_bytes().decode("utf-8", "surrogateescape").rstrip("\n")
    except OSError:
        kt = None
    if kt == key and sums_ok(d):
        return True
    print("gencache: entry %s failed its check (key or file hash), removed" % d.as_posix(), file=sys.stderr)
    shutil.rmtree(d, ignore_errors=True)
    return False


def gc_get(gen, key, pairs):
    if CACHE in ("off", "check"):
        return False
    d = gc_dir(gen, key)
    if not gc_ok(d, key):
        return False
    for name, _ in pairs:
        if not (d / name).is_file():
            shutil.rmtree(d, ignore_errors=True)
            return False
    for name, dst in pairs:
        src = d / name
        if not (os.path.isfile(dst) and filecmp(src, dst)):
            os.makedirs(os.path.dirname(dst) or ".", exist_ok=True)
            shutil.copyfile(src, dst)
    return True


def filecmp(a, b):
    if os.path.getsize(a) != os.path.getsize(b):
        return False
    with open(a, "rb") as fa, open(b, "rb") as fb:
        return fa.read() == fb.read()


def rk_id():
    return sha1b((HERE / "readtrace.c").read_bytes() + (HERE / "readkey.sh").read_bytes())


def rk_paths(lines, paths):
    s = [ln.split("\t", 1)[0] for ln in lines if under(ln.split("\t", 1)[0], paths)]
    s += [x for x in GC_EXCL if under(x, paths) and os.path.isfile(x)]
    return csort(s)


def rk_ids(idm, reads):
    out = []
    for p in reads:
        i = idm.get(p) if p not in GC_EXCL else None
        if i is None:
            i = ("d:" + sha1f(p)) if os.path.isfile(p) else "-"
        out.append("%s\t%s" % (p, i))
    return "\n".join(out)


def rk_get(gen, k0, idm, pairs):
    """(hit, RK_DIR)."""
    rkd = "%s/%s" % (gen, sha1b(k0.encode("utf-8", "surrogateescape"))[:16])
    if CACHE in ("off", "check"):
        return False, rkd
    base = Path(GC_ROOT) / rkd
    for c in sorted(base.glob("*/")) if base.is_dir() else []:
        if ".tmp." in c.name or not (c / "reads.txt").is_file():
            continue
        reads = [ln for ln in (c / "reads.txt").read_text(encoding="utf-8").splitlines() if ln]
        full = "%s\nreads\n%s" % (k0, rk_ids(idm, reads))
        if gc_get(rkd, full, pairs):
            return True, rkd
    return False, rkd


def pairs_of(text):
    return [tuple(w.split(":", 1)) for w in text.split()]


def write_lines(p, lines):
    with open(p + ".tmp", "w", encoding="utf-8", newline="\n") as f:
        f.write("".join(ln + "\n" for ln in lines))
    os.replace(p + ".tmp", p)


# ---- the steps -------------------------------------------------------------------------------------------------------------
def img_ids():
    r = subprocess.run(["docker", "image", "inspect", "-f", "{{.Id}} {{json .RootFS.Layers}}", "pm3ds-build:1", "pm3ds-host:1"],
                       capture_output=True, text=True, timeout=60)
    ln = [x for x in r.stdout.splitlines() if x.strip()]
    if r.returncode != 0 or len(ln) != 2:
        return None
    # the layer list + newline, as `docker image inspect -f '{{json .RootFS.Layers}}' | sha1sum` hashed it
    return [(x.split(" ", 1)[0], sha1b((x.split(" ", 1)[1] + "\n").encode())) for x in ln]


def gt_k0(lines, himg, ahash):
    paths = [p for p in rk_paths(lines, E["GT_IN"].split()) if p != "3ds/include/texkey_patch.h"]
    write_lines(OUT + "/rk_gfx_tab_paths.txt", paths)
    return "gfx_tab v4\ncmd %s\nhimg %s\nassets %s\nrk %s\npaths %s" % (
        E["GT_CMD"], himg, ahash, rk_id(), sha1b(("".join(p + "\n" for p in paths)).encode("utf-8", "surrogateescape")))


def pt_k0(lines, idm, himg, ahash):
    paths = [p for p in rk_paths(lines, E["PT_IN"].split()) if p not in ("3ds/src/nd_pso_tab.c", "3ds/include/texkey_patch.h")]
    write_lines(OUT + "/rk_pso_tab_paths.txt", paths)
    r = subprocess.run(["git", "-c", "core.quotepath=off", "grep", "-a", "--untracked", "--no-exclude-standard", "-lF", "ND_PSO_DESC", "--", "src", "3ds/src"],
                       capture_output=True)
    pset = set(paths)
    scan = csort(p for p in r.stdout.decode("utf-8", "surrogateescape").splitlines() if p.endswith(".c") and p in pset)
    write_lines(OUT + "/rk_pso_tab_scan.txt", scan)
    return "pso_tab v4\ncmd %s\nhimg %s\nassets %s\nrk %s\ngfxtab %s %s\npaths %s\nscan\n%s" % (
        E["PT_CMD"], himg, ahash, rk_id(), sha1f("3ds/src/nd_gfx_tab.c"), sha1f("3ds/src/nd_ntab_descs.c"),
        sha1b(("".join(p + "\n" for p in paths)).encode("utf-8", "surrogateescape")), rk_ids(idm, scan))


def shape_key(lines, idm, himg, romsha):
    g = E["SH_GROUPS"].split("|")
    src = OUT + "/shape_reads.txt"
    try:
        with redirect_stdout(io.StringIO()):
            rc = inc_scan.main(["inc_scan", src] + g)
    except SystemExit as e:
        rc = e.code
    if rc != 0:
        keep = ["include", "3ds/include", "3ds/stub_include", "3ds/tools/yay0_test.c", "3ds/tools/nd_shape_conv.c", "3ds/src/nd_yay0.c", "3ds/src/nd_seam.c", "3ds/src/nd_dlc.c"]
        write_lines(src, [ln.split("\t", 1)[0] for ln in in_pick(lines, keep, set(E["GEN_FILES"].split()) | {"3ds/include/pm_build.h"})])
    reads = [ln for ln in Path(src).read_text(encoding="utf-8").splitlines() if ln]
    miss = [p for p in reads if p not in idm]
    if miss:
        raise SystemExit("shape key: a read file has no content id: %s" % " ".join(miss[:3]))
    text = "shape v2\ncmd %s\nhimg %s\nrom %s\nreads\n" % (E["SH_CMD"], himg, romsha) + "".join("%s\t%s\n" % (p, idm[p]) for p in reads)
    Path(OUT + "/shape_keytext.txt").write_bytes(text.encode("utf-8", "surrogateescape"))
    se = sha1b(text.encode("utf-8", "surrogateescape"))
    Path(OUT + "/shape_key.txt").write_text(se, encoding="utf-8", newline="\n")
    return se


def build_key(lines, env):
    keyin = E["CODE_DIRS"].split() + ["docker/Dockerfile.build", "3ds/build.sh"]
    gens = E["GEN_FILES"].split()
    excl = set(gens) | {"3ds/include/pm_build.h"}
    t = ["key v2"] + in_pick(lines, keyin, excl) + gen_ids(["3ds/src/map_restore_table.c", "3ds/ld/map_restore.ld", "3ds/ld/map_restore_bss.ld"] + gens)
    t.append("assets %s" % env["AHASH"])
    t.append("shape %s" % env["SHAPE_E"])
    t.append("img %s %s" % (env["IMGID"], env["HIMGID"]))
    return sha1b(("\n".join(t) + "\n").encode("utf-8", "surrogateescape"))


def skip_ok(key):
    o = Path(OUT)
    try:
        return (not REV and CACHE == "on" and (o / "pm_3ds.3dsx").is_file()
                and (o / "build_key.txt").read_text(encoding="utf-8").strip() == key
                and (o / "build_out.sha1").read_text(encoding="utf-8").strip() == sha1f(o / "pm_3ds.3dsx"))
    except OSError:
        return False


def write_env(env):
    with open(OUT + "/key_env.sh.tmp", "w", encoding="utf-8", newline="\n") as f:
        for k, v in env.items():
            f.write("%s='%s'\n" % (k, str(v).replace("'", "'\\''")))
    os.replace(OUT + "/key_env.sh.tmp", OUT + "/key_env.sh")


def load_env():
    env = {}
    for ln in Path(OUT + "/key_env.sh").read_text(encoding="utf-8").splitlines():
        k, v = ln.split("=", 1)
        env[k] = v[1:-1].replace("'\\''", "'")
    return env


def read_lines(p):
    return [ln for ln in Path(p).read_text(encoding="utf-8").splitlines() if ln]


def cmd_pre():
    # MRTGEN: gen_map_restore.py writes a file only when its content changes (git-ignored outputs)
    old = sys.argv
    try:
        sys.argv = [str(HERE / "gen_map_restore.py")]
        with redirect_stdout(io.StringIO()):
            runpy.run_path(str(HERE / "gen_map_restore.py"), run_name="__main__")
    except SystemExit as e:
        if e.code not in (None, 0):
            print("gen_map_restore.py failed", file=sys.stderr)
            return 1
    finally:
        sys.argv = old
    paths = E["CODE_DIRS"].split() + ["3ds/tools", "docker", "3ds/build.sh"]
    lines = in_manifest(paths)
    write_lines(OUT + "/in_now.txt", lines)
    idm = idmap(lines)
    ids = img_ids()
    if ids is None:
        return 11
    dkey = sha1b("".join(ln + "\n" for ln in in_pick(lines, ["docker"], set())).encode("utf-8", "surrogateescape"))
    stamp = Path(E["MAIN_ROOT"]) / "build3ds" / "img_build_stamp.txt"
    want = "%s %s" % (dkey, ids[0][0])
    try:
        have = stamp.read_text(encoding="utf-8").strip()
    except OSError:
        have = ""
    env = {"DKEY": dkey, "IMGID": ids[0][1], "HIMGID": ids[1][1], "HIMG_FULL": ids[1][0]}
    if have != want:
        write_env(env)
        return 10
    # assets (asset_ids.py with the stat cache) and the ROM
    base = E["ASSET_BASE"]
    cache = None
    if CACHE != "off":
        cache = "%s/build3ds/asset_stat_%s.txt" % (E["MAIN_ROOT"], sha1b(str(Path(base).resolve().as_posix()).encode())[:8])
    ap, ai = asset_ids.manifest(base, ["assets/us", "ver/us/build"], cache)
    if not ap:
        raise SystemExit("asset_manifest: no asset files under %s" % base)
    write_lines(OUT + "/asset_paths.txt", ap)
    write_lines(OUT + "/asset_now.txt", ["%s\t%s" % (p, i) for p, i in zip(ap, ai)])
    ahash = sha1f(OUT + "/asset_now.txt")
    env["AHASH"] = ahash
    env["AVOL"] = ("pm3ds-assets-cold%s" % E.get("PID", os.getpid())) if CACHE == "off" else "pm3ds-assets-" + ahash[:16]
    rom = base + "/ver/us/baserom.z64"
    if not os.path.isfile(rom):
        raise SystemExit("missing %s" % rom)
    env["ROMSHA"] = romsha = sha1f(rom)
    # texkey (gencache texkey); the key text of build.sh tk_key
    tk = "texkey v1\ntool %s\npython %s\nrom %s\nassets %s" % (sha1f("3ds/tools/texkey_bake.py"), sys.version, romsha, ahash)
    Path(OUT + "/tk_key.txt").write_bytes(tk.encode("utf-8"))
    env["TK_MISS"] = 0 if gc_get("texkey", tk, [("texkey_patch.h", "3ds/include/texkey_patch.h")]) else 1
    if env["TK_MISS"] == 0:
        print("texkey_bake: cache hit", file=sys.stderr)
    env["SHAPE_E"] = se = shape_key(lines, idm, ids[1][1], romsha)
    sh_hit = CACHE == "on" and (Path(GC_ROOT) / "shapevol" / se).is_file()
    env["SH_MISS"] = 0 if sh_hit or (E.get("PM_KEY_ONLY") and CACHE != "check") else 1
    if sh_hit:
        print("gen_shapes: cache hit", file=sys.stderr)
    k0 = gt_k0(lines, ids[1][1], ahash)
    Path(OUT + "/gt_k0.txt").write_bytes(k0.encode("utf-8", "surrogateescape"))
    hit, env["GT_RKDIR"] = rk_get("gfx_tab", k0, idm, pairs_of(E["GT_OUT"]))
    env["GT_MISS"] = 0 if hit else 1
    env["PT_MISS"] = 2  # unknown until gfx_tab ran
    if hit:
        print("gen_gfx_tab: cache hit", file=sys.stderr)
        pt_step(env, lines, idm)
    return finish(env, lines)


def pt_step(env, lines, idm):
    k0 = pt_k0(lines, idm, env["HIMGID"], env["AHASH"])
    Path(OUT + "/pt_k0.txt").write_bytes(k0.encode("utf-8", "surrogateescape"))
    hit, env["PT_RKDIR"] = rk_get("pso_tab", k0, idm, [("nd_pso_tab.c", "3ds/src/nd_pso_tab.c")])
    env["PT_MISS"] = 0 if hit else 1
    if hit:
        print("gen_pso_tab: cache hit", file=sys.stderr)


def finish(env, lines):
    done = all(str(env.get(k)) == "0" for k in ("TK_MISS", "SH_MISS", "GT_MISS", "PT_MISS"))
    env["KEY"] = build_key(lines, env) if done else ""
    env["SKIP"] = 1 if done and skip_ok(env["KEY"]) else 0
    write_env(env)
    return 0


def cmd_post():
    """After the generators: the pso_tab lookup when gfx_tab ran (pso_only), else the build key."""
    env = load_env()
    lines = read_lines(OUT + "/in_now.txt")
    if sys.argv[2:] == ["pso"]:
        pt_step(env, lines, idmap(lines))
        write_env(env)
        return 0
    for k in ("TK_MISS", "SH_MISS", "GT_MISS", "PT_MISS"):
        env[k] = 0
    return finish(env, lines)


def cmd_src_now():
    lines = read_lines(OUT + "/in_now.txt")
    gens = E["GEN_FILES"].split() + ["3ds/include/pm_build.h"]
    s = in_pick(lines, E["CODE_DIRS"].split(), set(gens)) + gen_ids(gens)
    write_lines(OUT + "/src_now.txt", csort(s))
    return 0


if __name__ == "__main__":
    c = sys.argv[1] if len(sys.argv) > 1 else ""
    sys.exit({"pre": cmd_pre, "post": cmd_post, "src_now": cmd_src_now}.get(c, lambda: (print(__doc__), 2)[1])())
