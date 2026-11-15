"""GSET: move the Gfx lists of a device source into the host twin (3ds/tools/gfxin) and rewrite their users.

Usage: gset.py <gset.d file> [gset_ok.txt] [root]
  gset.d file: one device source per line (src/effects/gfx/<x>.c or src/entity/model/<x>.c), '#' comments.
  gset_ok.txt: the lists the converter (nd_gfx_conv.c, gset_check) marks ok, one name per line.
  root: the repo root (default: the current directory). Run from the repo root.

Include mode (src/effects/gfx/<x>.c): each `#include "<inc>.inc.c"` whose assets/us/<inc> defines Gfx lists, all of them ok, moves to the twin
3ds/tools/gfxin/<source path> (the twin's first line includes the source; gen_gfx_syms.py reads both).
Block mode (any other device source, e.g. src/entity/model/<x>.c): each block `Gfx X[] = {` .. the first later line that is exactly `};` moves when X is in gset_ok.txt and
X is not named on a hold line (hold_line) or in an unmoved block of the file (a fixpoint: a removed name can block another). The block moves to the twin, in file
order, after the first line `#include "<source path>"`. The device file keeps one line `NDN_DECL(X);` in its place.
Then every source file under src/ and include/ (not src/effects/gfx) rewrites the moved names: `extern Gfx X[];` -> `NDN_DECL(X);`, any other use of X -> `NDN(X)`
(nd_gtab.h). A line with `X[`, `&X`, `sizeof`, BranchList or SetCustomGfx stays and prints `gset keep <file>:<line>`: the list has no symbol, so that
line must be edited by hand. Prints `gset moved <k> files <f> keep <c>` then `gset ok` or `gset FAIL` (exit 1 on a keep line). A user file that defines a moved name itself keeps that name and prints `gset own <file> <n>`."""
import pathlib
import re
import sys

KEEP_RULE = True  # the seeded test (3ds/tests/host.d/gset.sh) runs a copy with this False and expects a diff
HOLD_RULE = True  # GSETB: the seeded test (gset.sh) runs a copy with this False and expects a diff
OWN_RULE = True  # ISTAR: a user file that defines a moved name keeps its own copy (PAL twins); the seeded test (gset.sh) runs a copy with this False
NS_RULE = True  # GSETW: N(x) is rewritten only in the files of its namespace; the seeded test (gset.sh) runs a copy with this False
BLOCK_END = "};"  # the block closing line (exact, CR dropped); the seeded test in gset.sh changes this and is_block_end
DEF = re.compile(r"^\s*(?:const\s+)?Gfx\s+(\w+)\s*\[", re.M)
BLOCK_OPEN = re.compile(r"^Gfx (?:N\((\w+)\)|(\w+))\[\] = \{")
INC = re.compile(r'^(\s*#include\s+"([^"]+\.inc\.c)")\s*$', re.M)
DECL = re.compile(r"^(\s*)extern\s+Gfx\s+(\w+)\s*\[\s*\]\s*;")
NS_DEF = re.compile(r"^\s*#\s*define\s+NAMESPACE\s+(\w+)", re.M)
NDECL = re.compile(r"^(\s*)extern\s+Gfx\s+(N\(\w+\))\s*\[\s*\]\s*;")
NSN = {}  # GSETW: moved N(x) name -> (token, namespace, directory)
SKIP_DIR = "src/effects/gfx/"


def read_names(p):
    return [s.strip() for s in p.read_text(encoding="utf-8").splitlines() if s.strip() and not s.strip().startswith("#")]


def gfx_names(text):
    return DEF.findall(text)


def assets_dir(root):
    """assets/us is git-ignored: a worktree reads it from the main folder (the parent of the git common dir), as listcen.py does."""
    if (root / "assets/us").is_dir():
        return root / "assets/us"
    import subprocess
    try:
        common = subprocess.check_output(["git", "-C", str(root), "rev-parse", "--path-format=absolute", "--git-common-dir"], text=True, stderr=subprocess.DEVNULL).strip()
    except (subprocess.CalledProcessError, OSError):
        return root / "assets/us"  # not a git repo (a fixture): no assets, include mode finds none
    return pathlib.Path(common).parent / "assets/us"


