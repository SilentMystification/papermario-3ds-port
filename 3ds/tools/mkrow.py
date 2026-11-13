#!/usr/bin/env python3
"""mkrow.py <row id> <name>... [--seeds <file>]: write docs/handoff/row_<row id>.md, a codemod batch row.

Run from the repo root. Each name is an effect file stem (src/effects/<name>.c). Its line in the seeds file (default
docs/handoff/row_SEEDS.md) must have verdict `live`. codemod --dry must match the table's value, keep, state and dl.
Fields come from docs/handoff/row_BATCHTEMPLATE.md. The row is then checked by rowlint.py; a rowlint fault deletes it.

Faults print `mkrow FAULT <name> <reason>` and exit 1, with nothing written (except a rowlint fault, which deletes the row).
"""
import os
import re
import subprocess
import sys

SEEDS_DEFAULT = 'docs/handoff/row_SEEDS.md'
TEMPLATE = 'docs/handoff/row_BATCHTEMPLATE.md'
CM_RE = re.compile(r'codemod file (\S+) value (\d+) keep (\d+) state (\d+) dl (\d+)')


def fault(name, reason):
    print(f'mkrow FAULT {name} {reason}')
    sys.exit(1)


def read_seeds(path):
    """Data lines of the seed table: 7 fields split by ' | '. Other lines are not data.

    Returns name -> (fields, exact line text)."""
    rows = {}
    with open(path, encoding='utf-8') as fh:
        for ln in fh:
            raw = ln.rstrip('\n')
            parts = [p.strip() for p in raw.split(' | ')]
            if len(parts) == 7:
                rows.setdefault(parts[0], (parts, raw))
    return rows


def codemod_counts(path):
    """(value, keep, state, dl) from codemod --dry on one file."""
    r = subprocess.run([sys.executable, '-I', '3ds/tools/codemod.py', '--dry', path],
                       capture_output=True, text=True, encoding='utf-8', errors='replace')
    if r.returncode != 0:
        print(r.stdout + r.stderr)
        sys.exit(1)
    for ln in r.stdout.splitlines():
        m = CM_RE.match(ln.strip())
        if m and m.group(1) == path:
            return tuple(int(m.group(i)) for i in range(2, 6))
    return None


def git_head_short():
    r = subprocess.run(['git', 'rev-parse', '--short', 'HEAD'], capture_output=True, text=True)
    return r.stdout.strip()


def main(argv):
    seeds = SEEDS_DEFAULT
    if '--seeds' in argv:
        i = argv.index('--seeds')
        seeds = argv[i + 1]
        del argv[i:i + 2]
    if len(argv) < 2:
        print('usage: mkrow.py <row id> <name>... [--seeds <file>]', file=sys.stderr)
        return 2
    rid, names = argv[0], argv[1:]
    table = read_seeds(seeds)

    recs = []
    for n in names:
        hit = table.get(n)
        if hit is None:
            fault(n, 'no seed line')
        p, raw = hit
        if p[6] != 'live':
            fault(n, f'verdict {p[6]}')
        path = f'src/effects/{n}.c'
        want = tuple(int(x) for x in p[2].split())
        got = codemod_counts(path)
        if got is None:
            fault(n, 'codemod --dry printed no line for ' + path)
        if got != want:
            fault(n, f'codemod value/keep/state/dl {got} differs from table {want}')
        recs.append({'name': n, 'path': path, 'id': p[1], 'line': p[3], 'y': p[5].strip('`'), 'raw': raw,
                     'v': got[0], 'k': got[1], 's': got[2], 'd': got[3]})

    n_files = len(recs)
    values = sum(r['v'] for r in recs)
    table_lines = []
    for i, r in enumerate(recs):
        frame = 412 + 44 * i
        table_lines.append(
            f"  - {r['path']}: {r['v']} {r['k']} {r['s']} {r['d']} | fxtest {r['id']} | frame {frame} | seed "
            f"`{r['path']}:{r['line']}@{frame}|{r['y']}|{r['y']} + 20.0f`")

    fields = {
        'ID': rid,
        'N': str(n_files),
        'VALUES': str(values),
        'MAIN': git_head_short(),
        'TABLE': '\n'.join(table_lines),
        'FILES': ', '.join(r['path'] for r in recs),
        'PATHS': ' '.join(r['path'] for r in recs),
        'SCENE': rid.lower() + '_drec',
        'IDS': ','.join(r['id'] for r in recs),
        'LAST': str(412 + 44 * (n_files - 1)),
        'NEEDS': '\n'.join(f"Needs: {seeds} has {r['raw']}" for r in recs),
    }
    with open(TEMPLATE, encoding='utf-8') as fh:
        text = fh.read()
    for key, val in fields.items():
        text = text.replace('{' + key + '}', val)

    out = f'docs/handoff/row_{rid}.md'
    with open(out, 'w', encoding='utf-8', newline='\n') as fh:
        fh.write(text)

    r = subprocess.run([sys.executable, '-I', '3ds/tools/rowlint.py', out], capture_output=True, text=True,
                       encoding='utf-8', errors='replace')
    lines = (r.stdout + r.stderr).strip().splitlines()
    for ln in lines:
        if ln.startswith('rowlint FAULT'):
            print(ln)
    print(lines[-1] if lines else 'rowlint printed nothing')
    if r.returncode != 0:
        os.remove(out)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
