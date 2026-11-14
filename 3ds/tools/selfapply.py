#!/usr/bin/env python3
r"""selfapply.py [--dry] <ntab_tables.txt> <file or dir>...: the ND_SELF writer of FXSELF (ws_FXSELF.md, group 1).

tab = the lines of the tables file (the self-contained v2 tables, ntab_tables.txt). A dir argument means every *.c below it.
A line `^(\s*)nd_rec_dl\((.+)\);(.*)$` (the whole call on one line) whose names (gen_ntab_entries.names, of the file text)
are all in tab becomes `<indent>nd_draw_list(<arg>, ND_SELF);<rest>` (ND_SELF is the null entry: nd_rec_dl_e(l, NULL) is
nd_rec_dl(l), nd_gtab.c). Any other line that calls nd_rec_dl( is kept. Output, one line per candidate, in file order:
    selfapply site <file>:<line>
    selfapply keep <file>:<line> <names not in tab, comma-joined, or skip>
    selfapply total site <n> keep <k>
--dry prints only. Line endings (CRLF) and all other bytes are kept.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_ntab_entries import names  # noqa: E402

LINE = re.compile(r"^(\s*)nd_rec_dl\((.+)\);(.*)$")
CALL = re.compile(r"\bnd_rec_dl\(")
COMMENT = ("/*", "*", "//")


def c_files(paths):
    out = []
    for p in paths:
        if os.path.isdir(p):
            for root, dirs, files in os.walk(p):
                dirs.sort()
                for f in sorted(files):
                    if f.endswith(".c"):
                        out.append(os.path.join(root, f))
        else:
            out.append(p)
    return out


def main(argv):
    sys.stdout.reconfigure(newline="\n")  # LF on every host (the expected output a.log is LF)
    dry = False
    if argv and argv[0] == "--dry":
        dry = True
        argv = argv[1:]
    if len(argv) < 2:
        print("usage: selfapply.py [--dry] <ntab_tables.txt> <file or dir>...", file=sys.stderr)
        return 2
    with open(argv[0], encoding="latin-1", newline="") as fh:
        tab = set(ln.strip() for ln in fh.read().split("\n") if ln.strip())
    site = keep = 0
    for path in c_files(argv[1:]):
        name = path.replace("\\", "/")
        with open(path, encoding="latin-1", newline="") as fh:
            text = fh.read()
        lines = text.split("\n")
        changed = False
        for i, ln in enumerate(lines):
            if not CALL.search(ln):
                continue
            if ln.lstrip().startswith(COMMENT):
                continue
            where = "%s:%d" % (name, i + 1)
            m = LINE.match(ln)
            ns = names(text, m.group(2)) if m else None
            ok = ns is not None and len(ns) > 0 and all(n in tab for n in ns)
            if ok:
                lines[i] = "%snd_draw_list(%s, ND_SELF);%s" % (m.group(1), m.group(2), m.group(3))
                changed = True
                site += 1
                print("selfapply site " + where)
            else:
                keep += 1
                bad = [n for n in ns if n not in tab] if ns else []
                print("selfapply keep %s %s" % (where, ",".join(bad) if bad else "skip"))
        if changed and not dry:
            with open(path, "w", encoding="latin-1", newline="") as fh:
                fh.write("\n".join(lines))
    print("selfapply total site %d keep %d" % (site, keep))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
