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
New names: <every new bench and scenario name, or "none">. Host link: <the host test that links a touched 3ds/src file, or "none">.
Needs: <path> has <text>, one line per data state a gate needs on main (a verdict, a stored base, a merged tool), or none.
(rowlint.py reads these lines.)

## Exact edits
1. <file, function, the edit>

## Gate (CLAUDE.md 2: nd_host and byte compare only, no picture)
- `PM_ROW=<id> PM_LANE=<id> sh 3ds/build.sh > build3ds/out/build.txt 2>&1`, memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row <id> --why "<text>"`: PASS.
- A codemod or conversion row uses the drec gate (`drec=<first>:<count>[:<step>]` in frames where each file draws, drec.txt
  byte-equal), not the pixel gate: no agent may count pixels, and drec records every draw of the window, seen or not
  (CODEMODFX10). The draw proof: a line the base run writes anyway (fxtest: `fxtest: live 1` and `fxcenb <id> tri` above the
  `fxcenb -1 tri` base window).
- The seed (picked from the code read before any run): + 20.0f on the y argument of the first guTranslateF or guPositionF of the
  file's draw function with no branch above it (every draw of the file moves; drec records the matrix uniforms). Not a
  G_MTX_LOAD/MUL flip on a first matrix (the stack top is the identity), not a colour the combine does not read, never
  nd_val_seg (the lists hold host addresses). The coder proves the rebuild by the ELF sha1 (the build number does not change).
- A new gate has a seeded fault first. The seed changes a value; it never deletes a statement or a call (-Werror).
- A row that adds a debug switch or a new scene bases that scene at its own first commit and proves "no draw change" with an old
  scene against the merge-base. The first row of a new kind is a pilot: the oracle reads its result before it writes copies.

## Row text faults the coder fixes (CLAUDE.md 5; note the fix in the commit message)
- Dead seed (the ELF sha1 changed, the gate shows no difference): take the next transform of the same function by the seed rule
  above (the next unguarded guTranslateF, guPositionF or guScaleF; for guScaleF multiply its y argument by 2.0f). If the
  function has none: the first matrix call pushed under an earlier push of the same function, G_MTX_MUL <-> G_MTX_LOAD. If none:
  stop and hand back.
- Name clash (bench.py: "defined in both"): rename the row's new name to `<name>_<id in lower case>` in its azt.d and bench.d
  files, then base the renamed scene.
- Scene does not draw the file (the draw proof fails): an effect file `<n>.c` uses fxtest id = the index of `E(<n>,` in FXLIST
  (3ds/src/pm_fxtest.c, from 0) in a copy of fxpic_89. A map file of src/world/area_<a>/<map>/ uses `warp=<map>:0`. Else stop.
- A line number that moved: find the quoted text in the same function and use its line.

## Stop
A tool fault, a difference of the real edit (drec or bytes), a counter that does not fall, or an edit that does not fit the code:
commit, then hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash, the gate results, the memcheck line.
