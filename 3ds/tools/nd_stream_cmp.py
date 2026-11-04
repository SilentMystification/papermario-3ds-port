#!/usr/bin/env python3
"""nd_stream_cmp.py: compare a native draw report with an ndref reference (plan_native_system 9.3, the parts the S0 decoder fills).

  nd_stream_cmp.py <ref.bin> <native.txt>     native.txt: nd_pica_decode output. Exit 0 equal, 1 differ.
  nd_stream_cmp.py --selftest                 identity passes, one changed texture word fails (no game files needed).

Compared per draw, in order: texture words of units 0 and 1 (rule 3), triangle count. Batches of the native report must not exceed the
reference (rule 2). Not compared yet: cull, blend, depth, alpha, combiner (PICA mapping comes with S1), resolved vertices (S1 vertex layout).
"""
import os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import nd_ref_dump as rd

LINE = re.compile(r"^draw (\d+) cull (\d+) tw0 ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+) tw1 ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+) ibuf [0-9a-f]+ tris (\d+)$")


def ref_draws(path):
    _, recs = rd.parse(path)
    return [r for r in recs if r["kind"] == rd.KIND_DRAW]


def native_draws(path):
    out = []
    for ln in open(path, encoding="ascii"):
        if not ln.strip():
            continue
        m = LINE.match(ln.strip())
        if not m:
            raise ValueError("bad line: " + ln.strip())
        g = m.groups()
        out.append({"tw0": [int(x, 16) for x in g[2:6]], "tw1": [int(x, 16) for x in g[6:10]], "tris": int(g[10])})
    return out


def compare(ref, nat):
    """Return None when equal, else a message naming the first differing draw and field."""
    if len(nat) > len(ref):
        return "batches: native %d above reference %d" % (len(nat), len(ref))
    for i, n in enumerate(nat):
        r = ref[i]
        for f, rv in (("tw0", r["tw0"]), ("tw1", r["tw1"]), ("tris", r["nv"] // 3)):
            nv = n[f]
            if (f == "tris" and nv != rv) or (f != "tris" and list(nv) != list(rv)):
                return "draw %d field %s: native %s reference %s" % (i, f, nv, rv)
    return None


def as_text(draws):
    lines = []
    for i, r in enumerate(draws):
        lines.append("draw %d cull 0 tw0 %s tw1 %s ibuf 00000000 tris %d" % (
            i, " ".join("%08x" % v for v in r["tw0"]), " ".join("%08x" % v for v in r["tw1"]), r["nv"] // 3))
    return "\n".join(lines) + "\n"


def selftest():
    import tempfile
    here = os.path.dirname(os.path.abspath(__file__))
    ref_path = os.path.join(here, "..", "tests", "ndref", "ndref_yard.bin")
    if not os.path.exists(ref_path):
        print("SELFTEST SKIP no reference file " + ref_path)
        return 1
    ref = ref_draws(ref_path)
    with tempfile.TemporaryDirectory() as d:
        p = os.path.join(d, "same.txt")
        open(p, "w").write(as_text(ref))
        same = compare(ref, native_draws(p))
        mut = [dict(r) for r in ref]
        mut[len(mut) // 2]["tw0"] = [mut[len(mut) // 2]["tw0"][0] ^ 1] + list(mut[len(mut) // 2]["tw0"][1:])
        q = os.path.join(d, "mut.txt")
        open(q, "w").write(as_text(mut))
        diff = compare(ref, native_draws(q))
    ok = same is None and diff is not None
    print("SELFTEST %s identity: %s; one texture word changed: %s" % ("PASS" if ok else "FAIL", same or "equal", diff or "equal (wrong)"))
    return 0 if ok else 1


def main(argv):
    if argv == ["--selftest"]:
        return selftest()
    if len(argv) != 2:
        print(__doc__)
        return 2
    msg = compare(ref_draws(argv[0]), native_draws(argv[1]))
    if msg is None:
        print("EQUAL %d draws" % len(native_draws(argv[1])))
        return 0
    print("DIFF " + msg)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
