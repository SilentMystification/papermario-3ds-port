#!/usr/bin/env python3
"""codemod.py: rewrite the value calls of the call sites to their native form (CODEMOD row, plan section 2).

    python 3ds/tools/codemod.py [--dry] [--ntab <ntab_tables.txt>] <file or directory>...   (a directory: every *.c below it)

A tokenizer (comments, strings, character constants and preprocessor lines are tokens of their own) finds each call of a mapped
name with its whole argument list (balanced brackets). A call is rewritten only when it is exact:
  - the first argument is gMainGfxPos++ (nd_rec_dl takes the list alone),
  - the argument count is the one of the macro,
  - no comment and no preprocessor line lies inside the call, and the call is a whole statement (`;` follows the closing bracket),
  - a call that is removed (gDPPipeSync) also stands as its own statement (after `;`, `{`, `}` or `:`),
  - the call is not inside a macro definition,
  - a static list call has exactly one ND_PSO_DESC name in the file (the entry).
Any other site stays as it is and is printed `codemod keep <file>:<line> <name> <reason>`. Never a partial rewrite of a statement.

Mapping (P is gMainGfxPos++):
  gSPMatrix(P, m, f)                       -> nd_xf_push(m, f)
  gSPPopMatrix(P, n)                       -> nd_xf_pop(n)
  gDPPipeSync(P)                           -> removed (the native draw orders itself)
  gDPSetPrimColor(P, m, l, r, g, b, a)     -> nd_val_prim(m, l, r, g, b, a)
  gDPSetEnvColor(P, r, g, b, a)            -> nd_val_env(r, g, b, a)
  gSPSegment(P, s, base)                   -> nd_val_seg(s, base)
  gDPSetTileSize(P, t, uls, ult, lrs, lrt) -> nd_val_tile_size(t, uls, ult, lrs, lrt)
  nd_rec_dl(list), gSPDisplayList(P, list) -> nd_draw_list(list, <the ND_PSO_DESC name of the file>)
  nd_rec_dl(list) of a v2 table (--ntab)      -> nd_draw_list(list, ND_SELF)

--dry changes no file and prints per file `codemod file <path> value <v> keep <k> state <s> dl <d>` (v, k: the seven value groups;
state: the other gDP/gSP/gPm calls the tool leaves to the hand rows; d: static list call sites), then per name
`codemod group <name> value <v> keep <k>` and `codemod total ...` with the share of the value calls among the active calls.
"""
import os
import re
import sys

P = 'gMainGfxPos++'
# name -> (argument count, first argument is P, native name or None for removal, argument indexes of the native call)
MAP = {
    'gSPMatrix': (3, True, 'nd_xf_push', (1, 2)),
    'gSPPopMatrix': (2, True, 'nd_xf_pop', (1,)),
    'gDPPipeSync': (1, True, None, None),
    'gDPSetPrimColor': (7, True, 'nd_val_prim', (1, 2, 3, 4, 5, 6)),
    'gDPSetEnvColor': (5, True, 'nd_val_env', (1, 2, 3, 4)),
    'gSPSegment': (3, True, 'nd_val_seg', (1, 2)),
    'gDPSetTileSize': (6, True, 'nd_val_tile_size', (1, 2, 3, 4, 5)),
    'nd_rec_dl': (1, False, 'nd_draw_list', (0,)),
    'gSPDisplayList': (2, True, 'nd_draw_list', (1,)),
}
DL_NAMES = ('nd_rec_dl', 'gSPDisplayList')
NTAB = set()  # CONV2F: the names of the self-contained v2 tables (--ntab <file>); empty without the option
STATE_RE = re.compile(r'^g(DP|SP|Pm)[A-Z]\w*$')
STMT_PREV = (';', '{', '}', ':')
ID_RE = re.compile(r'[A-Za-z_]\w*')
PUNCT3 = ('<<=', '>>=', '...')
PUNCT2 = ('++', '--', '->', '<<', '>>', '<=', '>=', '==', '!=', '&&', '||', '+=', '-=', '*=', '/=', '%=', '&=', '|=', '^=', '##')


