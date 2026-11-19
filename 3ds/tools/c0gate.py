"""c0gate: end proof that core 0 runs no N64 state machine (plan_native.md section 2).

Usage:
  python 3ds/tools/c0gate.py <elf>              print call sites and symbols; exit 0
  python 3ds/tools/c0gate.py --selftest <elf>   seeded faults A and B; exit 0 or 1

The three numbers (end state: calls 0, interp 0, state 0):
  calls   interpreter call sites in 3ds/src/*.c and src/*.c
  interp  FUNC symbols in the ELF whose base name is an interpreter name
  state   OBJECT symbols in the ELF whose name is a state name (23 on main)

Column 0 rule: a line that starts at column 0 with a letter or "_" is a
definition line. Its first name is the defined function and is not a call.
Names come from 3ds/tools/c0gate_names.txt ("interp <name>", "state <name>").
"""
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import elfgate  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parents[2]
CALL = re.compile(r"\b(\w+)\s*\(")
CMT = re.compile(r"/\*.*?\*/", re.S)
FIXTURE = (
    "void nd_nr_run(const unsigned* r) { nd_nr_exec(r, 1u); }\n"
    "static void f(void) {\n"
    "    nd_comb_sync(); /* nd_rnd_sync() */\n"
    "}\n"
)


def names():
    c = {"interp": set(), "state": set()}
    text = (ROOT / "3ds/tools/c0gate_names.txt").read_text(encoding="utf-8")
    for ln in text.splitlines():
        if ln.strip() and not ln.startswith("#"):
            k, n = ln.split()
            c[k].add(n)
    return c["interp"], c["state"]


def src_calls(files, interp):
    """Call sites per file. A column 0 definition line does not count its first name."""
    per = {}
    for f in files:
        t = f.read_text(encoding="utf-8", errors="replace")
        if not any(n in t for n in interp):
            continue
        t = CMT.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), t)
        n = 0
        for line in t.splitlines():
            line = line.split("//")[0]
            hits = [m.group(1) for m in CALL.finditer(line) if m.group(1) != "__attribute__"]
            if line[:1].isalpha() or line[:1] == "_":
                hits = hits[1:]
            n += sum(1 for h in hits if h in interp)
        if n:
            per[f.relative_to(ROOT).as_posix()] = n
    return per


def elf_hits(elf, interp, state):
    syms, _ = elfgate.parse_elf(elf)
    b = elfgate.base_name
    funcs = sorted({b(s[0]) for s in syms if s[3] == 2 and b(s[0]) in interp})
    objs = sorted({s[0] for s in syms if s[3] == 1 and s[0] in state})
    return funcs, objs


def main(elf):
    interp, state = names()
    files = sorted((ROOT / "3ds/src").glob("*.c")) + sorted((ROOT / "src").rglob("*.c"))
    per = src_calls(files, interp)
    for path, n in sorted(per.items(), key=lambda kv: (-kv[1], kv[0]))[:40]:
        print(f"c0gate file {path} {n}")
    fn, ob = elf_hits(elf, interp, state)
    print("c0gate interp_syms " + " ".join(fn))
    print("c0gate state_syms " + " ".join(ob))
    print(f"c0gate calls {sum(per.values())} interp {len(fn)} state {len(ob)}")
    return 0


def selftest(elf):
    interp, state = names()
    fix_dir = ROOT / "build3ds/out/c0gate_self"
    fix_dir.mkdir(parents=True, exist_ok=True)
    fix = fix_dir / "fix.c"
    fix.write_text(FIXTURE, encoding="utf-8")
    base = elf_hits(elf, interp, state)[0]
    seeded = elf_hits(elf, interp | {"nd_stream_run"}, state)[0]
    a_ok = set(seeded) - set(base) == {"nd_stream_run"}
    b_ok = sum(src_calls([fix], interp).values()) == 2
    print("c0gate selftest A " + ("ok" if a_ok else "FAIL"))
    print("c0gate selftest B " + ("ok" if b_ok else "FAIL"))
    if a_ok and b_ok:
        print("c0gate selftest ok")
        return 0
    print("c0gate selftest FAIL")
    return 1


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--selftest":
        sys.exit(selftest(sys.argv[2]))
    if len(sys.argv) == 2:
        sys.exit(main(sys.argv[1]))
    print("usage: c0gate.py [--selftest] <elf>", file=sys.stderr)
    sys.exit(2)
