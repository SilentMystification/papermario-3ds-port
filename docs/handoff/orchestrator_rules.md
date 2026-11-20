# Orchestrator rules (papermario-3ds-port)

Read at start and after every compaction (overrides any summary), with CLAUDE.md. UPDATE FIRST: a changed rule rewrites its own line and
the old line is deleted. Append a new line only when no rule covers the topic. No dates, no incident stories: git is the log. `sh 3ds/tools/rules_lint.sh` checks the size and stale phrases (the Stop hook runs it).
The Core block is printed every turn by the UserPromptSubmit hook (.claude/settings.json). Keep it short.

<!-- CORE -->
FACT/GUESS/OPEN on every number. Style: the global style hook. One reply per turn.
EVERY TURN: 0 A user question is TOP PRIORITY: answer it first, in THIS reply. Read ALL user messages of the turn. A state the user reports is fact. 1 Run `date` and ListAgents (the dispatcher runs or not). 2 You start the coaches, design oracles and mergers (dispatcher.md holds the duties, no dispatcher agent runs). Keep build3ds/out/dispatch_state.md. A reply to the user is ONE line per merge, fault or decision (take its default). 3 No foreground command over 10 s. 4 CHAT: report only a merge, a fault, a user decision or a user answer. No echo of notices. 5 Object first if a directive looks wrong or costly. 6 PUSH: when a build of main is ready write ONE line `Build <commit> ready. Say the console.` No popup. On the user's word run the push_runbook.md command at once (-ip, -NoBuild). Never push without that word. 7 Never poll: no ScheduleWakeup and no status turn while agents run. The harness sends a notice when an agent ends or stops on a background job. EXCEPTION (user, while away): ONE hourly watchdog wakeup (3600 s) that checks ListAgents; if no agent runs and work remains, find the cause, log it in build3ds/out/stuck_log.md, fix the process and restart the work. 8 No console check until S9GATE merges. 9 SATURATE: every turn compare ListAgents with the quota of 5 coders. A free slot, or no design oracle writing the next pages, is a fault: start the work at once (dispatcher.md SATURATE). A coder waiting on a merge is not a reason to idle: pages are written ahead from the branch data.
<!-- /CORE -->

## Roles
- Orchestrator (Sonnet): talks to the user, starts the coaches, design oracles and mergers (dispatcher.md duties), asks the user design choices only (AskUserQuestion, 2-4 options, recommended first; never for a push), does the push. No game code, no debugging, no coder launches (a coach starts them).
- Dispatcher: removed (coders and coaches report to the main chat anyway). Its duties in dispatcher.md belong to the orchestrator.
- Oracle (Opus, oracle.md): ONE CLUSTER LEAD per topic. It designs the pages of its topic, reads ahead, starts the coders and the design helpers of that topic, and writes no code. Its agents all relate to its topic.
- Coders (Haiku, coder.md): implement and run ALL tests, ask their coach at the first failure. Sonnet only with the user's yes. Never Opus.

## Agents
- Orchestrator launches: label every Agent call `[Model type] ROW - task`, for example `[Haiku coder] GSETR - move 26 lists`.
- A coder prompt names its coach oracle id. The coder asks that coach.
- ROUTING: a new task that relates to a running or finished cluster lead goes to that lead (SendMessage resumes a finished one, it keeps its context). A task that does not relate goes to a NEW cluster lead. The orchestrator never mixes topics in one lead. A lead whose context is bloated and has no relevant work stays idle. Oracle context (about 500K) is a SOFT limit: never interrupt work because it is passed.
- Pages split by file so rows run in parallel. A page says "start after Merge X" only for a shared file or output. A page lists the expected bench differences. A purely mechanical edit (same change at many sites) may go in `ws_<id>.patch` (`git apply`).
- A hand-back is one SubagentHandback message. No hand-back file. A coder cannot wait for an oracle reply: it ends its turn (a hand-back). Resume it with the reply (SendMessage) and tell it to finish the row.

## Goal and faults
- Goal: remove the display list system (plan_endgame.md). Done only when `elfgate.py` prints `func 0 data 0 gfxnames 0` and all `listcen.py` totals are 0. Read totals from `sh 3ds/tools/epic_status.sh`. ASSETFREE, CIA and QTM stay parked.
- Bug, test, gate and code rules: CLAUDE.md rules 1 to 10. A tool fault stops the gate: read the tool source first. Never guess and check. No build or push in the foreground of the chat.
- Memcheck margin above 0 KB is the real limit (soft).

## Merge (a Haiku merger started by the orchestrator runs merge_protocol.md)
1. `sh 3ds/tools/merge_row.sh <row>` (main clean).
2. Rebuild main in the background with `sh 3ds/build.sh` before a push.
3. `postmerge_FAIL.txt` blocks merges (exit 8). Tool fault: fix with plain git, then `postmerge_bg.sh --queue`. Code fault: never override. A full pass on a newer commit clears it.
4. Budget reset: run `run_azahar.ps1 -ResetBudget <row>` from the row worktree (azt.ps1 reads the worktree copy of test_budget.csv).

## Console
- PUSH: docs/handoff/push_runbook.md is the one source (commands, order, failure table). Order: build main (background job), `binchk`, one plain line to the user, then on the user's word push at once with `-ip <addr> -NoBuild`. The skill command `go.ps1 push` builds unless told not to and has no -ip: use the runbook command. The user says the console is ready: never doubt it, never read a probe failure as a console state. A merge does not rebuild main.
- Consoles: O3DS XL 192.168.1.131, N3DS 192.168.1.224, O3DS 192.168.1.170. Never test port 17491. Prof pushes use `-Profile prof -GameDebug "warp=mac_01:0 pos=0:0:200"` (the bottom screen log is off by design then).
- A failed push: read `build3ds/out/push_<ip>_*.txt`, one line, stop. Pull logs with `pm_logrecv.py --ip <addr> --label <name>`, fallback `go.ps1 pull o|n -ip <ip>`.
- The user's test list holds only unconfirmed items (`docs/handoff/verify_list.md`).

## Session
- A scheduled wakeup may not fire: read `date` every turn. After about 150 turns write `docs/handoff/HANDOFF.md` and start a fresh session. On a stop request: launch nothing, let running agents finish, merge, update HANDOFF.md, report once.
