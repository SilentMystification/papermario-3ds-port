#!/usr/bin/env python3
"""rowlint.py <row file>: check a row file against main before a coder starts (no Azahar start).

Run from the repo root. Paths, file lines, registries, build.sh lists and host tests are read from main (HEAD);
the row file itself is read from disk. Prints `rowlint FAULT <kind>: <detail>` per fault, then
`rowlint <file> faults <n>`. Exit 1 when n > 0, else 0 (2 for a missing row file).
Kinds: path (a), line and quote (b), names (c), sync (d), host (e), seed (f), symbol (g), needs (h).
"""
import json
import os
import re
import subprocess
import sys

PATH = r'(?:3ds|src|include|assets|docs)/[\w./-]+\.(?:c|h|py|sh|json|md|txt|ps1|cmake|ld)'
PATH_RE = re.compile(r'(?<![\w/.-])(' + PATH + r')')
TOK_RE = re.compile(r'(?<![\w/.-])(' + PATH + r')(?::(\d+)(?:-(\d+))?)?|(?<![\w/]):(\d+)(?:-(\d+))?')
QUOTE_RE = re.compile(r'`([^`]+)`')
SRC_RE = r'3ds/src/[^/]+\.c'
NEW_RE = re.compile(r'\b[Nn]ew\b')

_cache = {}


def git(*args):
    return subprocess.run(['git', *args], capture_output=True)


def head(path):
    """Text of path at HEAD, or None when main has no such file."""
    if path not in _cache:
        r = git('show', 'HEAD:' + path)
        _cache[path] = r.stdout.decode('utf-8', 'replace') if r.returncode == 0 else None
    return _cache[path]


def in_head(path):
    return git('cat-file', '-e', 'HEAD:' + path).returncode == 0


def ls_tree(d):
    r = git('ls-tree', '-r', '--name-only', 'HEAD', '--', d)
    return r.stdout.decode('utf-8', 'replace').split() if r.returncode == 0 else []


def section(lines, prefix):
    out, on = [], False
    for ln in lines:
        if ln.startswith('## '):
            on = ln.startswith(prefix)
            continue
        if on:
            out.append(ln)
    return out


def field(lines, label):
    """Text after the first `label` on a line of the file, or None."""
    for ln in lines:
        i = ln.find(label)
        if i >= 0:
            return ln[i + len(label):]
    return None


def build_list(text, key):
    m = re.search(r'^' + key + r'="([^"]*)"', text, re.M)
    return m.group(1).split() if m else []


def covered(p, entries):
    return any(p == e or p.startswith(e.rstrip('/') + '/') for e in entries)


def names_of(node, out):
    """Every bench or scenario name in a registry JSON node."""
    if isinstance(node, dict):
        if isinstance(node.get('name'), str):
            out.add(node['name'])
        for k, v in node.items():
            if k in ('benches', 'scenarios') and isinstance(v, dict):
                out.update(v.keys())
            if isinstance(v, (dict, list)):
                names_of(v, out)
    elif isinstance(node, list):
        for x in node:
            names_of(x, out)
    return out


def registries(rid):
    """file -> set of names, for the four registries on main. The row's own azt.d/bench.d files are exempt."""
    files = ['3ds/tools/bench_defs.json', '3ds/tools/azt_scenarios.json']
    for d in ('3ds/tools/bench.d', '3ds/tools/azt.d'):
        files += [f for f in ls_tree(d) if f.endswith('.json') and os.path.basename(f) != rid + '.json']
    reg = {}
    for f in files:
        text = head(f)
        if text is None:
            continue
        try:
            reg[f] = names_of(json.loads(text), set())
        except ValueError as e:
            print(f'rowlint: bad JSON in {f} on main: {e}')
            sys.exit(2)
    return reg


CALL_RE = re.compile(r'\b([A-Za-z_]\w*)\s*\(')
BT_RE = re.compile(r'`([^`]+)`')
EDIT_RE = re.compile(r'^\d+\.')
NEEDS_RE = re.compile(r'^\s*Needs:\s*(\S+)\s+has\s+(.+?)\s*$')
C_SKIP = {'if', 'for', 'while', 'switch', 'return', 'sizeof',
          'memcpy', 'memset', 'strcmp', 'strlen', 'printf', 'snprintf', 'fprintf', 'fopen', 'fclose', 'malloc', 'free'}
_repo = {}


def repo_file(p):
    if p not in _repo:
        _repo[p] = in_head(p)
    return _repo[p]


def edit_blocks(lines):
    """Text of each numbered edit in the Exact edits section."""
    out = []
    for ln in section(lines, '## Exact edits'):
        if EDIT_RE.match(ln):
            out.append([ln])
        elif out:
            out[-1].append(ln)
    return ['\n'.join(b) for b in out]


def snippets(txt):
    """(file, body) for each backtick span with ( and { in one numbered edit; file is the last 3ds/src/*.c before it."""
    srcs = [(m.start(), m.group(0)) for m in re.finditer(SRC_RE, txt)]
    out = []
    for m in BT_RE.finditer(txt):
        body = m.group(1)
        if '(' not in body or '{' not in body:
            continue
        before = [f for pos, f in srcs if pos < m.start()]
        if before:
            out.append((before[-1], body))
    return out


