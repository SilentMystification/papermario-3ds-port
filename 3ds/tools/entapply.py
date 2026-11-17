#!/usr/bin/env python3
r"""ENTRYFX E2: apply entcap walker descriptions to the effect sources (nd_rec_dl site -> nd_draw_list with a ND_PSO_DESC).

Usage: entapply.py [--dry] <entcap.py output>...   (run from the repo root; site paths are relative to it)
A log line with 11 hex words counts as an entry. Its site is a src/ path (ENTSRC: any directory below src, a .c or .inc.c file):(\d+)
in the line; no site: counts as other.
A site may carry a key `#<k>` (FXENT3: nd_rec_dl_k sites, entcap.py prints the key on the loc).
A line `so <list> <fn> <file:line>` (FXENT3: a state-only list, no words) marks its site as so.
Per site: the set of distinct 11-word tuples per key (unkeyed sites: key None).
  so and entry lines at one site: `entapply mixed <file>:<line>`, no edit.
  so lines only, unkeyed: the line `nd_rec_dl(<arg>);` -> `nd_draw_list(<arg>, ND_SELF);`; `entapply so <file>:<line>`.
  so lines only, keyed: `entapply kso <file>:<line>`, no edit.
  unkeyed, two or more tuples: `entapply multi <file>:<line> <n>`, no edit.
  unkeyed, one tuple: site line not `^(\s*)nd_rec_dl\((.+)\);\s*$`: `entapply skip <file>:<line>`, no edit.
    else name pe_<file stem>_<line>; the line becomes `<indent>nd_draw_list(<arg>, <name>);`; `entapply site <file>:<line> <name>`.
  keyed (nd_rec_dl_k(<arg>, <K>, <N>);): every key 0..N-1 has exactly one tuple, else `entapply kmiss <file>:<line> <keys>`
    (a missing key, or a key >= N) or `entapply multi <file>:<line>#<k> <n>`, no edit. Else the line becomes a switch on <K>:
    `switch (<K>) {`, `    case k: nd_draw_list(<arg>, pe_<stem>_<line>_k<k>); break;` for k < N-1, `    default: ...k<N-1>); break;`, `}`;
    `entapply keyed <file>:<line> <N>`.
  Desc: ND_PSO_DESC(<name>, w0..w7 as 0x%08x, 12 bytes of w8, w9, w10 little endian decimal). Per file: the desc lines follow the include line.
  file without the line `#include "nd_gtab.h"`: `entapply skip <file> include` (once), no edit.
Last line: `entapply sites <n> keyed <k> so <s> multi <m> kmiss <q> mixed <x> skip <sk> other <o> files <f>`
(n = applied sites, all kinds; k keyed; s so; f = files with an applied edit).
--dry writes no file.
"""
import os
import re
import sys

