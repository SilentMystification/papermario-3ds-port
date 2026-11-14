#!/usr/bin/env python3
r"""ENTRYFX E2: apply entcap walker descriptions to the effect sources (nd_rec_dl site -> nd_draw_list with a ND_PSO_DESC).

Usage: entapply.py [--dry] <entcap.py output>...   (run from the repo root; site paths are relative to it)
A log line counts when it ends with 11 hex words. Its site is (src/effects/\w+\.c):(\d+) in the line; no site: counts as other.
Per site: the set of distinct 11-word tuples.
  two or more tuples: `entapply multi <file>:<line> <n>`, no edit.
  site line not `^(\s*)nd_rec_dl\((.+)\);\s*$`: `entapply skip <file>:<line>`, no edit.
  file without the line `#include "nd_gtab.h"`: `entapply skip <file> include` (once), no edit.
  else name pe_<file stem>_<line>; the line becomes `<indent>nd_draw_list(<arg>, <name>);`; `entapply site <file>:<line> <name>`.
  Desc: ND_PSO_DESC(<name>, w0..w7 as 0x%08x, 12 bytes of w8, w9, w10 little endian decimal). Per file: the desc lines follow the include line.
Last line: `entapply sites <n> multi <m> skip <s> other <o> files <f>` (n = applied sites, f = files with an applied site).
--dry writes no file.
"""
import os
import re
import sys

WORDS = re.compile(r"((?: [0-9a-f]{8}){11})\s*$")
SITE = re.compile(r"(src/effects/\w+\.c):(\d+)")
NDREC = re.compile(r"^(\s*)nd_rec_dl\((.+)\);\s*$")
INC = '#include "nd_gtab.h"'


def desc_line(name, words, cr):
    w = [int(x, 16) for x in words]
    head = ", ".join("0x%08x" % x for x in w[:8])
    tail = ", ".join(str((x >> (8 * k)) & 0xff) for x in w[8:11] for k in range(4))
    return "ND_PSO_DESC(%s, %s, %s);%s" % (name, head, tail, cr)


def main(argv):
    dry = "--dry" in argv
    paths = [a for a in argv if a != "--dry"]
    sites = {}
    other = 0
    for p in paths:
        with open(p, encoding="utf-8", errors="replace") as fh:
            for raw in fh.read().split("\n"):
                m = WORDS.search(raw)
                if not m:
                    continue
                words = tuple(m.group(1).split())
                s = SITE.search(raw)
                if not s:
                    other += 1
                    continue
                sites.setdefault((s.group(1), int(s.group(2))), set()).add(words)

    by_file = {}
    for (f, ln), tups in sorted(sites.items()):
        by_file.setdefault(f, []).append((ln, tups))

    applied = multi = skip = files = 0
    for f in sorted(by_file):
        ok = os.path.isfile(f)
        lines = []
        cr = ""
        inc_idx = None
        if ok:
            with open(f, encoding="utf-8", newline="") as fh:
                lines = fh.read().split("\n")
            inc_idx = next((i for i, l in enumerate(lines) if l.rstrip("\r") == INC), None)
            if inc_idx is not None:
                cr = "\r" if lines[inc_idx].endswith("\r") else ""
        if ok and inc_idx is None:
            print("entapply skip %s include" % f)
        stem = os.path.splitext(os.path.basename(f))[0]
        descs = []
        for ln, tups in by_file[f]:
            if len(tups) >= 2:
                print("entapply multi %s:%d %d" % (f, ln, len(tups)))
                multi += 1
                continue
            if not ok or inc_idx is None or ln < 1 or ln > len(lines):
                skip += 1
                continue
            text = lines[ln - 1]
            scr = "\r" if text.endswith("\r") else ""
            m = NDREC.match(text[:-1] if scr else text)
            if not m:
                print("entapply skip %s:%d" % (f, ln))
                skip += 1
                continue
            name = "pe_%s_%d" % (stem, ln)
            lines[ln - 1] = "%snd_draw_list(%s, %s);%s" % (m.group(1), m.group(2), name, scr)
            descs.append(desc_line(name, next(iter(tups)), cr))
            print("entapply site %s:%d %s" % (f, ln, name))
            applied += 1
        if descs:
            files += 1
            lines[inc_idx + 1:inc_idx + 1] = descs
            if not dry:
                with open(f, "w", encoding="utf-8", newline="") as fh:
                    fh.write("\n".join(lines))
    print("entapply sites %d multi %d skip %d other %d files %d" % (applied, multi, skip, other, files))
    return 0


if __name__ == "__main__":
    sys.stdout.reconfigure(newline="\n")
    sys.exit(main(sys.argv[1:]))
