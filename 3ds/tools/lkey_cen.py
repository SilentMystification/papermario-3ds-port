"""LKEY census: the list word readers. NDN(x) (nd_gtab.h) and a moved list's v1 table give an NdNDev, not list units: a reader that walks such a
pointer reads the struct as words (the VCOUNT crash). Every file with a reader line must be in the allow file with its count and a verdict.

Usage: lkey_cen.py <allow.txt> [root]
Reader line (comments stripped): src/**/*.c|h and include/**/*.h (not src/effects/gfx/): a read of `words.w0`/`words.w1` (not an assignment
target) or a cast `(const uint32_t*)`/`(uint32_t*)`/`(u32*)` of a name with Gfx, gfx, dl, list, g, l or q in it; 3ds/src/*.c: the `words.w0`/`w1`
read or a read of `<name>->list`. allow.txt: `<path> <count> <verdict> <reason>`; verdict KEY (the file calls nd_lkey_ or nd_ntab_find), MAP (map
shape lists only), LOCAL (its own stack, heap or indexed array), REC (record words), HOLD (a list gset.py keeps: SetCustomGfx, BranchList, X[]),
OTHER (not a Gfx list). Prints `lkey census ok files <n> lines <m>` (exit 0) or one `lkey census bad <path> ...` line per fault and `lkey census FAIL` (exit 1)."""
import pathlib
import re
import sys

VERDICTS = ("KEY", "MAP", "LOCAL", "REC", "HOLD", "OTHER")
R_WORD = re.compile(r"words\s*\.\s*w[01]\b(?!\s*=[^=])")
R_CAST = re.compile(r"\((?:const\s+)?(?:uint32_t|u32)\s*\*\)\s*(?:\(const\s+void\s*\*\)\s*)?\(?\s*(?:\w+(?:->|\.))*\w*(?:[Gg]fx|[Dd]l|[Ll]ist|\bg\b|\bl\b|\bq\b)")
R_LIST = re.compile(r"\w+->list\b(?!\s*=[^=])")
R_KEY = re.compile(r"\bnd_lkey_\w+\s*\(|\bnd_ntab_find\s*\(")
R_COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)


def strip(text):
    return R_COMMENT.sub(lambda m: "\n" * m.group(0).count("\n"), text)


def count(rel, text):
    lines = strip(text).split("\n")
    if rel.startswith("3ds/src/"):
        return sum(1 for ln in lines if R_WORD.search(ln) or R_LIST.search(ln))
    return sum(1 for ln in lines if R_WORD.search(ln) or R_CAST.search(ln))


def census(root):
    out = {}
    pats = ("src/**/*.c", "src/**/*.h", "include/**/*.h", "3ds/src/*.c")
    for pat in pats:
        for f in sorted(root.glob(pat)):
            rel = f.relative_to(root).as_posix()
            if rel.startswith("src/effects/gfx/") or not f.is_file():
                continue
            text = f.read_text(encoding="utf-8", errors="replace")
            n = count(rel, text)
            if n:
                out[rel] = (n, text)
    return out


def main(argv):
    if len(argv) < 2:
        print(__doc__, file=sys.stderr)
        return 2
    root = pathlib.Path(argv[2]) if len(argv) > 2 else pathlib.Path(".")
    allow = {}
    for ln in pathlib.Path(argv[1]).read_text(encoding="utf-8").replace("\r", "").split("\n"):
        if not ln.strip() or ln.lstrip().startswith("#"):
            continue
        p = ln.split(None, 3)
        allow[p[0]] = (int(p[1]), p[2])
    got = census(root)
    bad = 0
    for rel, (n, text) in sorted(got.items()):
        if rel not in allow:
            print(f"lkey census bad {rel} {n} not listed"); bad += 1; continue
        an, v = allow[rel]
        if v not in VERDICTS:
            print(f"lkey census bad {rel} verdict {v}"); bad += 1
        if n != an:
            print(f"lkey census bad {rel} {n} listed {an}"); bad += 1
        if v == "KEY" and not R_KEY.search(strip(text)):
            print(f"lkey census bad {rel} KEY without nd_lkey_ or nd_ntab_find"); bad += 1
    for rel in sorted(set(allow) - set(got)):
        print(f"lkey census bad {rel} listed, no reader line"); bad += 1
    if bad:
        print("lkey census FAIL")
        return 1
    print(f"lkey census ok files {len(got)} lines {sum(n for n, _ in got.values())}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
