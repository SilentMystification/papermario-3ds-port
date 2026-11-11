# Row TOOLB8: base --fill accepts a main build whose inputs equal HEAD's (docs-only commits after the build)

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts (2026-11-11)
Main build ran (build3ds/out/main_build2.txt, exit 0). `bench.py base --fill` then printed "build3ds/pm_3ds.3dsx is build 3999 git
141c5e371, main HEAD is 3c2b9ced2: build main first. Nothing was run." Main HEAD moved by docs commits only.
## Cause (oracle code read)
- build.sh:141-143 skips the build when build_key.txt equals the tree key (key_now, :98-105: tracked inputs of KEY_IN :96 by tree id,
  the diff, untracked files, asset sizes and times). docs/ is not in KEY_IN, so a docs commit keeps the key.
- build.sh:150 writes pm_build.h (PM_GIT_HASH) only after the skip check: a skipped build keeps the header of the 3dsx it kept.
- bench.py base_fill :1742-1745 requires the header commit to equal HEAD (`same_commit(hcommit, head)`), so every docs commit after a
  build refuses the fill. build_stale("") (:905-924, TOOLB1) already proves "the main build is built from the inputs of HEAD".
## Start
`sh 3ds/tools/wt_new.sh TOOLB8` (build3ds/wt/TOOLB8, branch TOOLB8 from main). Rows running: HC8 and HC7 do not edit bench.py.
## Touch only
3ds/tools/bench.py, function base_fill only (the `if not same_commit(hcommit, head):` block).
## Exact edit
Replace
```
    if not same_commit(hcommit, head):
        print("base --fill: build3ds/pm_3ds.3dsx is build %d git %s, main HEAD is %s: build main first. Nothing was run." % (bld, hcommit, head[:9]))
        return 5
```
with
```
    if not same_commit(hcommit, head):
        stale = build_stale("")  # TOOLB8: a docs-only commit keeps the build key; build.sh skips and keeps the old header (build.sh:141-150)
        if stale:
            print("base --fill: build3ds/pm_3ds.3dsx is build %d git %s, main HEAD is %s, and %s. Nothing was run." % (bld, hcommit, head[:9], stale))
            return 5
        print("base --fill: build %d git %s has the build inputs of HEAD %s (same build key): stored under HEAD." % (bld, hcommit, head[:9]))
```
The rest of base_fill (stores and records under `head`) stays: base_run passes `head` as the commit, and resolve_build (:707-718)
takes the given commit before the log's.
## Gate (tool row: no memcheck, no Azahar start)
- `python 3ds/tools/bench.py run nd_host --row TOOLB8 --why "TOOLB8 import check"`: PASS.
- `python -c "import ast; ast.parse(open('3ds/tools/bench.py',encoding='utf-8').read())"`: no error.
- In the worktree: `python -c "import sys; sys.path.insert(0,'3ds/tools'); import bench; print(repr(bench.build_stale('')))"` prints a
  STALE BUILD text or '' (it runs; no build3ds/build_key.txt in a new worktree gives the STALE text). The fill itself is the
  orchestrator's next main-folder step after the merge.
## Stop and end
A failed gate or an edit that does not fit the code: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash and the gate results.
