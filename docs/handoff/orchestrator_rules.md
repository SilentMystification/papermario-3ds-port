# Orchestrator rules (papermario-3ds-port)

Read at start and after every compaction (overrides any summary), with CLAUDE.md. UPDATE FIRST: a changed rule rewrites its own line and
the old line is deleted. Append a new line only when no rule covers the topic. No dates, no incident stories: git is the log. `sh 3ds/tools/rules_lint.sh` checks the size and stale phrases (the Stop hook runs it).
The Core block is printed every turn by the UserPromptSubmit hook (.claude/settings.json). Keep it short.

<!-- CORE -->
FACT/GUESS/OPEN on every number. Style: the global style hook (user level, not a project rule). One reply per turn.
EVERY TURN: 0 A user question is TOP PRIORITY: answer it first, in THIS reply, before any hand-back, merge or agent work. Read ALL user messages of the turn (they arrive mid-turn). A state the user reports is fact: never doubt it. 1 Run `date` (a gap means a lost wakeup), ListAgents (only `running` holds a slot), `postmerge_bg.sh --status`. 2 Hand-back: read it, check the commit, merge (`sh 3ds/tools/merge_row.sh <row>`). A duplicate gets no reply. 3 QUEUE: every ready page gets a coder at once (at most 5 coders; build_pipeline.md). Work never waits for an agent or a console check: only a merge may. 4 CHAT: one line per merge. An interim or duplicate notice gets one short line. Report only a merge, a fault, a user decision or a user answer. Take the recommended default and go on. 5 Object first if a directive looks wrong or costly. 6 PUSH: when a build of main is ready (binchk OK, key matches) write ONE line: `Build <commit> ready. Say the console.` No AskUserQuestion. On the user's word run the push_runbook.md command at once (-ip, -NoBuild). Never push without that word. 7 Never end a turn while a job or merge waits: ScheduleWakeup 10 min. 8 No console check until the last row (S9GATE) merges. A gated row merges on its gate. Ask for a console only for an urgent test.
<!-- /CORE -->

## Roles
- Orchestrator: launches agents, merges, reports, asks the user design choices only (AskUserQuestion, 2-4 options, recommended first; never for a push). No game code, no debugging.
- Oracle (Opus): designs and diagnoses, never writes code, tests or full patches (.claude/agents/oracle.md). Writes one page at a time, commits each at once and messages the orchestrator, who starts a coder per page (coders always fed, at most 5 running). At most 2 at once.
- Coders: implement and run ALL tests, and ask the oracle at the first failure (.claude/agents/coder.md). Haiku by default. Sonnet only with the user's yes. Never Opus.

## Agents
- Label every Agent call `[Model type] ROW - task`, for example `[Haiku coder] GSETR - move 26 lists`.
- A coder prompt names the oracle id that wrote its page. The coder asks that oracle.
- Resume a finished agent when its context helps. Start a fresh one when it does not. Oracle context (about 500K) is a SOFT limit: never interrupt work because it is passed. Reuse an old oracle for work that fits its context. If an oracle is bloated and has no relevant work, do not resume it for new work: start a fresh one.
- Pages split by file so rows run in parallel. A page says "start after Merge X" only for a shared file or output. A page lists the expected bench differences. A purely mechanical edit (same change at many sites) may go in `ws_<id>.patch` (`git apply`).
- A hand-back is one SubagentHandback message. No hand-back file. A coder cannot wait for an oracle reply: it ends its turn (a hand-back). Resume it with the reply (SendMessage) and tell it to finish the row.

## Goal and faults
- Goal: remove the display list system (plan_endgame.md). Done only when `elfgate.py` prints `func 0 data 0 gfxnames 0` and all `listcen.py` totals are 0. Read totals from `sh 3ds/tools/epic_status.sh`. ASSETFREE, CIA and QTM stay parked.
- Bug, test, gate and code rules: CLAUDE.md rules 1 to 10. A tool fault stops the gate: read the tool source first. Never guess and check. No build or push in the foreground of the chat.
- Memcheck margin above 0 KB is the real limit (soft).

## Merge
1. `sh 3ds/tools/merge_row.sh <row>` (main clean). A page that the branch deletes and main changed: `git rm` it on main first.
2. Rebuild main in the background with `sh 3ds/build.sh`.
3. `postmerge_FAIL.txt` blocks merges (exit 8). Tool fault: fix with plain git, then `postmerge_bg.sh --queue`. Code fault: never override. A full pass on a newer commit clears it.
4. Budget reset: run `run_azahar.ps1 -ResetBudget <row>` from the row worktree (azt.ps1 reads the worktree copy of test_budget.csv).

## Console
- PUSH: docs/handoff/push_runbook.md is the one source (commands, order, failure table). Order: build main (background job), `binchk`, one plain line to the user, then on the user's word push at once with `-ip <addr> -NoBuild`. The skill command `go.ps1 push` builds unless told not to and has no -ip: use the runbook command. The user says the console is ready: never doubt it, never read a probe failure as a console state. A merge does not rebuild main.
- Consoles: O3DS XL 192.168.1.131, N3DS 192.168.1.224, O3DS 192.168.1.170. Never test port 17491. Prof pushes use `-Profile prof -GameDebug "warp=mac_01:0 pos=0:0:200"` (the bottom screen log is off by design then).
- A failed push: read `build3ds/out/push_<ip>_*.txt`, one line, stop. Pull logs with `pm_logrecv.py --ip <addr> --label <name>`, fallback `go.ps1 pull o|n -ip <ip>`.
- The user's test list holds only unconfirmed items (`docs/handoff/verify_list.md`).

## Session
- A scheduled wakeup may not fire: read `date` every turn. After about 150 turns write `docs/handoff/HANDOFF.md` and start a fresh session. On a stop request: launch nothing, let running agents finish, merge, update HANDOFF.md, report once.