def add_gtab(out):
    """Adds `#include "nd_gtab.h"` after the first #include line of the lines (or first line when there is none)."""
    joined = "\n".join(out)
    inc = re.search(r'^\s*#include\s+"[^"]+"', joined, re.M)
    if inc:
        idx = joined.find(inc.group(0)) + len(inc.group(0))
        joined = joined[:idx] + '\n#include "nd_gtab.h"' + joined[idx:]
        return joined.split("\n")
    return ['#include "nd_gtab.h"'] + out


def move_includes(root, src, ok, twin_dir, assets):
    """Include mode. Returns (moved names, the twin path or None)."""
    p = root / src
    lines = p.read_text(encoding="utf-8").split("\n")
    moved, keep_lines, out = [], [], []
    for ln in lines:
        m = INC.match(ln)
        if not m:
            out.append(ln)
            continue
        inc = m.group(2)
        a = assets / inc
        if not a.exists():
            out.append(ln)
            continue
        names = gfx_names(a.read_text(encoding="utf-8", errors="replace"))
        if names and all(n in ok for n in names):
            moved.extend(names)
            keep_lines.append(f'#include "{inc}"')
        else:
            out.append(ln)
    if not keep_lines:
        return [], None
    p.write_text("\n".join(out), encoding="utf-8")
    twin = root / twin_dir / src
    twin.parent.mkdir(parents=True, exist_ok=True)
    old = twin.read_text(encoding="utf-8").split("\n") if twin.exists() else [f'#include "{src}"']
    body = old[:1] + [x for x in old[1:] if x] + [x for x in keep_lines if x not in old]
    twin.write_text("\n".join(body) + "\n", encoding="utf-8")
    return moved, twin


def is_block_end(ln):
    return ln.rstrip() == BLOCK_END


def file_ns(p, seen=None):
    """GSETW: the namespace of a device source: its NAMESPACE define, else that of an included header, else that of the .c file that includes it."""
    seen = set() if seen is None else seen
    if p in seen:
        return None
    seen.add(p)
    text = p.read_text(encoding="utf-8", errors="replace")
    m = NS_DEF.search(text)
    if m:
        return m.group(1)
    for inc in re.findall(r'^\s*#include\s+"([^"]+\.h)"', text, re.M):
        h = p.parent / inc
        if h.is_file():
            m = NS_DEF.search(h.read_text(encoding="utf-8", errors="replace"))
            if m:
                return m.group(1)
    for q in sorted(p.parent.glob("*.c")):
        if q == p:
            continue
        if re.search(r'^\s*#include\s+"' + re.escape(p.name) + r'"', q.read_text(encoding="utf-8", errors="replace"), re.M):
            r = file_ns(q, seen)
            if r:
                return r
    return None


def find_blocks(lines, ns=None, tok=None):
    """[(name, first line, last line)] of the Gfx blocks with a closing line (indices, both inclusive), in file order.
    GSETW: `Gfx N(x)[] = {` gives the name ns_x (with ns) and tok[ns_x] = "N(x)"; without ns it gives no entry."""
    found, i = [], 0
    while i < len(lines):
        m = BLOCK_OPEN.match(lines[i])
        if m:
            j = i + 1
            while j < len(lines) and not is_block_end(lines[j]):
                j += 1
            if j < len(lines):
                if m.group(2):
                    found.append((m.group(2), i, j))
                elif ns:
                    found.append((ns + "_" + m.group(1), i, j))
                    tok[ns + "_" + m.group(1)] = "N(" + m.group(1) + ")"
                i = j + 1
                continue
        i += 1
    return found