def host_lines():
    """(script, line, logical text) of each gcc line in the host scripts on main; a trailing backslash joins the next line."""
    scripts = ['3ds/tools/nd_host_test.sh'] + [f for f in ls_tree('3ds/tests/host.d') if f.endswith('.sh')]
    out = []
    for s in scripts:
        text = head(s)
        if text is None:
            continue
        phys = text.splitlines()
        i = 0
        while i < len(phys):
            start, logical = i + 1, phys[i]
            while logical.rstrip().endswith('\\') and i + 1 < len(phys):
                i += 1
                logical = logical.rstrip()[:-1] + ' ' + phys[i]
            i += 1
            if 'gcc' in logical:
                out.append((s, start, logical))
    return out


def header_define(name, headers):
    """True when one of the header files (paths in headers, from HEAD) has `#define name`."""
    pat = r'^[[:space:]]*#[[:space:]]*define[[:space:]]+' + re.escape(name) + r'([^A-Za-z0-9_]|$)'
    r = git('grep', '-E', '-n', pat, 'HEAD', '--', 'include', '3ds/include', '3ds/tools')
    for ln in r.stdout.decode('utf-8', 'replace').splitlines():
        if ln[len('HEAD:'):].split(':', 1)[0] in headers:
            return True
    return False


def strip_comment(ln):
    return re.sub(r'/\*.*?\*/|//.*$', '', ln).rstrip()


def check_symbols(lines, faults):
    """Check g: each call in an added 3ds/src/*.c snippet is defined in a file its host link compiles, or is a #define."""
    edits = []
    for blk in edit_blocks(lines):
        if 'ND_SCOMP_HOST' in blk and 'inside' in blk:
            continue
        edits.append(blk)
    jobs = []
    for blk in edits:
        for f, body in snippets(blk):
            jobs.append((f, body))
    if not jobs:
        return
    hl = host_lines()
    headers = [f for f in ls_tree('3ds/include') if os.path.dirname(f) == '3ds/include' and f.endswith('.h')]
    headers += [f for f in ls_tree('include') if f.endswith('.h')]
    headers += [f for f in ls_tree('3ds/tools') if os.path.dirname(f) == '3ds/tools' and f.endswith('.h')]
    headers = set(headers)
    seen = set()
    for f, body in jobs:
        naming = [(s, n, lg) for s, n, lg in hl if re.search(re.escape(f) + r'(?![\w.])', lg)]
        if not naming:
            continue  # no host link of this file
        linked = set()
        objs = False
        for s, n, lg in naming:
            linked |= {t for t in re.findall(r'[\w./-]+\.c(?![\w])', lg) if repo_file(t)}
            objs = objs or bool(re.search(r'[\w./-]+\.o(?![\w])', lg))
        where = min((os.path.basename(s), n) for s, n, _ in naming)
        brace = body.find('{')
        own = [m.group(1) for m in CALL_RE.finditer(body[:brace])]
        own = own[-1] if own else None
        defs = {t: head(t) or '' for t in linked}
        for m in CALL_RE.finditer(body):
            name = m.group(1)
            if name == own or name in C_SKIP or re.fullmatch(r'[A-Z0-9_]+', name):
                continue
            if (f, name) in seen:
                continue
            seen.add((f, name))
            if name.startswith(('ndn_', 'ndg_')) and objs:
                continue
            pat = re.compile(r'^[A-Za-z_][\w \*]*\b' + re.escape(name) + r'\s*\(')
            if any(pat.match(strip_comment(ln)) and not strip_comment(ln).endswith(';')
                   for t in linked for ln in defs[t].splitlines()):
                continue
            if header_define(name, headers):
                continue
            faults.append(('symbol', f'{name} called in {f} is not in its host link ({where[0]}:{where[1]})'))


def check_needs(lines, faults):
    """Check h: each `Needs: <path> has <text>` line holds when the HEAD file contains the text."""
    for ln in lines:
        m = NEEDS_RE.match(ln)
        if not m:
            continue
        p, text = m.group(1), m.group(2)
        src = head(p)
        if src is None or text not in src:
            faults.append(('needs', f'{p} lacks {text}'))


