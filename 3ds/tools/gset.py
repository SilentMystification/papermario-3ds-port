"""GSET: move the Gfx lists of a device source into the host twin (3ds/tools/gfxin) and rewrite their users.

Usage: gset.py <gset.d file> [gset_ok.txt] [root]
  gset.d file: one device source per line (src/effects/gfx/<x>.c), '#' comments.
  gset_ok.txt: the lists the converter (nd_gfx_conv.c, gset_check) marks ok, one name per line.
  root: the repo root (default: the current directory). Run from the repo root.

Per device source: each `#include "<inc>.inc.c"` whose assets/us/<inc> defines Gfx lists, all of them ok, moves to the twin
3ds/tools/gfxin/<source path> (the twin's first line includes the source; gen_gfx_syms.py reads both). Then every source file under
src/ and include/ (not src/effects/gfx) rewrites the moved names: `extern Gfx X[];` -> `NDN_DECL(X);`, any other use of X -> `NDN(X)`
(nd_gtab.h). A line with `X[`, `&X`, `sizeof` or BranchList stays and prints `gset keep <file>:<line>`: the list has no symbol, so that
line must be edited by hand. Prints `gset moved <k> files <f> keep <c>` then `gset ok` or `gset FAIL` (exit 1 on a keep line)."""
import pathlib
import re
import sys

KEEP_RULE = True  # the seeded test (3ds/tests/host.d/gset.sh) runs a copy with this False and expects a diff
DEF = re.compile(r"^\s*(?:const\s+)?Gfx\s+(\w+)\s*\[", re.M)
INC = re.compile(r'^(\s*#include\s+"([^"]+\.inc\.c)")\s*$', re.M)
DECL = re.compile(r"^(\s*)extern\s+Gfx\s+(\w+)\s*\[\s*\]\s*;")
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
    common = subprocess.check_output(["git", "-C", str(root), "rev-parse", "--path-format=absolute", "--git-common-dir"], text=True).strip()
    return pathlib.Path(common).parent / "assets/us"


def move_includes(root, src, ok, twin_dir, assets):
    """Returns (moved names, the twin path or None)."""
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
        out, changed, keep_here = [], False, 0
        for i, ln in enumerate(text.split("\n"), 1):
            if not shield.search(ln):
                out.append(ln)
                continue
            d = DECL.match(ln)
            if d and d.group(2) in names:
                out.append(f"{d.group(1)}NDN_DECL({d.group(2)});")
                changed = True
                continue
            hit = shield.findall(ln)
            bad = KEEP_RULE and ("BranchList" in ln or "sizeof" in ln or any(re.search(r"\b" + re.escape(n) + r"\s*\[|&\s*" + re.escape(n) + r"\b", ln) for n in hit))
            if bad:
                print(f"gset keep {rel}:{i}")
                keep_here += 1
                nkeep += 1
                out.append(ln)
                continue
            out.append(shield.sub(lambda m: f"NDN({m.group(1)})", ln))
            changed = True
        if not changed:
            continue
        if "nd_gtab.h" not in text:
            inc = re.search(r'^\s*#include\s+"[^"]+"', "\n".join(out), re.M)
            if inc:
                idx = "\n".join(out).find(inc.group(0)) + len(inc.group(0))
                joined = "\n".join(out)
                joined = joined[:idx] + '\n#include "nd_gtab.h"' + joined[idx:]
                out = joined.split("\n")
            else:
                out = ['#include "nd_gtab.h"'] + out
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
        moved, twin = move_includes(root, src, ok, "3ds/tools/gfxin", assets)
        if twin is not None:
            all_moved.extend(moved)
    nf, nkeep = rewrite_users(root, sorted(set(all_moved)))
    files_done = nf
    print(f"gset moved {len(set(all_moved))} files {files_done} keep {nkeep}")
    if nkeep:
        print("gset FAIL")
        return 1
    print("gset ok")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
