# Row MKROW: new 3ds/tools/mkrow.py writes a codemod batch row from row_BATCHTEMPLATE.md and the live seeds of row_SEEDS.md

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
Blocked by ROWLINT (mkrow.py runs rowlint.py on its output).
## Facts (ORACLE8, main of 2026-11-13)
- docs/handoff/row_SEEDS.md: data lines `file | fxtest id | value keep state dl | seed line | `call` | `y argument` | verdict`
  (7 fields split by ` | `), then a "Later" list. Only verdict `live` may go into a row. Correction 1 (ORACLE8, 2026-11-13): the
  template names no tool that main lacks (step 5 reads the verdicts), so the rowlint path check on the output passes; resume after 743420fa9.
- docs/handoff/row_BATCHTEMPLATE.md: the row text with the fields {ID} {N} {VALUES} {MAIN} {TABLE} {FILES} {PATHS} {SCENE} {IDS}
  {LAST} {NEEDS}. Every field is plain text replacement.
- `python 3ds/tools/codemod.py --dry <paths>` prints `codemod file <path> value v keep k state s dl d` per file.
Start: `sh 3ds/tools/wt_new.sh MKROW`. PM_ROW=MKROW PM_LANE=MKROW on every call.
## Touch only (places)
New 3ds/tools/mkrow.py and 3ds/tools/mkrow_fixtures/ (a seed table fixture). New names: none. Host link: none.
## Exact edits
1. mkrow.py `<row id> <name>... [--seeds <file>]` (names: effect file stems; default seeds file docs/handoff/row_SEEDS.md):
   a. For each name take its row_SEEDS.md line. No line, or a verdict other than `live`: print `mkrow FAULT <name> <reason>`,
      write nothing, exit 1.
   b. Run codemod --dry on the paths src/effects/<name>.c. Its value, keep, state and dl must equal the table; else FAULT.
   c. Fields: N = count, VALUES = sum of value, MAIN = `git rev-parse --short HEAD`, IDS = the fxtest ids in the given order,
      LAST = 412 + 44 (N - 1), SCENE = `<row id in lower case>_drec`, PATHS = the paths with blanks, FILES = the paths with
      commas, TABLE = one line per file: `  - src/effects/<name>.c: <v k s d> | fxtest <id> | frame <412 + 44 i> | seed`
      followed by the spec `src/effects/<name>.c:<line>@<frame>|<y>|<y> + 20.0f` in backticks.
   NEEDS = one line per file `Needs: <the --seeds path> has <the exact seeds table line of the file>` (the live verdict).
   d. Write docs/handoff/row_<row id>.md, then run `python 3ds/tools/rowlint.py` on it and print its last line. A rowlint fault
      deletes the written file and exits 1.
2. New fixture file 3ds/tools/mkrow_fixtures/seeds.md (first line `# FIXTURE: synthetic verdicts, not probe results`): the lines of
   aura and balloon copied from row_SEEDS.md with the verdict set to `live`, and the line of big_snowflakes with `dead`.
   row_SEEDS.md stays unprobed (seedprobe.py is not on main); mkrow.py reads the fixture through --seeds.
3. Commit: `MKROW: mkrow.py writes codemod batch rows from the template and the live seeds`.
## Gate (expected values)
1. With the new tool and fixture: `python 3ds/tools/mkrow.py MKTEST aura balloon --seeds 3ds/tools/mkrow_fixtures/seeds.md`: it
   writes the new file docs/handoff/row_MKTEST.md with 2 table lines and prints `rowlint ... faults 0`. Delete row_MKTEST.md after the check.
2. Seeded fault: the same call with big_snowflakes added: `mkrow FAULT big_snowflakes verdict dead`, exit 1, no file written.
3. Budget 0 Azahar starts. Counter: none moves (tool row).
## Stop and end
A written row with a rowlint fault, or a tool fault: stop, commit, hand back the exact output. Delete this row file at the end.
