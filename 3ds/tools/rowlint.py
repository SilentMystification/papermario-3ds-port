#!/usr/bin/env python3
"""rowlint.py <row file>: check a row file against main before a coder starts (no Azahar start).

Run from the repo root. Paths, file lines, registries, build.sh lists and host tests are read from main (HEAD);
the row file itself is read from disk. Prints `rowlint FAULT <kind>: <detail>` per fault, then
`rowlint <file> faults <n>`. Exit 1 when n > 0, else 0 (2 for a missing row file).
Kinds: path (a), line and quote (b), names (c), sync (d), host (e), seed (f).
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
