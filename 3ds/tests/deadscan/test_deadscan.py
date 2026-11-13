#!/usr/bin/env python3
"""Fixture test for 3ds/tools/deadscan.py: scans fix_a.c with the ELF symbol set {live_fn}.

Prints PASS: <case> / FAIL: <case> per case. Exit 1 on any FAIL.
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
FIX = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / '3ds' / 'tools'))
import deadscan  # noqa: E402


def main():
    recs = deadscan.scan(FIX, [ROOT / 'include'], {'live_fn'})
    by_name = {r['name']: r for r in recs}
    fails = 0

    def check(case, ok):
        nonlocal fails
        print(f"PASS: {case}" if ok else f"FAIL: {case}")
        if not ok:
            fails += 1

    live = by_name.get('live_fn')
    check('live_fn elf 1 calls 2',
          live is not None and live['elf'] == 1 and live['calls'] == 2 and not live['dead'])
    inl = by_name.get('inl_fn')
    check('inl_fn elf 0 refs 1 not dead',
          inl is not None and inl['elf'] == 0 and inl['refs'] == 1 and inl['calls'] == 1 and not inl['dead'])
    dead = by_name.get('dead_fn')
    check('dead_fn elf 0 refs 0 dead',
          dead is not None and dead['elf'] == 0 and dead['refs'] == 0 and dead['calls'] == 3 and dead['dead'])
    dead_recs = [r for r in recs if r['dead']]
    check('total dead 1 dead_calls 3',
          len(dead_recs) == 1 and sum(r['calls'] for r in dead_recs) == 3)
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
