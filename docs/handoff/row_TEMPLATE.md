# Row <id>: <full name>

(Model: <Haiku>. 60 lines or fewer. The agent reads only this file, CLAUDE.md and what this file names.)

## User facts (bugs only)
What the user did and saw, in the user's words. The fault is fact (CLAUDE.md 1).

## Cause (oracle code read)
<file:line and value of each path that causes it. For a format change: every reader and writer of the format.>

## Start
`sh 3ds/tools/wt_new.sh <id>` (build3ds/wt/<id>, branch <id> from main). PM_ROW=<id> PM_LANE=<id> on every build and bench call.

## Touch only
Files: <paths>. Symbols: <symbol> in <file>. The oracle checks that no other running row edits these files (merge conflicts).

## Exact edits
1. <file, function, the edit>

## Gate (CLAUDE.md 2: nd_host and byte compare only, no picture)
- `PM_ROW=<id> PM_LANE=<id> sh 3ds/build.sh > build3ds/out/build.txt 2>&1`, memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row <id> --why "<text>"`: PASS.
- `python 3ds/tools/bench.py cmp <bench> --row <id> --commit <merge-base>` after a run of <bench>: byte-equal (at most 2 scenes).

## Stop
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, then hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash, the gate results, the memcheck line.
