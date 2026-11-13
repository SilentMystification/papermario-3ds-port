#!/usr/bin/env python3
"""unifdef3ds.py <file>...: remove the N64-only branches of TARGET_3DS blocks (row N64DEL1).

Line based, in place, per file. Comments are not directives.
- `#ifdef TARGET_3DS` or `#if defined(TARGET_3DS)` (no && or ||): keep the if part, drop the #else part.
- `#ifndef TARGET_3DS` or `#if !defined(TARGET_3DS)`: drop the if part, keep the #else part.
- The directive lines of those blocks are dropped. Nested blocks of other conditions are copied with their parent part.
- A block with #elif, or a compound condition naming TARGET_3DS, is copied unchanged and printed as skip.
Prints `unifdef3ds: skip <file>:<line>`, `unifdef3ds: <file> blocks <n>` per changed file, `unifdef3ds: total blocks <n>`.
Idempotent: a second run changes nothing. CRLF line ends are kept.
"""
import re
import sys

DIR_RE = re.compile(r'^\s*#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)$')
DEF_POS = r'defined\s*(\(\s*TARGET_3DS\s*\)|TARGET_3DS)'
DEF_NEG = r'!\s*defined\s*(\(\s*TARGET_3DS\s*\)|TARGET_3DS)'


def clean(rest):
    rest = re.sub(r'/\*.*?\*/', '', rest)
    rest = rest.split('//')[0]
    return rest.strip()


def comment_after(line, in_c):
    """Block comment state at the end of line, given state at its start."""
    i = 0
    n = len(line)
    while i < n:
        if in_c:
            j = line.find('*/', i)
            if j < 0:
                return True
            in_c = False
            i = j + 2
        else:
            j = line.find('/*', i)
            k = line.find('//', i)
            if k >= 0 and (j < 0 or k < j):
                return False
            if j < 0:
                return False
            in_c = True
            i = j + 2
    return in_c


def classify(kind, rest):
    if kind == 'ifdef' and rest == 'TARGET_3DS':
        return 'pos'
    if kind == 'ifndef' and rest == 'TARGET_3DS':
        return 'neg'
    if kind == 'if':
        if re.fullmatch(DEF_POS, rest):
            return 'pos'
        if re.fullmatch(DEF_NEG, rest):
            return 'neg'
    if re.search(r'\bTARGET_3DS\b', rest):
        return 'skip'
    return 'ord'


def cur_emit(frames):
    if not frames:
        return True
    f = frames[-1]
    if not f['t3']:
        return f['pe']
    active = f['keep'] != f['else']
    return f['pe'] and active


def process(path):
    """Returns (new_text, old_text, blocks, skips)."""
    with open(path, 'r', encoding='latin-1', newline='') as fh:
        text = fh.read()
    parts = text.split('\n')
    lines = [p + '\n' for p in parts[:-1]]
    if parts[-1]:
        lines.append(parts[-1])

    # pre-pass: directives (not inside block comments), matching #endif, #elif per #if
    dirs = {}
    in_c = False
    for i, line in enumerate(lines):
        if not in_c:
            m = DIR_RE.match(line)
            if m:
                dirs[i] = (m.group(1), clean(m.group(2)))
        in_c = comment_after(line, in_c)
    match_end = {}
    has_elif = {}
    stack = []
    for i in sorted(dirs):
        kind = dirs[i][0]
        if kind in ('if', 'ifdef', 'ifndef'):
            stack.append(i)
        elif kind == 'elif':
            if stack:
                has_elif[stack[-1]] = True
        elif kind == 'endif':
            if stack:
                match_end[stack.pop()] = i

    out = []
    frames = []
    skip_until = None
    skip_emit = False
    blocks = 0
    skips = []
    for i, line in enumerate(lines):
        if skip_until is not None:
            if skip_emit:
                out.append(line)
            if i == skip_until:
                skip_until = None
            continue
        emit = cur_emit(frames)
        d = dirs.get(i)
        if d is None:
            if emit:
                out.append(line)
            continue
        kind, rest = d
        if kind in ('if', 'ifdef', 'ifndef'):
            c = classify(kind, rest)
            if c in ('pos', 'neg') and i in has_elif:
                c = 'skip'
            if c == 'skip':
                skips.append(i + 1)
                skip_until = match_end.get(i, len(lines) - 1)
                skip_emit = emit
                if emit:
                    out.append(line)
                continue
            if c in ('pos', 'neg'):
                blocks += 1
                frames.append({'t3': True, 'keep': c == 'pos', 'else': False, 'pe': emit})
                continue
            frames.append({'t3': False, 'pe': emit})
            if emit:
                out.append(line)
            continue
        if kind == 'elif':
            if emit:
                out.append(line)
            continue
        if kind == 'else':
            if not frames:
                if emit:
                    out.append(line)
                continue
            f = frames[-1]
            if f['t3']:
                f['else'] = True
                continue
            if emit:
                out.append(line)
            continue
        if kind == 'endif':
            if not frames:
                if emit:
                    out.append(line)
                continue
            f = frames.pop()
            if f['t3']:
                continue
            if emit:
                out.append(line)
            continue
    return ''.join(out), text, blocks, skips


def main(argv):
    if not argv:
        print('usage: unifdef3ds.py <file>...', file=sys.stderr)
        return 2
    total = 0
    any_skip = False
    for path in argv:
        new, old, blocks, skips = process(path)
        total += blocks
        for ln in skips:
            print(f'unifdef3ds: skip {path}:{ln}')
            any_skip = True
        if new != old:
            with open(path, 'w', encoding='latin-1', newline='') as fh:
                fh.write(new)
            print(f'unifdef3ds: {path} blocks {blocks}')
    print(f'unifdef3ds: total blocks {total}')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
