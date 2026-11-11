# Row FXCEN3: fxcen_report.py subtracts each chunk's own base window

(Model: Haiku. Base: main after FXCEN2. Tool only: 3ds/tools/fxcen_report.py. Disjoint from every running row. Reads this file and CLAUDE.md.)

## Fact (oracle code read)
- Each chunk log (fxcen_c0 .. fxcen_c7, one boot each) has its own base line (fxcena/fxcenb with index -1, frames 360-400).
  fxcen_report.py joins all logs into one dict by index (`rec`), so `base = rec[-1]` (:72) is the base of the last log read, and every
  effect of chunks 0-6 is measured against chunk 7's base. effect_63 (index 93, chunk 5) shows negative draws, tris and indices for that
  reason; every chunk-0-6 row is off by its chunk's base difference.

## Start
`sh 3ds/tools/wt_new.sh FXCEN3`. PM_ROW=FXCEN3 on every call. No game build.

## Exact edits (3ds/tools/fxcen_report.py)
1. Parse each log file on its own: per file a dict index -> record and its own base (index -1); a file without a base: exit with
   `fxcen_report: no base line (fxcena -1) in <file>`. per_frame(r, base) of each effect uses the base of its own file.
2. The base line of the output lists every file's base: `base <file name>: frames %d us per frame %.1f draw per frame %.2f`.
3. The docstring (:9-10) says the base is per log.

## Gate
- `python 3ds/tools/fxcen_report.py <the 8 FXCEN2 logs> --label azahar` (the logs: build3ds/azt/logs/fxcen_c0.txt .. c7.txt in the
  FXCEN2 worktree, else in the main folder; if none exist, hand back: no new Azahar run in this row): exit 0; the effect_63 row has no
  negative draw, tri or nidx value; write the new top 15 by draw into the commit message.
- `git diff main --stat`: only fxcen_report.py.

## Stop and end
An edit that does not fit or no log files: commit, hand back the exact output. Delete this row file in the completing commit. Hand
back the hash and the new top-15 lists.
