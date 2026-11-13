# Row ROWLINT: 3ds/tools/rowlint.py, the check of a row file against main before a coder starts (no Azahar start)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main of 2026-11-13)
- Row text faults stopped coders 10 times on 2026-11-13: a wrong line, a clashing bench name (spot_mac00_det), a file outside
  CODE_DIRS (3ds/tools/cov.d, build.sh:136 and :150), a symbol the host link lacks (nd_walk_state in nd_gtab.c, nd_host_test.sh:73),
  and dead seeds. row_TEMPLATE.md now asks each row for a `New names:` line and a `Host link:` line under "Touch only".
- Name registries: 3ds/tools/bench_defs.json and azt_scenarios.json (JSON, a list or a dict of scenarios), 3ds/tools/bench.d/*.json
  and azt.d/*.json (`{"benches": [...]}`, `{"scenarios": [...]}`). build.sh CODE_DIRS (:136) and KEY_IN (:150) are space lists.
Start: `sh 3ds/tools/wt_new.sh ROWLINT`. PM_ROW=ROWLINT PM_LANE=ROWLINT on every call.
## Touch only (places)
New 3ds/tools/rowlint.py and 3ds/tools/rowlint_fixtures/ (test rows). New names: none. Host link: none.
## Exact edits
1. rowlint.py `<row file>` (Python 3, standard library, run from the repo root). Each check prints `rowlint FAULT <kind>: <detail>`
   per fault; at the end `rowlint <file> faults <n>`; exit 1 when n > 0, else 0.
   a. path: every token that matches `(3ds|src|include|assets|docs)/[\w./-]+\.(c|h|py|sh|json|md|txt|ps1|cmake|ld)` exists
      (`git cat-file -e HEAD:<path>`), unless the same line says "New" or "new" before it.
   b. line: a token `<path>:<n>` or `<path>:<n>-<m>` (and a bare `:<n>` after such a path on the same line) is within the file
      length. When a backtick quote follows on the same line, its text with blanks removed must occur in lines n..m (or n) with
      blanks removed; else FAULT quote.
   c. names: the names after `New names:` (comma list, "none" allowed) must not exist in any of the four registries, except in
      `azt.d/<row id>.json` and `bench.d/<row id>.json` of the same row.
   d. sync: a new file under 3ds/ named in "Touch only" that the device build reads (extension c, h, cmake, ld, or a path under
      3ds/tools/cov.d) must start with an entry of CODE_DIRS and of KEY_IN; else FAULT sync.
   e. host: for each 3ds/src/*.c in "Touch only", grep 3ds/tools/nd_host_test.sh and 3ds/tests/host.d/*.sh for its path. If one
      links it and the `Host link:` line says "none", FAULT host.
   f. seed: a row whose title starts `# Row CODEMOD` needs, for each src/*.c file in "Touch only", a line that names the file with
      a line number and the word "seed" (or a table line of docs/handoff/row_SEEDS.md for that file), and a scene or drec name.
2. rowlint_fixtures/: one row file per fault kind (a to f) that has exactly that fault, and good.md that has none.
3. Commit: `ROWLINT: rowlint.py checks a row file against main (paths, lines, quotes, names, sync, host link, seeds)`.
## Gate (expected values)
1. `python 3ds/tools/rowlint.py 3ds/tools/rowlint_fixtures/good.md`: `faults 0`, exit 0.
2. Each fixture of kind X: exactly one line `rowlint FAULT X`, exit 1 (this is the seeded fault set of the gate).
3. `python 3ds/tools/rowlint.py docs/handoff/row_CONV2D2.md` and `row_COVMAP.md`: hand back the output (no pass expected).
4. Budget 0 Azahar starts. Counter: none moves (tool row; from now on every launched row passes rowlint first).
## Stop and end
A fixture with a different fault count, or a tool fault: stop, commit, hand back the exact output. Delete this row file at the end.
