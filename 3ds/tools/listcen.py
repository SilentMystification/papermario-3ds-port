#!/usr/bin/env python3
"""listcen.py: NATIVEAPI end-state census. Counts display-list code left in the tree, per class and per file.

Classes: build (gDP/gSP calls in game code), gfxpos (gMainGfxPos uses), data (gsDP/gsSP calls in game code and assets),
data_arrays (static Gfx arrays), gfxtype (Gfx type uses), recdl (nd_rec_dl / nd_scomp_rec calls in src), reader (list op /
walker state readers in 3ds/src), model (walker state model function definitions). Comments are stripped before matching.

Active versus N64-only: the classes build, gfxpos, data, data_arrays, gfxtype and recdl split every match by the TARGET_3DS
flag of its line. A line is active (compiled for the 3DS) when every open conditional branch is taken for TARGET_3DS. A line
inside `#ifndef TARGET_3DS`, `#if !defined(TARGET_3DS)`, or the `#else` of `#ifdef TARGET_3DS` is N64-only and counts in the
class's `_n64` twin. Other conditionals (`#if` with other tests, `#ifdef` of other names) keep the line active. The conditional
directives themselves (if/ifdef/ifndef/elif/else/endif) count as neither. Other `#` lines (define, include) take the flag of
their enclosing branches.

Groups: each file falls into at most one group. src/effects/gfx, src/effects, src/world, src/entity, src/battle, src/pause,
src/filemenu (longest prefix wins), include, assets/<dir>, and one group per root src file (src/<name>.c). Other files get no
group.

Run from anywhere: python 3 standard library only. Exit 0.
Output lines:
  'listcen group <group> <class> <n>' for each non-zero value,
  'listcen file <class> <path> <count>' (15 largest per class; reader and model print every file),
  'listcen total <class> <n>' for every class including the _n64 twins.
Option '--group <prefix>': prints 'listcen file <class> <path> <count>' for every non-zero file under the prefix, no limit.
"""
import bisect
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TOP = 15

SRC_GROUPS = ['src/effects/gfx', 'src/effects', 'src/world', 'src/entity', 'src/battle', 'src/pause', 'src/filemenu']

BUILD_RX = r'\bg[DS]P[A-Za-z0-9_]*\s*\('
GFXPOS_RX = r'\bgMainGfxPos\b'
DATA_RX = r'\bgs[DS]P[A-Za-z0-9_]*\s*\('
ARRAYS_RX = r'^\s*(static\s+)?(const\s+)?Gfx\s+[\w()]+\s*\['
GFXTYPE_RX = r'\bGfx\b'
RECDL_RX = r'\bnd_rec_dl\s*\(|\bnd_scomp_rec\s*\('
READER_RX = (r'\bNDO_[A-Z0-9_]+|\bG_(DL|VTX|TRI1|TRI2|TEXRECT|SETCOMBINE|SETOTHERMODE_[HL]|MTX|POPMTX)\b'
             r'|\bnd_ops_run\b|\bnd_walk_state\b|\bnd_rec_state\b|\bNDC_STATE\b|\bENT_GBI\b')

CONDITIONAL_WORDS = ('if', 'ifdef', 'ifndef', 'elif', 'else', 'endif')
DIRECTIVE_RX = re.compile(r'^\s*#\s*(\w+)\s*(.*?)\s*$')


def strip_comments(text):
    """Blank out C comments; keep newlines and string/char literals intact."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ''
        if c == '/' and nxt == '/':
            while i < n and text[i] != '\n':
                out.append(' ')
                i += 1
        elif c == '/' and nxt == '*':
            out.append('  ')
            i += 2
            while i < n and not (text[i] == '*' and i + 1 < n and text[i + 1] == '/'):
                out.append('\n' if text[i] == '\n' else ' ')
                i += 1
            if i < n:
                out.append('  ')
                i += 2
        elif c in '"\'':
            q = c
            out.append(c)
            i += 1
            while i < n and text[i] != '\n':
                ch = text[i]
                out.append(ch)
                i += 1
                if ch == chr(92) and i < n and text[i] != '\n':
                    out.append(text[i])
                    i += 1
                    continue
                if ch == q:
                    break
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def files(patterns):
    seen = set()
    for pat in patterns:
        for p in ROOT.glob(pat):
            if p.is_file() and p not in seen:
                seen.add(p)
                yield p


def read_stripped(p):
    return strip_comments(p.read_text(encoding='utf-8', errors='replace'))


def classify_if(word, rest):
    """Return [kind, taking] for an #if/#ifdef/#ifndef line. kind T = TARGET_3DS test, U = other test."""
    r = rest.strip()
    if word == 'ifdef':
        return ['T', True] if r == 'TARGET_3DS' else ['U', True]
    if word == 'ifndef':
        return ['T', False] if r == 'TARGET_3DS' else ['U', True]
    if re.fullmatch(r'defined\s*\(\s*TARGET_3DS\s*\)', r):
        return ['T', True]
    if re.fullmatch(r'!\s*defined\s*\(\s*TARGET_3DS\s*\)', r):
        return ['T', False]
    return ['U', True]


def line_flags(text):
    """Per line of comment-stripped text: True active, False N64-only, None conditional directive line."""
    stack = []
    flags = []
    for line in text.split('\n'):
        m = DIRECTIVE_RX.match(line)
        if m and m.group(1) in CONDITIONAL_WORDS:
            word, rest = m.group(1), m.group(2)
            if word in ('if', 'ifdef', 'ifndef'):
                stack.append(classify_if(word, rest))
            elif word == 'else':
                if stack and stack[-1][0] == 'T':
                    stack[-1][1] = not stack[-1][1]
            elif word == 'elif':
                if stack:
                    stack[-1] = ['U', True]
            elif word == 'endif':
                if stack:
                    stack.pop()
            flags.append(None)
            continue
        flags.append(all(taking for _, taking in stack))
    return flags