def hold_line(ln, hit):
    """True when the line needs the list's symbol: an index, an address, sizeof, BranchList or SetCustomGfx (GSETB)."""
    return "BranchList" in ln or "sizeof" in ln or "SetCustomGfx" in ln or any(re.search(r"(?<!\w)" + re.escape(n) + r"\s*\[|&\s*" + re.escape(n) + r"(?!\w)", ln) for n in hit)


def move_blocks(root, src, ok, twin_dir):
    """Block mode. Returns (moved names, the twin path or None)."""
    p = root / src
    lines = p.read_text(encoding="utf-8").split("\n")
    ns = file_ns(p)
    tok = {}
    blocks = find_blocks(lines, ns, tok)
    T = lambda n: tok.get(n, n)
    moving = {n for n, _, _ in blocks if n in ok}
    in_block = set()
    for _, s, e in blocks:
        in_block.update(range(s, e + 1))
    if HOLD_RULE:
        for k, ln in enumerate(lines):
            if k in in_block or DECL.match(ln) or NDECL.match(ln):
                continue
            hit = {n for n in moving if re.search(r"(?<!\w)" + re.escape(T(n)) + r"(?!\w)", ln)}
            if hit and hold_line(ln, {T(n) for n in hit}):
                moving -= hit
    while True:
        inside = set()
        for n, s, e in blocks:
            if n in moving:
                inside.update(range(s, e + 1))
        unmoved = "\n".join(lines[k] for k in sorted(in_block) if k not in inside)
        bad = {n for n in moving if re.search(r"(?<!\w)" + re.escape(T(n)) + r"(?!\w)", unmoved)}
        if not bad:
            break
        moving -= bad
    if not moving:
        return [], None
    out, body, moved, k = [], [], [], 0
    starts = {s: (n, e) for n, s, e in blocks if n in moving}
    while k < len(lines):
        if k in starts:
            n, e = starts[k]
            out.append(f"NDN_DECL({T(n)});")
            body.extend(lines[k:e + 1])
            moved.append(n)
            k = e + 1
            continue
        out.append(lines[k])
        k += 1
    if "nd_gtab.h" not in "\n".join(lines):
        out = add_gtab(out)
    p.write_text("\n".join(out), encoding="utf-8")
    twin = root / twin_dir / src
    twin.parent.mkdir(parents=True, exist_ok=True)
    old = twin.read_text(encoding="utf-8").split("\n") if twin.exists() else [f'#include "{src}"']
    tbody = old[:1] + [x for x in old[1:] if x] + body
    twin.write_text("\n".join(tbody) + "\n", encoding="utf-8")
    for n in moved:
        if n in tok:
            NSN[n] = (tok[n], ns, p.parent)
    return moved, twin


def rewrite_users(root, names):
    """Returns (files rewritten, keep lines printed)."""
    if not names:
        return 0, 0
    alt = "|".join(re.escape(n) for n in names)
    shield = re.compile(r"(?<!NDN\()(?<!NDN_DECL\()(?<!\w)(" + alt + r")(?!\w)")
    files = []
    for pat in ("src/**/*.c", "src/**/*.h", "include/**/*.h"):
        for f in root.glob(pat):
            rel = f.relative_to(root).as_posix()
            if rel.startswith(SKIP_DIR) or not f.is_file():
                continue
            files.append((rel, f))
    nfiles, nkeep = 0, 0
    for rel, f in sorted(files):
        text = f.read_text(encoding="utf-8", errors="replace")
        if not shield.search(text):
            continue
        own = set(DEF.findall(text)) & set(names) if OWN_RULE else set()
        sh = shield
        if own:
            print(f"gset own {rel} {len(own)}")
            rest = [n for n in names if n not in own]
            if not rest:
                continue
            sh = re.compile(r"(?<!NDN\()(?<!NDN_DECL\()(?<!\w)(" + "|".join(re.escape(n) for n in rest) + r")(?!\w)")
        out, changed, keep_here = [], False, 0
        for i, ln in enumerate(text.split("\n"), 1):
            if not sh.search(ln):
                out.append(ln)
                continue
            d = DECL.match(ln)
            if d and d.group(2) in names:
                out.append(f"{d.group(1)}NDN_DECL({d.group(2)});")
                changed = True
                continue
            hit = sh.findall(ln)
            bad = KEEP_RULE and hold_line(ln, hit)
            if bad:
                print(f"gset keep {rel}:{i}")
                keep_here += 1
                nkeep += 1
                out.append(ln)
                continue
            out.append(sh.sub(lambda m: f"NDN({m.group(1)})", ln))
            changed = True
        if not changed:
            continue
        if "nd_gtab.h" not in text:
            out = add_gtab(out)
        f.write_text("\n".join(out), encoding="utf-8")
        nfiles += 1
    return nfiles, nkeep


