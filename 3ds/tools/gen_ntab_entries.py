#!/usr/bin/env python3
"""gen_ntab_entries.py <dir> [<file>[=<NAMESPACE>] ...]: the device entry writer of CONV2C2 (ws_CONV2C2.md, group 1).

For every nd_draw_list(<arg>, <desc>) site of <dir>/*.c (sorted; <desc> not ND_SELF) the list names of <arg> (names()) paired with
the 20 tokens of the line ND_PSO_DESC(<desc>, ...); of the same file. One output line per distinct (list, desc) of a site:
    <list> <desc> <file>:<line> <20 tokens>
A site that does not resolve (an arg form names() does not know, or no ND_PSO_DESC line of the desc) prints:
    gne skip <file>:<line>
Module form: names(text, arg) returns the list names (a list, or None for a skip); ws_FXSELF imports it.
ENTSRC: after the files of <dir>, each extra <file> (gen_gfx_tab.sh passes the lines of 3ds/tools/ntab_ent.d/*.txt) is scanned the same
way. With =<NAMESPACE>, N(x) in an argument is <NAMESPACE>_x (the gen_gfx_syms.py name). ENTITY_ADDR(e, Gfx*, x) or ENTITY_ADDR(e, NdLst, x) is x (include/entity.h:66).
"""
import os
import re
import sys

CALL = re.compile(r"\bnd_draw_list\((.+),\s*(\w+)\);")
DESC = re.compile(r"^ND_PSO_DESC\((\w+), (.+)\);")
ARRAY = re.compile(r"^(\w+)\[.*\]$")


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def _uniq(vals):
    out = []
    for v in vals:
        if v not in out:
            out.append(v)
    return out


def _outer(s):
    # strip parentheses that wrap the whole text (not "(a) ? b : c")
    while len(s) >= 2 and s[0] == "(" and s[-1] == ")":
        depth = 0
        for i, c in enumerate(s):
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
                if depth == 0 and i != len(s) - 1:
                    return s
        s = s[1:-1].strip()
    return s


def _init(text, name):
    # NAME[..]... = { ... }; of the same file: the elements, designators dropped
    m = re.search(r"\b%s\s*(?:\[[^\]]*\]\s*)+=\s*\{(.*?)\};" % re.escape(name), text, re.S)
    if not m:
        return None
    body = m.group(1)
    body = re.sub(r"\[[^\]]*\]", "", body)  # designators
    vals = []
    for tok in re.split(r"[,{}]", body):
        tok = tok.strip()
        if not tok:
            continue
        tok = re.sub(r"^NDN\((\w+)\)$", r"\1", tok)  # GSETB: a moved list is written NDN(X) (gset.py)
        if not re.fullmatch(r"\w+", tok):
            return None
        if tok in ("NULL", "nullptr", "0"):
            continue
        vals.append(tok)
    return _uniq(vals)


def _names(text, arg, local, ns=None):
    s = _outer(arg.strip())
    if ns:  # ENTSRC namespace
        s = re.sub(r"\bN\((\w+)\)", lambda m: ns + "_" + m.group(1), s)
    me = re.fullmatch(r"ENTITY_ADDR\(\s*\w+\s*,\s*(?:Gfx\s*\*|NdLst)\s*,\s*(.+)\)", s)  # ENTSRC entity
    if me:
        return _names(text, me.group(1), local, ns)
    s = re.sub(r"^NDN\((\w+)\)$", r"\1", s)  # GSETB: a moved list is written NDN(X) (gset.py)
    if "?" in s:
        rest = s.split("?", 1)[1]
        if ":" not in rest:
            return None
        a, b = rest.split(":", 1)
        na = _names(text, a, local, ns)
        nb = _names(text, b, local, ns)
        if na is None or nb is None:
            return None
        return _uniq(na + nb)
    if ARRAY.match(s):
        return _init(text, ARRAY.match(s).group(1))
    if re.fullmatch(r"\w+", s):
        if local:
            vals = []
            found = False
            for mm in re.finditer(r"\b%s\s*=\s*([^;=][^;]*);" % re.escape(s), text):
                found = True
                r = _names(text, mm.group(1).strip(), False, ns)
                if r is None:
                    return None
                vals += r
            if found:
                return _uniq(vals)
        return [s]
    return None


def names(text, arg, ns=None):
    return _names(strip_comments(text), arg, True, ns)


def scan(path, ns, seen):
    with open(path, encoding="utf-8", errors="replace") as fh:
        text = fh.read()
    lines = [l.rstrip("\r") for l in text.split("\n")]
    descs = {}
    for l in lines:
        m = DESC.match(l)
        if m and m.group(1) not in descs:
            descs[m.group(1)] = [t.strip() for t in m.group(2).split(",")]
    for i, l in enumerate(lines, 1):
        m = CALL.search(l)
        if not m or m.group(2) == "ND_SELF":
            continue
        where = "%s:%d" % (path, i)
        desc = m.group(2)
        if desc not in descs or len(descs[desc]) != 20:
            print("gne skip " + where)
            continue
        ns_ = names(text, m.group(1), ns)
        if ns_ is None:
            print("gne skip " + where)
            continue
        for n in ns_:
            if (n, desc) in seen:
                continue
            seen.add((n, desc))
            print(" ".join([n, desc, where] + descs[desc]))


def main(argv):
    if len(argv) < 2:
        print("usage: gen_ntab_entries.py <dir> [<file>[=<NAMESPACE>] ...]", file=sys.stderr)
        return 2
    sys.stdout.reconfigure(newline="\n")  # LF on every host (the expected output a.txt is LF)
    d = argv[1].rstrip("/")
    seen = set()
    for f in sorted(os.listdir(d)):
        if f.endswith(".c"):
            scan(d + "/" + f, None, seen)
    for x in argv[2:]:  # ENTSRC: the extra files, in argument order
        path, _, ns = x.partition("=")
        scan(path, ns or None, seen)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