_INFO = {}


def info(p):
    """(stripped text, per-line flags, newline offsets) for one file, read once."""
    if p not in _INFO:
        text = read_stripped(p)
        _INFO[p] = (text, line_flags(text), [m.start() for m in re.finditer('\n', text)])
    return _INFO[p]


def count_split(paths, rx):
    """Return (active, n64) dicts {path: n}. Each match counts on the line it starts on."""
    crx = re.compile(rx, re.M)
    act, inact = {}, {}
    for p in paths:
        text, flags, nl = info(p)
        a = b = 0
        for m in crx.finditer(text):
            f = flags[bisect.bisect_left(nl, m.start())]
            if f is True:
                a += 1
            elif f is False:
                b += 1
        if a:
            act[p] = a
        if b:
            inact[p] = b
    return act, inact


def count_regex(paths, rx):
    res = {}
    crx = re.compile(rx, re.M)
    for p in paths:
        n = len(crx.findall(info(p)[0]))
        if n:
            res[p] = n
    return res


MODEL_NAMES = ['sync_combine', 'nd_comb_decide', 'decide', 'decide_miss', 'sync_render', 'set_tilesize', 'tex_win0',
               'tex_win1', 'ensure_tile', 'ensure_tile_slow', 'nd_texrect', 'nd_fillrect', 'gv_tri', 'op_setcombine',
               'load_mtx']


def count_model(paths):
    res = {}
    name_rx = re.compile(r'^\w[^;]*\b(' + '|'.join(MODEL_NAMES) + r')\s*\(')
    for p in paths:
        lines = info(p)[0].split('\n')
        n = 0
        for i, line in enumerate(lines):
            if not name_rx.search(line):
                continue
            if '{' in line[name_rx.search(line).end():]:
                n += 1
                continue
            j = i + 1
            while j < len(lines) and not lines[j].strip():
                j += 1
            if j < len(lines) and lines[j].lstrip().startswith('{'):
                n += 1
        if n:
            res[p] = n
    return res


def rel(p):
    return p.relative_to(ROOT).as_posix()


def group_of(r):
    if r.startswith('include/'):
        return 'include'
    if r.startswith('assets/'):
        parts = r.split('/')
        return 'assets/' + parts[1] if len(parts) > 2 else 'assets'
    if r.startswith('src/'):
        for g in SRC_GROUPS:
            if r.startswith(g + '/'):
                return g
        if r.count('/') == 1:
            return r
    return None


def main(argv):
    prefix = None
    if '--group' in argv:
        prefix = argv[argv.index('--group') + 1].rstrip('/')

    src_c = list(files(['src/**/*.c']))
    src_ch = list(files(['src/**/*.c', 'src/**/*.h']))
    inc_h = list(files(['include/**/*.h']))
    assets_c = list(files(['assets/**/*.c']))

    # (name, file set, twin): twin classes print the active and _n64 dicts.
    split = []
    split.append(('build', *count_split(src_c + inc_h, BUILD_RX)))
    split.append(('gfxpos', *count_split(src_c, GFXPOS_RX)))
    split.append(('data', *count_split(src_c + inc_h + assets_c, DATA_RX)))
    split.append(('data_arrays', *count_split(src_c + inc_h + assets_c, ARRAYS_RX)))
    gfx_paths = list(files(['src/**/*.c', 'src/**/*.h', 'include/**/*.h', '3ds/src/**/*.c', '3ds/src/**/*.h']))
    split.append(('gfxtype', *count_split(gfx_paths, GFXTYPE_RX)))
    split.append(('recdl', *count_split(src_ch, RECDL_RX)))

    plain = []
    reader_paths = list(files(['3ds/src/*.c', '3ds/include/*.h']))
    plain.append(('reader', count_regex(reader_paths, READER_RX)))
    model_paths = list(files(['src/**/*.c', '3ds/src/**/*.c']))
    plain.append(('model', count_model(model_paths)))

    classes = []
    for name, act, inact in split:
        classes.append((name, act))
        classes.append((name + '_n64', inact))
    classes.extend(plain)

    if prefix is not None:
        for name, res in classes:
            under = [(p, n) for p, n in res.items() if rel(p) == prefix or rel(p).startswith(prefix + '/')]
            for p, n in sorted(under, key=lambda kv: (-kv[1], rel(kv[0]))):
                print('listcen file %s %s %d' % (name, rel(p), n))
        return 0

    groups = {}
    for name, res in classes:
        for p, n in res.items():
            g = group_of(rel(p))
            if g is not None:
                groups.setdefault(g, {}).setdefault(name, 0)
                groups[g][name] += n
    order = [name for name, _ in classes]
    for g in sorted(groups):
        for name in order:
            if name in groups[g]:
                print('listcen group %s %s %d' % (g, name, groups[g][name]))

    for name, res in classes:
        is_plain = name in ('reader', 'model')
        ranked = sorted(res.items(), key=lambda kv: (-kv[1], rel(kv[0])))
        if not is_plain:
            ranked = ranked[:TOP]
        for p, n in ranked:
            print('listcen file %s %s %d' % (name, rel(p), n))
    for name, res in classes:
        print('listcen total %s %d' % (name, sum(res.values())))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