def rewrite_ns(root):
    """GSETW: the N(x) names of the moved lists, rewritten in the sources of their directory and namespace (NS_RULE).
    Returns (files changed, keep lines printed)."""
    groups = {}
    for _, (tk, ns, d) in NSN.items():
        groups.setdefault((d, ns), set()).add(tk)
    nfiles, nkeep = 0, 0
    for (d, ns), toks in sorted(groups.items(), key=lambda kv: (kv[0][0].as_posix(), kv[0][1])):
        alt = "|".join(re.escape(t) for t in sorted(toks))
        shield = re.compile(r"(?<!NDN\()(?<!NDN_DECL\()(?<!\w)(" + alt + r")(?!\w)")
        for f in sorted(d.glob("*.c")) + sorted(d.glob("*.h")):
            if not f.is_file() or (NS_RULE and file_ns(f) != ns):
                continue
            rel = f.as_posix()
            text = f.read_text(encoding="utf-8", errors="replace")
            out, changed = [], False
            for i, ln in enumerate(text.split("\n"), 1):
                dm = NDECL.match(ln)
                if dm and dm.group(2) in toks:
                    out.append(f"{dm.group(1)}NDN_DECL({dm.group(2)});")
                    changed = True
                    continue
                if not shield.search(ln):
                    out.append(ln)
                    continue
                hit = shield.findall(ln)
                if KEEP_RULE and hold_line(ln, set(hit)):
                    print(f"gset keep {rel}:{i}")
                    nkeep += 1
                    out.append(ln)
                    continue
                out.append(shield.sub(lambda m: f"NDN({m.group(1)})", ln))
                changed = True
            if not changed:
                continue
            if "nd_gtab.h" not in text:
                out = add_gtab(out)
            f.write_text("\n".join(out), encoding="utf-8")
            nfiles += 1
    return nfiles, nkeep


def main(argv):
    if len(argv) < 2:
        print(__doc__, file=sys.stderr)
        return 2
    lst = pathlib.Path(argv[1])
    root = pathlib.Path(argv[3]) if len(argv) > 3 else pathlib.Path(".")
    ok = set(read_names(pathlib.Path(argv[2]) if len(argv) > 2 else pathlib.Path("build3ds/out/gset_ok.txt")))
    all_moved, files_done = [], 0
    assets = assets_dir(root)
    for src in read_names(lst):
        if src.startswith(SKIP_DIR):
            moved, twin = move_includes(root, src, ok, "3ds/tools/gfxin", assets)
        else:
            moved, twin = move_blocks(root, src, ok, "3ds/tools/gfxin")
        if twin is not None:
            all_moved.extend(moved)
    nf, nkeep = rewrite_users(root, sorted(set(all_moved)))
    nf2, nk2 = rewrite_ns(root)
    files_done = nf + nf2
    nkeep += nk2
    print(f"gset moved {len(set(all_moved))} files {files_done} keep {nkeep}")
    if nkeep:
        print("gset FAIL")
        return 1
    print("gset ok")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
