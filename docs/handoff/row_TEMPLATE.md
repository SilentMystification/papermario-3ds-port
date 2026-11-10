# Row <id>: <full name>

(60 lines or fewer. The agent reads only this file and what it names.)

## User facts (bugs only, required)
Trigger sequence: <inputs, menus, choices>. First wrong frame: <event or frame>. Expected look: <user words>. Reproducing scenario: <name, or "add one first">.

## Goal
<2 to 3 lines.>

## Start state
Worktree build3ds/wt/<id>, branch <id> from main <hash>. Copy assets/us and ver/us/build into it. STATE from the previous agent (10 lines or fewer):

## Cause (oracle code read of how it draws)
<file:line and value of each path; the fix below changes each one. No debug-log or test step to confirm it.>

## Touch only
Files: <paths>. Symbols: <symbol> in <file>.

## Done criteria (all must pass; handback only then)
1. <gate: command, expected result>
Affected scenarios (no others): <list>

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=<id> on every build.sh and bench call. No git stash. Wait for background runs in the foreground. Commit after each passed step.

## Commands
- Build: PM_ROW=<id> PM_LANE=<id> sh 3ds/build.sh (it generates 3ds/src/map_restore_table.c).
- Test: PM_ROW=<id> PM_LANE=<id> python 3ds/tools/bench.py run <bench> --row <id> --lane <id> --why "<text>"
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget <id>

## At the end
Delete this row file in the commit that completes the row. SendMessage main "<id> ready <hash>". The orchestrator merges and removes the worktree.
