# Row MRTGEN: generate 3ds/src/map_restore_table.c in the build, untrack it

## User facts
User (2026-11-09): "Main has uncommitted items. These should be committed or gitignored. Why are we not committing the map restore
files?" Decision: "Generate it in build.sh". Found: gen_map_restore.py writes the same content as git (only LF vs CRLF line endings
differ), so the file always looked modified; the old rule made every build back it up and restore it. Its .ld outputs
(3ds/ld/map_restore.ld, map_restore_bss.ld) are already generated and gitignored.

## Start state
Worktree build3ds/wt/MRTGEN, branch MRTGEN from main. PM_ROW=MRTGEN, PM_LANE=MRTGEN. One Haiku agent. Tools and docs only.

## Steps
1. 3ds/build.sh runs `python 3ds/tools/gen_map_restore.py` before configure (in the worktree root of the build), for every lane and
   the main build. gen_map_restore.py writes LF line endings and writes the file only when the content changes (no mtime churn).
2. `git rm --cached 3ds/src/map_restore_table.c`; add it to .gitignore next to the .ld outputs.
3. Delete the "back up / restore map_restore_table.c" and "never commit 3ds/src/map_restore_table.c" text from CLAUDE.md (rules 7,
   9), docs/handoff/orchestrator_rules.md, docs/handoff/ORCH_RULES.txt, row_TEMPLATE.md and any tool that backs it up
   (grep -rn "map_restore_table" 3ds docs CLAUDE.md). Rule 9 says instead: build.sh generates it from 3ds/map_restore.txt.
4. Keep `gen_map_restore.py learn` unchanged (it edits 3ds/map_restore.txt, which stays tracked).

## Gates
- A clean worktree (no map_restore_table.c) builds with PM_LANE=MRTGEN; memcheck line equal to main's.
- The main build (no PM_LANE) after the merge: git status shows no change to map_restore_table.c.
- The generated file equals the old tracked file except line endings (compare with `tr -d '\r'`).
