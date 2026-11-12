#!/usr/bin/env python3
"""listcen.py: NATIVEAPI end-state census. Counts display-list code left in the tree, per class and per file.

Classes: build (gDP/gSP calls in game code), gfxpos (gMainGfxPos uses), data (gsDP/gsSP calls in game code and assets),
data_arrays (static Gfx arrays), gfxtype (Gfx type uses), reader (list op / walker state readers in 3ds/src),
model (walker state model function definitions). Comments are stripped before matching.
Run from anywhere: python 3 standard library only. Exit 0. Output lines: 'listcen file <class> <path> <count>' (15 largest
per class), then 'listcen total <class> <n>'.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TOP = 15


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


def count_regex(paths, rx, multiline=False):
    res = {}
    flags = re.M if multiline else 0
    crx = re.compile(rx, flags)
    for p in paths:
        n = len(crx.findall(read_stripped(p)))
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
        lines = read_stripped(p).split('\n')
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


def main():
    src_c = list(files(['src/**/*.c']))
    inc_h = list(files(['include/**/*.h']))
    assets_c = list(files(['assets/**/*.c']))

    classes = []
    classes.append(('build', count_regex(src_c + inc_h, r'\bg[DS]P[A-Za-z0-9_]*\s*\(')))
    classes.append(('gfxpos', count_regex(src_c, r'\bgMainGfxPos\b')))
    classes.append(('data', count_regex(src_c + inc_h + assets_c, r'\bgs[DS]P[A-Za-z0-9_]*\s*\(')))
    classes.append(('data_arrays', count_regex(src_c + inc_h + assets_c,
                                               r'^\s*(static\s+)?Gfx\s+\w+\s*\[', multiline=True)))
    gfx_paths = list(files(['src/**/*.c', 'src/**/*.h', 'include/**/*.h', '3ds/src/**/*.c', '3ds/src/**/*.h']))
    classes.append(('gfxtype', count_regex(gfx_paths, r'\bGfx\b')))
    reader_paths = list(files(['3ds/src/*.c', '3ds/include/*.h']))
    classes.append(('reader', count_regex(reader_paths,
                                          r'\bNDO_[A-Z0-9_]+|\bG_(DL|VTX|TRI1|TRI2|TEXRECT|SETCOMBINE|SETOTHERMODE_[HL]|MTX|POPMTX)\b'
                                          r'|\bnd_ops_run\b|\bnd_walk_state\b|\bnd_rec_state\b|\bNDC_STATE\b|\bENT_GBI\b')))
    model_paths = list(files(['src/**/*.c', '3ds/src/**/*.c']))
    classes.append(('model', count_model(model_paths)))

    for name, res in classes:
        ranked = sorted(res.items(), key=lambda kv: (-kv[1], rel(kv[0])))[:TOP]
        for p, n in ranked:
            print('listcen file %s %s %d' % (name, rel(p), n))
    for name, res in classes:
        print('listcen total %s %d' % (name, sum(res.values())))
    return 0


if __name__ == '__main__':
    sys.exit(main())
