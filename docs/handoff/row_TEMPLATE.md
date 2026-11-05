# Row <id>: <full name>

(60 lines or fewer. The orchestrator copies this file to row_<id>.md and fills it. The agent reads only this file and what it names.)

## Goal
<2 to 3 lines: what changes and why.>

## Start state
Branch: <name>. Last commit: <hash>. Tree: <clean or named user files>.
STATE from the previous agent (paste, 10 lines or fewer):

## Open before you start (blockers are work)
- <blocker>: route to clear it: <command or file>.

## Touch only
- Files: <paths>
- Symbols (grep, a line shift is not a stop): <symbol> in <file>
- Plan sections to read: docs/handoff/<plan>.md section <n>

## Done criteria (all must pass; handback only then)
1. <gate: host test command, expected result>
2. <scenario: powershell -File 3ds/tools/azt.ps1 <scenario> -Row <id>, the quantity and the picture that prove it>
3. <console prof check if the user asks>
Affected scenarios (no others): <list>

## Rules that apply
CLAUDE.md rules 5, 7, 9. No toggles, no stubs, no git push, no attribution. Commit after each passed row. Never commit 3ds/src/map_restore_table.c.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=<id> sh 3ds/build.sh; then restore 3ds/src/map_restore_table.c.
- Host tests: sh 3ds/tools/nd_host_test.sh
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget <id>

## Tests
Entry point: python 3ds/tools/bench.py (process: docs/handoff/test_catalog.md, situations A to F). Name the benches here: <bench list>. Numbers go to the database, not to prose.

## At the end
Overwrite docs/handoff/agent_<epic>.md (do not append, do not create a new file) with a STATE block at the top (10 lines or fewer): branch, last commit, done rows, open rows, next command. Delete this row file in the commit that completes the row (CLAUDE.md rule 11).
