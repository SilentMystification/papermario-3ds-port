# Row FXCEN2: Azahar census of all 128 effects (proxy ranking)

(Model: Haiku. Waits: FXCEN1 merged. Branch from main after that merge. Reads only this file and CLAUDE.md.)

## Why
FXCEN1 adds per-slot counts (fxcena, fxcenb lines) and the ranking tool, tested on chunk 0. This row runs chunks 1 to 7 and ranks all
effects. The counts (draws, triangles, texture misses, commands, UpdateContext calls, full combine syncs, model misses) do not depend
on CPU speed; the us column is an Azahar proxy only (memory: Azahar is not the console). No decision is made from it in this row.

## Start
`sh 3ds/tools/wt_new.sh FXCEN2`. PM_ROW=FXCEN2 PM_LANE=FXCEN2 on every build and bench call. No source edit.

## Steps
1. `PM_ROW=FXCEN2 PM_LANE=FXCEN2 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
2. One list on one slot (8 starts, all chunks; the FXCEN1 worktree logs are gone): `bench.py run fxcen_c0,fxcen_c1,fxcen_c2,
   fxcen_c3,fxcen_c4,fxcen_c5,fxcen_c6,fxcen_c7 --row FXCEN2 --why "per-effect census, all chunks"` (no spaces in the list). Each run's
   LOG: copy is build3ds/azt/logs/fxcen_c<k>.txt.
3. `python 3ds/tools/fxcen_report.py build3ds/azt/logs/fxcen_c0.txt ... build3ds/azt/logs/fxcen_c7.txt --label azahar`.
4. Commit only the runs.jsonl lines the harness wrote; the table stays in build3ds/out.

## Gate
Every chunk rc 0 with 17 fxcena and 17 fxcenb lines in its log copy. A chunk without its until line, a log copy missing, or two
lines of one index: stop (tool fault, CLAUDE.md 4) and hand back the exact output. Budget 8 of 8.

## At the end
Delete this row file in the completing commit. Hand back: the hash, the table file path, and the top 15 lines of each ranking copied
as they are (no interpretation).
