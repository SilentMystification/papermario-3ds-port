#!/usr/bin/env python3
"""BUILDO: inc_scan.py <out> <group>...: the source files a C compile can read, for a generator cache key (3ds/build.sh gen_shapes).
A group is "-I d1,d2,... src1,src2,..." (repo-relative dirs in gcc order and the sources). Writes <out>: the sorted repo-relative
paths of every source and every header that an #include of a listed file can name, recursively.
Superset rule (cache safety): every include line counts, in every #if branch; each name adds every existing candidate (the
including file's folder for "x", then every -I folder; #include_next the same), not only the first. So a header that a new file
would shadow, and a header made later, change the list. A name found in no folder is a system header (the image id covers it).
An include by a macro name cannot be scanned: exit 3 (the caller then keys on every header). Host self-test: 3ds/tests/host.d/incscan.sh."""
import os
import re
import sys

INC = re.compile(r'^[ \t]*#[ \t]*include(?:_next)?[ \t]*(.)(.*)$', re.M)


def rel(p):
    return os.path.relpath(p).replace("\\", "/")


def scan(dirs, srcs, seen):
    todo = list(srcs)
    for s in srcs:
        if not os.path.isfile(s):
            raise SystemExit("inc_scan: missing source %s" % s)
    while todo:
        f = os.path.normpath(todo.pop())
        r = rel(f)
        if r in seen:
            continue
        seen.add(r)
        with open(f, "rb") as h:
            text = h.read().decode("latin-1")
        text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
        for m in INC.finditer(text):
            q, rest = m.group(1), m.group(2)
            if q == '"':
                name, cands = rest.split('"', 1)[0], [os.path.dirname(f)] + dirs
            elif q == "<":
                name, cands = rest.split(">", 1)[0], list(dirs)
            else:
                print("inc_scan: include by macro in %s: %s" % (r, m.group(0).strip()), file=sys.stderr)
                sys.exit(3)
            for d in cands:
                p = os.path.normpath(os.path.join(d, name))
                if os.path.isfile(p):
                    todo.append(p)


def main(a):
    if len(a) < 3:
        print(__doc__, file=sys.stderr)
        return 2
    seen = set()
    for g in a[2:]:
        parts = g.split()
        if len(parts) != 3 or parts[0] != "-I":
            print("inc_scan: bad group %r" % g, file=sys.stderr)
            return 2
        scan([d for d in parts[1].split(",") if d], parts[2].split(","), seen)
    out = sorted(p for p in seen if not p.startswith("../"))
    with open(a[1] + ".tmp", "w", encoding="utf-8", newline="\n") as h:
        h.write("".join(p + "\n" for p in out))
    os.replace(a[1] + ".tmp", a[1])
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