WORDS = re.compile(r"((?: [0-9a-f]{8}){11})\s*$")
SITE = re.compile(r"(?<=\s)(?:/[\w.]+/)?(src/(?:\w+/)*\w+(?:\.inc)?\.c):(\d+)(?: \(discriminator \d+\))?(?:#(\d+))?")  # ENTSRC site; ENTCAPD: the container root (/src/, /w/) is skipped, so /src/src/x.c -> src/x.c and 3ds/src/x.c is other
NDREC = re.compile(r"^(\s*)nd_rec_dl\((.+)\);\s*$")
NDKEY = re.compile(r"^(\s*)nd_rec_dl_k\((.+), (.+), (\d+)\);\s*$")
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
                is_so = raw.startswith("so ")
                if not m and not is_so:
                    continue
                s = SITE.search(raw)
                if not s:
                    other += 1
                    continue
                key = int(s.group(3)) if s.group(3) is not None else None
                ent = sites.setdefault((s.group(1), int(s.group(2))), {"l": [], "so": []})
                if is_so:
                    ent["so"].append(key)
                else:
                    ent["l"].append((key, tuple(m.group(1).split())))

    by_file = {}
    for (f, ln), ent in sorted(sites.items()):
        by_file.setdefault(f, []).append((ln, ent))

    applied = keyed = so_n = multi = kmiss = mixed = skip = files = 0
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
        edits = []  # (line index, replacement lines), applied in descending order
        for ln, ent in by_file[f]:
            l_list, so_list = ent["l"], ent["so"]
            tag = "%s:%d" % (f, ln)
            if l_list and so_list:
                print("entapply mixed %s" % tag)
                mixed += 1
                continue
            if not ok or inc_idx is None or ln < 1 or ln > len(lines):
                skip += 1
                continue
            text = lines[ln - 1]
            scr = "\r" if text.endswith("\r") else ""
            body = text[:-1] if scr else text
            if so_list:
                if any(k is not None for k in so_list):
                    print("entapply kso %s" % tag)
                    skip += 1
                    continue
                m = NDREC.match(body)
                if not m:
                    print("entapply skip %s" % tag)
                    skip += 1
                    continue
                edits.append((ln - 1, ["%snd_draw_list(%s, ND_SELF);%s" % (m.group(1), m.group(2), scr)]))
                print("entapply so %s" % tag)
                so_n += 1
                applied += 1
                continue
            keys = set(k for k, _ in l_list)
            if None in keys and len(keys) > 1:
                print("entapply mixed %s" % tag)
                mixed += 1
                continue
            if None not in keys:
                # FXENT3 keyed site
                m = NDKEY.match(body)
                if not m:
                    print("entapply skip %s" % tag)
                    skip += 1
                    continue
                n = int(m.group(4))
                per_key = {}
                for k, words in l_list:
                    per_key.setdefault(k, set()).add(words)
                bad = None
                for k, ws in sorted(per_key.items()):
                    if len(ws) >= 2:
                        bad = "entapply multi %s#%d %d" % (tag, k, len(ws))
                        break
                if bad:
                    print(bad)
                    multi += 1
                    continue
                missing = [k for k in range(n) if k not in per_key] + [k for k in per_key if k >= n]
                if missing:
                    print("entapply kmiss %s %s" % (tag, ",".join(str(k) for k in sorted(missing))))
                    kmiss += 1
                    continue
                ind, arg, kexpr = m.group(1), m.group(2), m.group(3)
                nl = [ind + "switch (%s) {" % kexpr]
                for k in range(n - 1):
                    nl.append("%s    case %d: nd_draw_list(%s, pe_%s_%d_k%d); break;" % (ind, k, arg, stem, ln, k))
                nl.append("%s    default: nd_draw_list(%s, pe_%s_%d_k%d); break;" % (ind, arg, stem, ln, n - 1))
                nl.append(ind + "}")
                for k in range(n):
                    descs.append(desc_line("pe_%s_%d_k%d" % (stem, ln, k), next(iter(per_key[k])), cr))
                edits.append((ln - 1, [x + scr for x in nl]))
                print("entapply keyed %s %d" % (tag, n))
                keyed += 1
                applied += 1
                continue
            tups = set(ws for _, ws in l_list)
            if len(tups) >= 2:
                print("entapply multi %s %d" % (tag, len(tups)))
                multi += 1
                continue
            m = NDREC.match(body)
            if not m:
                print("entapply skip %s" % tag)
                skip += 1
                continue
            name = "pe_%s_%d" % (stem, ln)
            edits.append((ln - 1, ["%snd_draw_list(%s, %s);%s" % (m.group(1), m.group(2), name, scr)]))
            descs.append(desc_line(name, next(iter(tups)), cr))
            print("entapply site %s %s" % (tag, name))
            applied += 1
        if edits:
            files += 1
            for idx, nl in sorted(edits, reverse=True):
                lines[idx:idx + 1] = nl
        if descs or edits:
            if descs:
                lines[inc_idx + 1:inc_idx + 1] = descs
            if not dry:
                with open(f, "w", encoding="utf-8", newline="") as fh:
                    fh.write("\n".join(lines))
    print("entapply sites %d keyed %d so %d multi %d kmiss %d mixed %d skip %d other %d files %d"
          % (applied, keyed, so_n, multi, kmiss, mixed, skip, other, files))
    return 0


if __name__ == "__main__":
    sys.stdout.reconfigure(newline="\n")
    sys.exit(main(sys.argv[1:]))
