#!/usr/bin/env python3
"""capcmp.py: compare two console prof logs window by window, only in windows with equal frame content.

  python 3ds/tools/capcmp.py <log a> <log b>

Window = all `prof f<F> w<W> <tag> ...` lines of frame F. Tag = the first word; `core2` and `stepx` take the first 2 words.
Values come from pb.fields (prof_by_scene.py). Signature (the frame content): ndc model spr ent call trect st sw (not kb);
nd dr tri; the at line map, area, m, sp, b, e, pos0, pos. An a window with chg1 or without an ndc line is no candidate.
A b window without ndc prints `b f<F> no ndc` and does not count. Match: the a window with an equal signature and the same
F, else the first equal a window in file order. No match: the a window with the fewest differing keys (at most 4 keys shown).
Match line fields: ft (a b delta, fps line), wk and fps (fps line), gbi (ph line), idle pace gx (c0i), kgx gpul gpuk (c0k),
lgms (core2 lgms), cyc (pmu cyc). A pair is `<a> <b>`; a missing value prints `-`.
Last line: `capcmp matched <n>/<m> d ft mean <x>` (m = b windows with ndc; x over the matches, `-` when none).
Log a is the first file, log b the second file (the b windows are the ones that are checked).
"""
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import prof_by_scene as pb  # noqa: E402

TWO = ("core2", "stepx")
NDC_KEYS = ("model", "spr", "ent", "call", "trect", "st", "sw")
ND_KEYS = ("dr", "tri")
AT_KEYS = ("map", "area", "m", "sp", "b", "e", "pos0", "pos")
# (name in the match line, tag, field key)
MATCH = (("wk", "fps", "wk"), ("fps", "fps", "fps"), ("gbi", "ph", "gbi"), ("idle", "c0i", "idle"), ("pace", "c0i", "pace"),
         ("gx", "c0i", "gx"), ("kgx", "c0k", "kgx"), ("gpul", "c0k", "gpul"), ("gpuk", "c0k", "gpuk"),
         ("lgms", "core2 lgms", "lgms"), ("cyc", "pmu", "pmu_cyc"))


def windows(path):
    """frame -> {tag: (fields, line)}. Tags keep their own dict entry: ent of cls and ent of ndc never share one."""
    win = {}
    with open(path, errors="replace") as fh:
        for ln in fh:
            m = pb.PROF.search(ln)
            t = m.group(3).split() if m else []
            if not t:
                continue
            n = 2 if t[0] in TWO and len(t) > 1 else 1
            win.setdefault(int(m.group(1)), {})[" ".join(t[:n])] = (pb.fields(m.group(3)), ln)
    return win


def at_of(w):
    """The pb.AT match of the at line of a window, or None."""
    if "at" not in w:
        return None
    return pb.AT.search(w["at"][1])


def chg(w):
    m = at_of(w)
    return m is not None and m.group(9) == "1"


def pos(m, i):
    return " ".join(m.group(i, i + 1, i + 2)) if m.group(i) else "-"


def val(fields, key):
    v = fields.get(key)
    return "-" if v is None else str(v)


def sig(w):
    """List of (key, value) of the frame content, or None without an ndc line."""
    if "ndc" not in w:
        return None
    out = [(k, val(w["ndc"][0], k)) for k in NDC_KEYS]
    nd = w["nd"][0] if "nd" in w else {}
    out += [(k, val(nd, k)) for k in ND_KEYS]
    m = at_of(w)
    if m is None:
        out += [(k, "-") for k in AT_KEYS]
    else:
        out += [("map", m.group(2)), ("area", "%s:%s" % (m.group(3), m.group(4))), ("m", m.group(5)), ("sp", m.group(6)),
                ("b", m.group(7)), ("e", m.group(8)), ("pos0", pos(m, 10)), ("pos", pos(m, 13))]
    return out


def get(w, tag, key):
    f = w.get(tag)
    return f[0].get(key) if f else None


def f2(x):
    return "-" if x is None else "%.2f" % x


def compare(path_a, path_b):
    """The output lines of capcmp for log a and log b (the last line is the summary)."""
    A, B = windows(path_a), windows(path_b)
    cand = [(G, sg) for G, w in A.items() if not chg(w) for sg in [sig(w)] if sg is not None]
    out, n, m, deltas = [], 0, 0, []
    for F, w in B.items():
        s = sig(w)
        if s is None:
            out.append("b f%d no ndc" % F)
            continue
        m += 1
        eq = [G for G, sg in cand if sg == s]
        if eq:
            G = F if F in eq else eq[0]
            wa = A[G]
            fa, fb = get(wa, "fps", "ft"), get(w, "fps", "ft")
            d = None if fa is None or fb is None else fb - fa
            if d is not None:
                deltas.append(d)
            parts = ["b f%d = a f%d" % (F, G),
                     "ft %s %s %s" % (f2(fa), f2(fb), "-" if d is None else "%+.2f" % d)]
            for name, tag, key in MATCH:
                parts.append("%s %s %s" % (name, f2(get(wa, tag, key)), f2(get(w, tag, key))))
            out.append(" | ".join(parts))
            n += 1
        elif not cand:
            out.append("b f%d none: no a window" % F)
        else:
            near, best = None, None
            for G, sg in cand:
                diff = [(k, va, vb) for (k, va), (_, vb) in zip(sg, s) if va != vb]
                if best is None or len(diff) < len(best):
                    near, best = G, diff
            shown = " ".join("%s %s/%s" % x for x in best[:4])
            out.append("b f%d none: near a f%d diff %s" % (F, near, shown))
    mean = "%+.2f" % (sum(deltas) / len(deltas)) if deltas else "-"
    out.append("capcmp matched %d/%d d ft mean %s" % (n, m, mean))
    return out


def main(argv):
    if len(argv) != 2:
        sys.stderr.write("usage: python 3ds/tools/capcmp.py <log a> <log b>\n")
        return 2
    for ln in compare(argv[0], argv[1]):
        print(ln)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
