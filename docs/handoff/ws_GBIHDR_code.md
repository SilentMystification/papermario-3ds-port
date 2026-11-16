# ws_GBIHDR_code: the 4 new files of ws_GBIHDR (copy them exactly, LF line ends)
Designer to ask: oracle DL3. FACT: this tool, run by the oracle on main a0115f743c, printed `gbiprune dead 132 lines 778`; the fixture gave dead 1 and the seed dead 3.
## 3ds/tools/gbiprune.py
```python
#!/usr/bin/env python3
"""GBIHDR: delete the gbi macros that no file uses (include/PR/gbi.h, include/gbi_custom.h, include/PR/gs2dex.h, include/PR/gt.h).
Usage: gbiprune.py [--dry] [root]   (root: the repo root, default: the current directory)
A candidate is a macro whose name starts with gDP, gSP, gsDP, gsSP, gdDP or gdSP. It is live when its name is a token of a user file, or of a
header line that is not in a #define block, or of the body of a live macro (closure). A user file: every git-tracked .c .h .inc .s .cpp .sh .txt
.json .ld .mk file and Makefile (not docs/, not *.py, not the 4 headers, not 3ds/include/nd_nolist.h), every .c and .h under assets/us, and USER_EXTRA.
Prints `gbiprune dead <n> lines <l>`, one `gbiprune hdr <path> blocks <b> lines <l>` per header, then `gbiprune ok`. Without --dry it deletes the
dead #define blocks (each line ending kept). Exit 0."""
import pathlib
import re
import subprocess
import sys

HDR = ["include/PR/gbi.h", "include/gbi_custom.h", "include/PR/gs2dex.h", "include/PR/gt.h"]
SKIP = {"3ds/include/nd_nolist.h"}  # the NOLIST poison list names every macro; it is not a user
USER_EXTRA = []  # GBIHDR fix scope: a user file the census misses (repo-relative path)
DEFRX = re.compile(r"^[ \t]*#[ \t]*define[ \t]+(\w+)")
CAND = re.compile(r"^(g|gs|gd)[DS]P")
USER = re.compile(r"\.(c|h|py|sh|inc|cpp|s|txt|json|ld|mk)$|Makefile")
TOK = re.compile(r"\b[A-Za-z_]\w*\b")


def asset_root(root):
    if (root / "assets/us").is_dir():
        return root
    try:
        common = subprocess.check_output(["git", "-C", str(root), "rev-parse", "--path-format=absolute", "--git-common-dir"], text=True, stderr=subprocess.DEVNULL).strip()
    except (subprocess.CalledProcessError, OSError):
        return root
    return pathlib.Path(common).parent


def tracked(root):
    try:
        out = subprocess.check_output(["git", "-C", str(root), "ls-files"], text=True, stderr=subprocess.DEVNULL)
        return [f for f in out.split("\n") if f]
    except (subprocess.CalledProcessError, OSError):  # a fixture without git: every file under root
        return [p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file()]


def blocks(lines):
    """(name, first line, last line) of every #define block (backslash continuation)."""
    out, i = [], 0
    while i < len(lines):
        m = DEFRX.match(lines[i])
        if m:
            j = i
            while lines[j].rstrip("\r\n").rstrip().endswith(chr(92)) and j + 1 < len(lines):
                j += 1
            out.append((m.group(1), i, j))
            i = j + 1
        else:
            i += 1
    return out


def main(argv):
    dry = "--dry" in argv
    rest = [a for a in argv if a != "--dry"]
    root = pathlib.Path(rest[0]) if rest else pathlib.Path(".")
    text, defs = {}, {}
    for h in HDR:
        p = root / h
        if not p.is_file():
            continue
        text[h] = p.read_text(encoding="utf-8", errors="replace").splitlines(keepends=True)
        for n, i, j in blocks(text[h]):
            defs.setdefault(n, []).append((h, i, j))
    cand = {n for n in defs if CAND.match(n)}
    users = [f for f in tracked(root) if not f.startswith("docs/") and f not in HDR and f not in SKIP and not f.endswith(".py") and USER.search(f)]
    paths = [root / f for f in users + USER_EXTRA]
    ar = asset_root(root)
    paths += sorted(p for p in (ar / "assets/us").rglob("*") if p.suffix in (".c", ".h") and p.is_file())
    used = set()
    for p in paths:
        try:
            t = p.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        used.update(w for w in TOK.findall(t) if w in defs)
    for h, lines in text.items():
        inblock = {k for lst in defs.values() for hh, i, j in lst if hh == h for k in range(i, j + 1)}
        for k, ln in enumerate(lines):
            if k not in inblock:
                used.update(w for w in TOK.findall(ln) if w in defs)
    live = {n for n in defs if n in used or n not in cand}
    stack = list(live)
    while stack:
        n = stack.pop()
        for h, i, j in defs[n]:
            for w in TOK.findall("".join(text[h][i:j + 1])):
                if w in defs and w not in live:
                    live.add(w)
                    stack.append(w)
    dead = sorted(cand - live)
    total = 0
    report = []
    for h, lines in text.items():
        drop = {k for n in dead for hh, i, j in defs[n] if hh == h for k in range(i, j + 1)}
        nb = sum(1 for n in dead for hh, i, j in defs[n] if hh == h)
        total += len(drop)
        report.append(f"gbiprune hdr {h} blocks {nb} lines {len(drop)}")
        if not dry and drop:
            (root / h).write_text("".join(ln for k, ln in enumerate(lines) if k not in drop), encoding="utf-8", newline="")
    print(f"gbiprune dead {len(dead)} lines {total}")
    for r in report:
        print(r)
    print("gbiprune ok")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
```
## 3ds/tests/gbiprune/case/include/PR/gbi.h (5 lines; line 3 ends with a backslash)
```c
#define G_X 1
#define gsDPB(a) (a)
#define gDPA(pkt, a) \
    gsDPB(a)
#define gDPC(pkt) 0
```
## 3ds/tests/gbiprune/case/src/user.c
```c
int f(void) { return gDPA(0, G_X); }
```
## 3ds/tests/host.d/gbiprune.sh
```sh
#!/bin/sh
# GBIHDR: gbiprune.py on the fixture 3ds/tests/gbiprune/case. Real: only gDPC is dead (gsDPB lives through the body of gDPA).
# Seeded fault: without src/user.c, gDPA and gsDPB die too (dead 3 lines 4). Prints `gbiprune ok seed ok` or fails.
T=${GBIPRUNE_T:-/tmp/gbiprunecase}
PY=${PY:-python3}
rm -rf "$T"; mkdir -p "$T"; cp -r 3ds/tests/gbiprune/case/. "$T/"
$PY 3ds/tools/gbiprune.py "$T" > "$T.out" 2>&1 || { echo "gbiprune FAIL: exit"; cat "$T.out"; exit 1; }
grep -qx "gbiprune dead 1 lines 1" "$T.out" || { echo "gbiprune FAIL: real case"; cat "$T.out"; exit 1; }
if grep -q "gDPC" "$T/include/PR/gbi.h"; then echo "gbiprune FAIL: gDPC stays"; exit 1; fi
grep -q "gsDPB" "$T/include/PR/gbi.h" || { echo "gbiprune FAIL: gsDPB removed"; exit 1; }
rm -rf "$T"; mkdir -p "$T"; cp -r 3ds/tests/gbiprune/case/. "$T/"; rm "$T/src/user.c"
$PY 3ds/tools/gbiprune.py --dry "$T" > "$T.out" 2>&1
grep -qx "gbiprune dead 3 lines 4" "$T.out" || { echo "gbiprune FAIL: the seed is not seen"; cat "$T.out"; exit 1; }
echo "gbiprune ok seed ok"
```
