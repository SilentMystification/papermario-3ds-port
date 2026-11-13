# Row <id>: <full name>

(Model: <Haiku>. 60 lines or fewer. The agent reads only this file, CLAUDE.md and what this file names.)

## User facts (bugs only)
What the user did and saw, in the user's words. The fault is fact (CLAUDE.md 1).

## Cause (oracle code read)
<file:line and value of each path that causes it. For a format change: every reader and writer of the format.>

## Start
`sh 3ds/tools/wt_new.sh <id>` (build3ds/wt/<id>, branch <id> from main). PM_ROW=<id> PM_LANE=<id> on every build and bench call.

## Touch only
Files: <paths>. Symbols: <symbol> in <file>. The oracle checks that no other running row edits the same places (merge conflicts).

## Exact edits
1. <file, function, the edit>

## Gate (CLAUDE.md 2: nd_host and byte compare only, no picture)
- `PM_ROW=<id> PM_LANE=<id> sh 3ds/build.sh > build3ds/out/build.txt 2>&1`, memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row <id> --why "<text>"`: PASS.
- `python 3ds/tools/bench.py cmp <bench> --row <id> --commit <merge-base>` after a run of <bench>: byte-equal (at most 2 scenes).
- The oracle picks the seed from the code read, before any run, never by trial. A seed is live only when (1) its call runs in
  the frames the gate records, with each guard on its path shown true for the scene's spawn arguments, and (2) the draw reads
  the value. Prefer the flags of a matrix call with no guard (G_MTX_LOAD <-> G_MTX_MUL): every draw after it moves. Use a colour
  only when the combine of the list drawn next names that channel (quote its gsDPSetCombineLERP or PM_CC line), and the blender
  writes it. Never nd_val_seg: the lists hold host addresses, so no draw reads the segment base. The coder proves the rebuild by
  the ELF sha1 (the build number does not change for an uncommitted edit).
- A new gate has a seeded fault first. The seed changes a value (a constant, a length, a mask). It never deletes a statement or a
  call: with -Werror an unused variable, parameter or function then stops the build before the gate runs. State the exact FAIL set.
- A row that adds a debug switch or a new scene: its merge-base cannot run the scene. Base the new scene at the row's own first
  real-code commit, prove the seed against that base, and prove "no draw changes" with an old scene against the merge-base.

## Stop
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, then hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash, the gate results, the memcheck line.
