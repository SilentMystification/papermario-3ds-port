# Row ROWLINT2: rowlint.py checks g (symbol: an added function calls only symbols its host link has) and h (Needs: lines)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main 75f49f988)
- The fault to catch: the first CONV2D2 text put `void nd_ntab_run(const NdNTab* t) { ... nd_walk_state(...); }` into
  3ds/src/nd_gtab.c. nd_host_test.sh:73 and 3ds/tests/host.d/conv2c.sh:8 compile nd_gtab.c with -DND_SCOMP_HOST and without
  3ds/src/nd_walk.c, the only file that defines nd_walk_state, so the host link failed.
- 3ds/tools/rowlint.py (227 lines): main() at :109 reads the row, `section(lines, '## Touch only')` gives the Touch only lines,
  check e (host) at :199-204 reads the host scripts from HEAD with head() and ls_tree('3ds/tests/host.d'). Faults go to the list
  `faults` as (kind, detail) and are printed at :221.
Start: `sh 3ds/tools/wt_new.sh ROWLINT2`. PM_ROW=ROWLINT2 PM_LANE=ROWLINT2 on every call.
## Touch only (places)
3ds/tools/rowlint.py: a new function `check_symbols` and one call in main after check e; the docstring kinds line (:7).
New 3ds/tools/rowlint_fixtures/symbol.md. New names: none. Host link: none.
## Exact edits
1. check_symbols(lines, faults):
   a. Snippets: in "## Exact edits", each backtick span that contains `(` and `{` is a function body. Its file is the last
      `3ds/src/<x>.c` path named before it in the same numbered edit (or the same line). Skip the snippet when that edit names
      `ND_SCOMP_HOST` with "inside" (the host does not compile it), or when the file is not 3ds/src/*.c.
   b. Host links of the file: each gcc line of 3ds/tools/nd_host_test.sh and 3ds/tests/host.d/*.sh (from HEAD) that names the file.
      None: skip. The linked set is every repo path ending .c on those lines. An object file (.o) on the line: its symbols count as
      present only for names that start with `ndn_` or `ndg_` (the generated tables).
   c. Calls: each identifier followed by `(` in the snippet, minus the function's own name, C keywords (if for while switch return
      sizeof), casts (a type in parentheses), names in all capitals (macros), and the C library names memcpy memset strcmp strlen
      printf snprintf fprintf fopen fclose malloc free.
   d. For each call: present when one linked file (from HEAD) has a definition line `^[A-Za-z_][\w \*]*\b<name>\s*\(` that does
      not end with `;`, or a `#define <name>` exists in 3ds/include/*.h, include/**/*.h or 3ds/tools/*.h. Else
      `faults.append(('symbol', '<name> called in <file> is not in its host link (<script>:<line>)'))`.
2. check_needs(lines, faults), check h (needs): each line `Needs: <path> has <text>` (the data state a gate needs, for example
   `Needs: docs/handoff/row_SEEDS.md has | live`) holds when the HEAD file contains the text; else
   `faults.append(('needs', '<path> lacks <text>'))`. Add symbol (g) and needs (h) to the kinds line of the docstring.
   row_TEMPLATE.md asks each row for these lines (MKROW stopped on a gate that needed live verdicts that main lacked).
3. New fixtures 3ds/tools/rowlint_fixtures/needs.md (one line `Needs: docs/handoff/row_SEEDS.md has | live`, a fault on main
   today) and the new 3ds/tools/rowlint_fixtures/symbol.md: a fixture row (as host.md) with `Host link: nd_host_test.sh:73`, whose edit 1 names 3ds/src/nd_gtab.c
   and has the snippet `void nd_ntab_run(const NdNTab* t) { nd_walk_state((const uint32_t*)t->list, 1u); }`.
4. Commit: `ROWLINT2: rowlint check g (symbol) for functions added to host-linked files`.
## Gate (expected values)
1. On the new fixture: `python 3ds/tools/rowlint.py 3ds/tools/rowlint_fixtures/symbol.md`: exactly one line `rowlint FAULT symbol: nd_walk_state ...`,
   exit 1 (the seeded fault).
2. The 7 old fixtures give their old results (good.md: faults 0). needs.md: exactly one `rowlint FAULT needs`, exit 1.
3. `python 3ds/tools/rowlint.py docs/handoff/row_CONV2D2.md`: no symbol fault (its nd_ntab_find snippet is inside the
   ND_SCOMP_HOST guard, and nd_ntab_run goes into nd_walk.c, which no host script links).
4. Budget 0 Azahar starts. Counter: none moves (tool row).
## Stop and end
A fixture result other than the gate, or a tool fault: stop, commit, hand back the exact output. Delete this row file at the end.