def tokenize(s):
    """[(kind, start, end)] with kind ws, comment, id, num, str, char, pp, punct."""
    toks = []
    i, n = 0, len(s)
    bol = True
    while i < n:
        c = s[i]
        if c == '\n':
            toks.append(('ws', i, i + 1)); i += 1; bol = True; continue
        if c in ' \t\r\f\v':
            j = i
            while j < n and s[j] in ' \t\r\f\v':
                j += 1
            toks.append(('ws', i, j)); i = j; continue
        if c == '#' and bol:
            j = i
            while j < n:
                if s[j] == '\n':
                    if s[j - 1] == '\\' or (s[j - 1] == '\r' and j > 1 and s[j - 2] == '\\'):
                        j += 1; continue
                    break
                j += 1
            toks.append(('pp', i, j)); i = j; continue
        bol = False
        if s.startswith('//', i):
            j = s.find('\n', i)
            j = n if j < 0 else j
            toks.append(('comment', i, j)); i = j; continue
        if s.startswith('/*', i):
            j = s.find('*/', i + 2)
            j = n if j < 0 else j + 2
            toks.append(('comment', i, j)); i = j; continue
        if c == '"' or c == "'":
            j = i + 1
            while j < n and s[j] != c:
                j += 2 if s[j] == '\\' else 1
            j = min(j + 1, n)
            toks.append(('str' if c == '"' else 'char', i, j)); i = j; continue
        m = ID_RE.match(s, i)
        if m:
            toks.append(('id', i, m.end())); i = m.end(); continue
        if c.isdigit() or (c == '.' and i + 1 < n and s[i + 1].isdigit()):
            j = i + 1
            while j < n and (s[j].isalnum() or s[j] in '._' or (s[j] in '+-' and s[j - 1] in 'eEpP')):
                j += 1
            toks.append(('num', i, j)); i = j; continue
        w = 3 if s[i:i + 3] in PUNCT3 else 2 if s[i:i + 2] in PUNCT2 else 1
        toks.append(('punct', i, i + w)); i += w
    return toks


def lineno(s, pos):
    return s.count('\n', 0, pos) + 1


def rewrite(path, s):
    """Returns (new text, stats, state count, static list site count, keep lines). stats: name -> [rewritten, kept]."""
    toks = tokenize(s)
    descs = sorted(set(re.findall(r'ND_PSO_DESC\(\s*(\w+)', s)))
    if len(descs) == 1:
        entry, entry_why = descs[0], None
    else:
        entry, entry_why = None, 'no-entry' if not descs else 'entry-ambiguous'
    stats = {k: [0, 0] for k in MAP}
    state = 0
    keeps = []
    edits = []  # (start, end, text)

    def keep(name, pos, why):
        stats[name][1] += 1
        keeps.append('codemod keep %s:%d %s %s' % (path, lineno(s, pos), name, why))

    # calls inside a macro definition: never rewritten
    for k, a, b in toks:
        if k != 'pp':
            continue
        for m in re.finditer(r'\b(\w+)\s*\(', s[a:b]):
            nm = m.group(1)
            if nm in MAP:
                keep(nm, a + m.start(), 'macro')
            elif STATE_RE.match(nm):
                state += 1
    sig = [i for i, t in enumerate(toks) if t[0] != 'ws']
    i = 0
    while i < len(sig):
        k, a, b = toks[sig[i]]
        nm = s[a:b]
        if k != 'id':
            i += 1
            continue
        nk = toks[sig[i + 1]] if i + 1 < len(sig) else None
        if nm not in MAP:
            if STATE_RE.match(nm) and nk and nk[0] == 'punct' and s[nk[1]] == '(':
                state += 1
            i += 1
            continue
        nargs, firstp, native, idx = MAP[nm]
        if not (nk and nk[0] == 'punct' and s[nk[1]] == '('):
            if nk and nk[0] == 'comment' and i + 2 < len(sig) and s[toks[sig[i + 2]][1]] == '(':
                keep(nm, a, 'comment')
            i += 1
            continue
        depth = 0
        j = i + 1
        args = []
        cur = toks[sig[j]][2]
        bad = None
        while j < len(sig):
            tk, ta, tb = toks[sig[j]]
            if tk == 'comment':
                bad = bad or 'comment'
            elif tk == 'pp':
                bad = bad or 'preproc'
            elif tk == 'punct':
                c = s[ta]
                if c in '([{':
                    depth += 1
                elif c in ')]}':
                    depth -= 1
                    if depth == 0:
                        args.append(s[cur:ta])
                        break
                elif c == ',' and depth == 1:
                    args.append(s[cur:ta]); cur = tb
            j += 1
        if j >= len(sig):
            keep(nm, a, 'unbalanced')
            i += 1
            continue
        close = j
        endpos = toks[sig[close]][2]
        args = [x.strip() for x in args]
        if nargs == 1 and args == ['']:
            args = []
        nxt = toks[sig[close + 1]] if close + 1 < len(sig) else None
        prev = toks[sig[i - 1]] if i > 0 else None
        prev_txt = s[prev[1]:prev[2]] if prev else ';'
        pk = prev[0] if prev else 'pp'
        why = bad
        if not why and len(args) != nargs:
            why = 'args'
        if not why and firstp and re.sub(r'\s+', '', args[0]) != P:
            why = 'first-arg'
        if not why and not (nxt and nxt[0] == 'punct' and s[nxt[1]] == ';'):
            why = 'stmt'
        if not why and native is None and not (pk == 'pp' or prev_txt in STMT_PREV):
            why = 'ctx'
        if not why and native is not None and not (pk == 'pp' or prev_txt in STMT_PREV or prev_txt == ')' or prev_txt == 'else'):
            why = 'ctx'
        # CONV2F: a v2 table of the file (--ntab) takes ND_SELF and needs no entry
        self_ok = nm == 'nd_rec_dl' and bool(args) and re.sub(r'\s+', '', args[0]) in NTAB
        if not why and nm in DL_NAMES and entry is None and not self_ok:
            why = entry_why
        if why:
            keep(nm, a, why)
            i = close + 1
            continue
        stats[nm][0] += 1
        semi_end = nxt[2]
        if native is None:
            ls = s.rfind('\n', 0, a) + 1
            le = s.find('\n', semi_end)
            le = len(s) if le < 0 else le + 1
            if s[ls:a].strip() == '' and s[semi_end:le].strip() == '':
                edits.append((ls, le, ''))
            else:
                edits.append((a, semi_end, ''))
        else:
            out = [args[x] for x in idx]
            if nm in DL_NAMES:
                out.append('ND_SELF' if self_ok else entry)
            edits.append((a, endpos, '%s(%s)' % (native, ', '.join(out))))
        i = close + 1
    out = s
    for a, b, t in sorted(edits, reverse=True):
        out = out[:a] + t + out[b:]
    dl = sum(stats[x][0] + stats[x][1] for x in DL_NAMES)
    return out, stats, state, dl, keeps


