# Row FIXSYMBOL: symbol fixture

(Model: Haiku. Fixture for rowlint.py. One fault kind only.)
## Facts
- none.
## Touch only
Files: 3ds/src/nd_gtab.c. New names: none. Host link: nd_host_test.sh:73.
## Exact edits
1. Edit 3ds/src/nd_gtab.c: add `void nd_ntab_run(const NdNTab* t) { nd_walk_state((const uint32_t*)t->list, 1u); }`.
## Gate
- rowlint on this file: faults 1 (symbol), exit 1.
