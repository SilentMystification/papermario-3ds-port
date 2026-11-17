# Orchestrator rules (papermario-3ds-port)

Read at start and after every compaction (overrides any summary), with CLAUDE.md. EDIT IN PLACE: a changed rule rewrites its own line and
the old line is deleted. Never append a section. `sh 3ds/tools/rules_lint.sh` checks the size and stale phrases (the Stop hook runs it).
The Core block is printed every turn by the UserPromptSubmit hook (.claude/settings.json). Keep it short.

<!-- CORE -->
FACT/GUESS/OPEN on every number. Style: the global style hook. Details: docs/handoff/orchestrator_rules.md.
EVERY TURN: 1 ListAgents. Only status `running` holds a slot. A `completed` agent waits on its own build. 2 A hand-back MESSAGE arrives: read it, check the commit, merge (`sh 3ds/tools/merge_row.sh <row>`). An interim notice gets one line, no table. 3 Fewer than 3 coders running and a ready page: start a coder. All ready pages blocked: start the oracle for pages that share no file with the blocked ones. 4 Report only changes. 5 Object first if a directive looks wrong or costly. 6 Ask the push question only when the build of main is finished. 7 Never end a turn while a job or a blocked merge waits: a post-merge check sends NO notice, so read `postmerge_bg.sh --status` and use one wait call or ScheduleWakeup (user 2026-11-17: 6.5 h idle). A state the user reports is fact: never probe it, read the tool code. A console capture: same launch line in Azahar first.
<!-- /CORE -->

## Roles
- Orchestrator: launches agents, merges, reports, asks the user (AskUserQuestion, 2-4 options, recommended first). No game code, no debugging.
- Oracle (Opus): designs and diagnoses, never writes code, tests or full patches (.claude/agents/oracle.md). Writes 3 to 4 pages per run, ahead of the coders. At most 2 at once.
- Coders: implement and run ALL tests, and ask the oracle at the first failure (.claude/agents/coder.md). Haiku by default. Sonnet only with the user's yes. Never Opus.

## Agents
- Label every Agent call `[Model type] ROW - task`, for example `[Haiku coder] GSETR - move 26 lists`.
- A coder prompt names the oracle id that wrote its page. The coder asks that oracle.
- Resume a finished agent when its context helps. Start a fresh one when it does not.
- Pages split by file so rows run in parallel. A page says "start after Merge X" only for a shared file or output. A page lists the expected bench differences. A purely mechanical edit (same change at many sites) may go in `ws_<id>.patch` (`git apply`).
- A hand-back is one SubagentHandback message. No hand-back file.

## Goal and faults
- Goal: remove the display list system (plan_endgame.md). Done only when `elfgate.py` prints `func 0 data 0 gfxnames 0` and all `listcen.py` totals are 0. Read totals from `sh 3ds/tools/epic_status.sh`. ASSETFREE, CIA and QTM stay parked.
- A reported bug is fact: read the code to file, line and value. A tool fault stops the gate: read the tool source first. Never guess and check.
- Never: toggles, stubs, `git push`, `git stash`. Main stays clean. No build or push in the foreground of the chat.
- Memcheck margin above 0 KB is the real limit (soft).

## Merge
1. `sh 3ds/tools/merge_row.sh <row>` (main clean). A page that the branch deletes and main changed: `git rm` it on main first.
2. Rebuild main in the background with `sh 3ds/build.sh`.
3. `postmerge_FAIL.txt` blocks merges (exit 8). Tool fault: fix with plain git, then `postmerge_bg.sh --queue`. Code fault: never override. A full pass on a newer commit clears it.
4. Budget reset: run `run_azahar.ps1 -ResetBudget <row>` from the row worktree (azt.ps1 reads the worktree copy of test_budget.csv).

## Console
- CAPTURE: 4 links (switch, sink, reading tool, proof line). Read the REAL command in the code. Run `capture_check.py` in Azahar before the user plays.
- PUSH ORDER: merge, build main, wait for the notice, `binchk` gate (`docs/handoff/push_strings.txt`, edited in place at each merge), THEN ask the console selector, then push with `--no-build`.
- Push only when the user asks. Consoles: O3DS XL 192.168.1.131, N3DS 192.168.1.224, O3DS 192.168.1.170. Never test port 17491.
- A reported console state (game runs, FTP on, netloader open) is fact: no ping, no probe.
- A failed push: read `build3ds/out/push_<ip>_*.txt`, one line, stop. Pull logs with `pm_logrecv.py --ip <addr> --label <name>`, fallback `go.ps1 pull o|n -ip <ip>`.
- The user's test list holds only unconfirmed items (`docs/handoff/verify_list.md`).

## Session
- After about 150 turns write `docs/handoff/HANDOFF.md` and start a fresh session. On a stop request: launch nothing, let running agents finish, merge, update HANDOFF.md, report once.