def main():
    if len(sys.argv) != 2:
        print('usage: python 3ds/tools/rowlint.py <row file>')
        return 2
    fn = sys.argv[1]
    try:
        with open(fn, encoding='utf-8', errors='replace') as fh:
            lines = fh.read().splitlines()
    except OSError:
        print(f'rowlint: no such row file {fn}')
        return 2
    faults = []
    title = next((ln for ln in lines if ln.startswith('# Row ')), '')
    m = re.match(r'# Row (\S+?):', title)
    rid = m.group(1) if m else ''

    touch = []  # (path, new) for each path token in the Touch only section
    for ln in section(lines, '## Touch only'):
        for m in PATH_RE.finditer(ln):
            touch.append((m.group(1), bool(NEW_RE.search(ln[:m.start()]))))

    # a. path: every path token exists on main, unless its line says New before it
    done = set()
    for i, ln in enumerate(lines, 1):
        for m in PATH_RE.finditer(ln):
            p = m.group(1)
            if p in done or NEW_RE.search(ln[:m.start()]):
                continue
            done.add(p)
            if not in_head(p):
                faults.append(('path', f'line {i}: {p} not on main'))

    # b. line and quote: path:n or path:n-m (and a bare :n after such a path) inside the file; quote in n..m
    for i, ln in enumerate(lines, 1):
        cur = None
        for m in TOK_RE.finditer(ln):
            if m.group(1):
                if m.group(2) is None:
                    cur = None
                    continue
                p, n, k = m.group(1), int(m.group(2)), int(m.group(3) or m.group(2))
                cur = p
            else:
                if cur is None:
                    continue
                p, n, k = cur, int(m.group(4)), int(m.group(5) or m.group(4))
            src = head(p)
            if src is None:
                continue  # a new or missing file: the path check reports it
            src_lines = src.splitlines()
            if n < 1 or k < n or k > len(src_lines):
                faults.append(('line', f'line {i}: {p}:{n}-{k} outside 1..{len(src_lines)}'))
                continue
            rest = ln[m.end():]
            q = QUOTE_RE.search(rest)
            nxt = TOK_RE.search(rest)
            if q and (nxt is None or q.start() < nxt.start()):
                want = re.sub(r'\s', '', q.group(1))
                have = ''.join(re.sub(r'\s', '', x) for x in src_lines[n - 1:k])
                if want not in have:
                    faults.append(('quote', f'line {i}: `{q.group(1)}` not in {p}:{n}-{k}'))

    # c. names: each New names entry is absent from the registries (row's own azt.d/bench.d excepted)
    names_text = field(lines, 'New names:')
    if names_text is not None:
        nm = names_text.split('Host link:')[0].strip().rstrip('.').strip()
        names = [] if nm.lower() in ('', 'none') else [x.strip().strip('`').rstrip('.') for x in nm.split(',') if x.strip()]
        reg = registries(rid)
        for name in names:
            for f, s in reg.items():
                if name in s:
                    faults.append(('names', f'{name} already in {f}'))
                    break

    host_text = field(lines, 'Host link:')
    host_link = (host_text or '').strip().rstrip('.').strip().lower()

    # d. sync: a new device-read file under 3ds/ must sit under a CODE_DIRS entry and a KEY_IN entry
    build = head('3ds/build.sh') or ''
    code_dirs = build_list(build, 'CODE_DIRS')
    key_in = build_list(build, 'KEY_IN')
    for p, new in touch:
        if not new or not p.startswith('3ds/'):
            continue
        if not (p.rsplit('.', 1)[-1] in ('c', 'h', 'cmake', 'ld') or p.startswith('3ds/tools/cov.d/')):
            continue
        miss = [k for k, lst in (('CODE_DIRS', code_dirs), ('KEY_IN', key_in)) if not covered(p, lst)]
        if miss:
            faults.append(('sync', f'new {p} not under {" or ".join(miss)} of 3ds/build.sh'))

    # e. host: a 3ds/src/*.c in Touch only that a host test links needs a Host link line that is not none
    host_src = (head('3ds/tools/nd_host_test.sh') or '')
    host_src += ''.join(head(f) or '' for f in ls_tree('3ds/tests/host.d') if f.endswith('.sh'))
    for p in sorted({p for p, _ in touch if re.fullmatch(SRC_RE, p)}):
        if p in host_src and host_link.startswith('none'):
            faults.append(('host', f'{p} is linked by a host test, Host link: none'))

    # g. symbol: a call in a new 3ds/src/*.c snippet of Exact edits is defined in a file its host link compiles
    check_symbols(lines, faults)

    # h. needs: each Needs: line holds on main
    check_needs(lines, faults)

    # f. seed: a CODEMOD row names each 3ds/src/*.c of Touch only with a seed line (file:line, seed, scene or drec)
    if title.startswith('# Row CODEMOD'):
        seed_tbl = [ln for ln in (head('docs/handoff/row_SEEDS.md') or '').splitlines() if ln.startswith('|')]
        reg_names = set()
        for s in registries(rid).values():
            reg_names |= s
        for p in sorted({p for p, _ in touch if re.fullmatch(SRC_RE, p)}):
            ok = any(re.search(re.escape(p) + r':\d+', ln) and 'seed' in ln.lower()
                     and (re.search(r'scene|drec', ln, re.I) or any(re.search(r'\b' + re.escape(n) + r'\b', ln) for n in reg_names))
                     for ln in lines)
            ok = ok or any(p in ln for ln in seed_tbl)
            if not ok:
                faults.append(('seed', f'{p} has no seed line with a line number and a scene or drec name'))

    for kind, detail in faults:
        print(f'rowlint FAULT {kind}: {detail}')
    print(f'rowlint {fn} faults {len(faults)}')
    return 1 if faults else 0


if __name__ == '__main__':
    sys.exit(main())
