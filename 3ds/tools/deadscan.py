#!/usr/bin/env python3
"""DEADSCAN: list source functions with list calls that the 3DS ELF does not hold (dead code).

Usage: python 3ds/tools/deadscan.py <elf> [--src <dir>]

For every .c file under the source dir (default <repo>/src): strip comments, find each function definition (a column-0
line that is not a prototype: the next `{` comes before the next `;`), take its body to the matching `}`, and count the
active build-class calls in the body (listcen BUILD_RX; line_flags True = active on the 3DS). For each function with one
or more calls:
  elf   1 when a FUNC symbol of the ELF has the name or its base name (elfgate base_name), else 0.
  refs  number of \\bNAME\\b matches in all .c and .h files under the source dir and <repo>/include, outside the
        function's own definition line (raw text).
  dead  elf 0 and refs 0.
Absent (elf 0) is not dead by itself: GCC can inline a static function into a live caller.

Output per function: 'deadscan fn <file> <name> calls <n> elf <0|1> refs <r>'.
Last line: 'deadscan total funcs <n> calls <c> absent <a> absent_calls <ac> dead <d> dead_calls <dc>'. Exit 0.
"""
import bisect
import collections
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import elfgate  # noqa: E402
import listcen  # noqa: E402

ROOT = listcen.ROOT
FUNC_RX = re.compile(r'^[A-Za-z_][\w \*]*\b(\w+)\s*\(', re.M)
BRACE_RX = re.compile(r'[{}]')
BUILD_RX = re.compile(listcen.BUILD_RX)
WORD_RX = re.compile(r'\w+')
NEWLINE_RX = re.compile('\n')
SYM_FUNC = 2


def rel(p):
    try:
        return p.resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return p.as_posix()


def c_files(d):
    if not d.is_dir():
        return []
    return sorted(p for p in d.rglob('*.c') if p.is_file())


def find_defs(stripped):
    """Yield (name, name_pos, body_open, body_end) for each function definition in comment-stripped text."""
    n = len(stripped)
    for m in FUNC_RX.finditer(stripped):
        brace = stripped.find('{', m.end())
        if brace < 0:
            break
        semi = stripped.find(';', m.end())
        if 0 <= semi < brace:
            continue
        depth = 0
        end = n
        for bm in BRACE_RX.finditer(stripped, brace):
            depth += 1 if bm.group() == '{' else -1
            if depth == 0:
                end = bm.end()
                break
        yield m.group(1), m.start(1), brace, end


def source_defs(raw):
    """Yield (name, line_index, calls, definition_line_text) for each function with one or more active build calls."""
    stripped = listcen.strip_comments(raw)
    flags = listcen.line_flags(stripped)
    nl = [m.start() for m in NEWLINE_RX.finditer(stripped)]
    raw_lines = raw.split('\n')
    for name, name_pos, lo, hi in find_defs(stripped):
        calls = 0
        for m in BUILD_RX.finditer(stripped, lo, hi):
            if flags[bisect.bisect_left(nl, m.start())] is True:
                calls += 1
        if calls == 0:
            continue
        line = bisect.bisect_left(nl, name_pos)
        yield name, line, calls, raw_lines[line]


def scan(src, includes, func_names):
    """Return one record per function with calls >= 1 (dicts: file, name, calls, elf, refs, dead)."""
    src = Path(src)
    ref_paths = set()
    for d in [src, *includes]:
        if d.is_dir():
            for pat in ('*.c', '*.h'):
                ref_paths.update(p for p in d.rglob(pat) if p.is_file())
    total = collections.Counter()
    for p in sorted(ref_paths):
        total.update(WORD_RX.findall(p.read_text(encoding='utf-8', errors='replace')))
    recs = []
    for p in c_files(src):
        raw = p.read_text(encoding='utf-8', errors='replace')
        for name, line, calls, def_text in source_defs(raw):
            elf = 1 if name in func_names else 0
            own = collections.Counter(WORD_RX.findall(def_text))[name]
            refs = total[name] - own
            recs.append({'file': rel(p), 'name': name, 'calls': calls, 'elf': elf, 'refs': refs,
                         'dead': elf == 0 and refs == 0})
    return recs


def report(recs):
    for r in recs:
        print(f"deadscan fn {r['file']} {r['name']} calls {r['calls']} elf {r['elf']} refs {r['refs']}")
    absent = [r for r in recs if r['elf'] == 0]
    dead = [r for r in recs if r['dead']]
    print(f"deadscan total funcs {len(recs)} calls {sum(r['calls'] for r in recs)} "
          f"absent {len(absent)} absent_calls {sum(r['calls'] for r in absent)} "
          f"dead {len(dead)} dead_calls {sum(r['calls'] for r in dead)}")


def elf_func_names(elf_path):
    syms, _count = elfgate.parse_elf(elf_path)
    names = set()
    for name, _value, _size, typ, _sec in syms:
        if typ == SYM_FUNC:
            names.add(name)
            names.add(elfgate.base_name(name))
    return names


def main(argv):
    if not argv or argv[0].startswith('--'):
        raise SystemExit('usage: deadscan.py <elf> [--src <dir>]')
    elf_path = argv[0]
    src = ROOT / 'src'
    if len(argv) >= 3 and argv[1] == '--src':
        src = Path(argv[2])
    elif len(argv) != 1:
        raise SystemExit('usage: deadscan.py <elf> [--src <dir>]')
    recs = scan(src, [ROOT / 'include'], elf_func_names(elf_path))
    report(recs)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