def main(argv):
    global NTAB
    dry = False
    files = []
    k = 0
    while k < len(argv):
        a = argv[k]
        k += 1
        if a == '--ntab':
            if k >= len(argv):
                print('codemod: --ntab needs a file', file=sys.stderr)
                return 2
            with open(argv[k], 'r', encoding='utf-8') as fh:
                NTAB = set(x.strip() for x in fh if x.strip())
            k += 1
        elif a == '--dry':
            dry = True
        elif os.path.isdir(a):
            for root, _, names in sorted(os.walk(a)):
                files.extend(os.path.join(root, x) for x in sorted(names) if x.endswith('.c'))
        else:
            files.append(a)
    if not files:
        print('usage: codemod.py [--dry] <file>...', file=sys.stderr)
        return 2
    tot = {k: [0, 0] for k in MAP}
    tstate = 0
    for f in files:
        with open(f, 'rb') as fh:
            raw = fh.read()
        s = raw.decode('utf-8', 'surrogateescape')
        new, stats, state, dl, keeps = rewrite(f, s)
        for ln in keeps:
            print(ln)
        v = sum(stats[x][0] for x in MAP if x not in DL_NAMES)
        kp = sum(stats[x][1] for x in MAP if x not in DL_NAMES)
        if dry:
            print('codemod file %s value %d keep %d state %d dl %d' % (f, v, kp, state, dl))
        elif new != s:
            with open(f, 'wb') as fh:
                fh.write(new.encode('utf-8', 'surrogateescape'))
        for x in MAP:
            tot[x][0] += stats[x][0]
            tot[x][1] += stats[x][1]
        tstate += state
    if dry:
        for x in MAP:
            print('codemod group %s value %d keep %d' % (x, tot[x][0], tot[x][1]))
        v = sum(tot[x][0] for x in MAP if x not in DL_NAMES)
        kp = sum(tot[x][1] for x in MAP if x not in DL_NAMES)
        dl = sum(tot[x][0] + tot[x][1] for x in DL_NAMES)
        act = v + kp + tstate
        share = (100.0 * v / act) if act else 0.0
        print('codemod total value %d keep %d state %d dl %d active %d share %.1f%%' % (v, kp, tstate, dl, act, share))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
