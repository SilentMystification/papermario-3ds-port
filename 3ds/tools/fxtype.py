#!/usr/bin/env python3
"""fxtype.py: FXTYPE list handle codemod (ws_FXTYPE.md). In each given file, a pointer array `Gfx* X[..] = {` or `static Gfx* X[..] = {`
whose every other use in the file is `nd_draw_list(X[`, `gSPDisplayList(gMainGfxPos++, X[` or `nd_rec_dl(X[` gets the type NdLst
(nd_gtab.h: typedef const void* NdLst). Any other array stays. A changed file without `#include "nd_gtab.h"` gets it after the last
#include line before its first array.
Usage: fxtype.py [--dry] <file>...  Prints `fxtype file <path> ok <k> other <o>` per file, then `fxtype arrays ok <k> other <o> files <f>`."""
import re
import sys

USE_RULE = True  # FXTYPE: an array with another use stays; host.d/fxtype.sh runs a copy with False (seeded fault)
DECL = re.compile(r"^(static )?Gfx\* (\w+)((?:\[\w*\])+) = \{", re.M)
OKUSE = (r"nd_draw_list\(\s*{n}\[", r"gSPDisplayList\(\s*gMainGfxPos\+\+,\s*{n}\[", r"nd_rec_dl\(\s*{n}\[")
INCL = re.compile(r"^#include .*$", re.M)
NEWDECL = re.compile(r"^(static )?NdLst \w+", re.M)


def handle_ok(t, m):
    n = re.escape(m.group(2))
    uses = [u for u in re.finditer(r"\b%s\b" % n, t) if u.start() != m.start(2)]
    if not uses:
        return False
    if not USE_RULE:
        return True
    for u in uses:
        ls = t.rfind("\n", 0, u.start()) + 1
        seg = t[max(ls, u.start() - 60):u.end() + 1]
        if not any(re.search(p.format(n=n), seg) for p in OKUSE):
            return False
    return True


def main(argv):
    dry = "--dry" in argv
    files = [a for a in argv if a != "--dry"]
    tok, tot, nf = 0, 0, 0
    for f in files:
        with open(f, encoding="utf-8", newline="") as fh:
            t = fh.read()
        ms = list(DECL.finditer(t))
        ok = [m for m in ms if handle_ok(t, m)]
        print(f"fxtype file {f} ok {len(ok)} other {len(ms) - len(ok)}")
        tok += len(ok)
        tot += len(ms)
        if not ok:
            continue
        nf += 1
        if dry:
            continue
        for m in reversed(ok):
            t = t[:m.start()] + (m.group(1) or "") + "NdLst " + t[m.start(2):]
        if not re.search(r'^#include "nd_gtab.h"', t, re.M):
            nl = "\r\n" if "\r\n" in t else "\n"
            first = NEWDECL.search(t).start()
            prev = [i for i in INCL.finditer(t) if i.start() < first]
            at = t.index("\n", prev[-1].start()) + 1 if prev else 0
            t = t[:at] + '#include "nd_gtab.h"' + nl + t[at:]
        with open(f, "w", encoding="utf-8", newline="") as fh:
            fh.write(t)
    print(f"fxtype arrays ok {tok} other {tot - tok} files {nf}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
