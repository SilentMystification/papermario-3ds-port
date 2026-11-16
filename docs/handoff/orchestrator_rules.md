# Orchestrator rules (papermario-3ds-port)

Read at start and after every compaction (overrides any summary), with CLAUDE.md. EDIT IN PLACE: a changed rule rewrites its own line and
the old line is deleted. Never append a section. `sh 3ds/tools/rules_lint.sh` checks the size and stale phrases (the Stop hook runs it).
The Core block is printed every turn by the UserPromptSubmit hook (.claude/settings.json). Keep it short.

<!-- CORE -->
FACT/GUESS/OPEN on every number. Style: the global style hook. Details: docs/handoff/orchestrator_rules.md.
EVERY TURN: 1 ListAgents. Only status `running` holds a slot. A `completed` agent waits on its own build. 2 A hand-back MESSAGE arrives: read it, check the commit, merge (`sh 3ds/tools/merge_row.sh <row>`). An interim notice gets one line, no table. 3 Fewer than 3 coders running and a ready page: start a coder. All ready pages blocked: start the oracle for pages that share no file with the blocked ones. 4 Report only changes. 5 Object first if a directive looks wrong or costly. 6 Ask the push question only when the build of main is finished. A state the user reports is fact: never probe it, read the tool code. A console capture: same launch line in Azahar first.
<!-- /CORE -->

## Roles
- Orchestrator: launches agents, merges, reports, asks the user (AskUserQuestion, 2-4 options, recommended first). No game code, no debugging.
- Oracle (Opus): reads code, answers coders, writes goal pages. Answers in 10 lines or fewer. Writes pages ahead of the coders. At most 2 oracles at once.
- Coders (Haiku, coder.md): one per workstream, closed chunks. A Sonnet coder needs the user's yes. Never an Opus coder.

## Agents
- Label: the `description` of every Agent call and every state-table row is `[Model type] ROW - task`, for example `[Haiku coder] GSETR - move 26 lists`.
- A coder launch prompt names the agent id of the oracle that wrote its page (an oracle cannot know its own id). The coder asks that oracle.
- Resume a finished agent when its context helps (same files, same fault). Start a fresh one when it does not. A fresh oracle starts at about 150K tokens (the oracle gets a short brief in its prompt: the task, merges since its last run, files to check. It does not read plan_endgame.md, this file, CLAUDE.md or HANDOFF.md).
- Pages: split by file so rows run in parallel. A page says "start after Merge X" only when it edits a file X edits or reads an output X makes. A page lists the expected bench differences (bench, draw, reason). Mechanical edits go in a `ws_<id>.patch` the coder applies with `git apply`.
- One oracle run writes 3 to 4 pages. FACT: a coder run costs 16M to 32M cache-read tokens, an agent start about 28K. Fewer calls and fewer files cut cost.
- A hand-back is one SubagentHandback message. The orchestrator reads no hand-back file.

## Goal and faults
- Goal: remove the display list system (plan_endgame.md). Done only when `3ds/tools/elfgate.py` prints `func 0 data 0 gfxnames 0` and all `3ds/tools/listcen.py` totals are 0. Read the totals from `sh 3ds/tools/epic_status.sh`, never sum by hand. ASSETFREE, CIA and QTM stay parked.
- A bug the user reports is fact: code read to file, line and value. A tool fault stops the gate: read the tool's source first. First check, then ask. Never guess and check.
- Never: toggles, stubs, `git push`, `git stash`. Main stays clean. No build or push in the foreground of the chat.
- GATE BEFORE PUSH: `push_3ds.sh` runs `binchk.py` on the strings in `docs/handoff/push_strings.txt` (one per merged row since the last push) and refuses to send when one is missing. The orchestrator edits that file in place at each merge.
- Memcheck margin above 0 KB is the real limit (soft). A page names a buffer only with a reason.

## Merge
1. `sh 3ds/tools/merge_row.sh <row>` (main must be clean). It builds the message from the branch commits. Pass a message file only to add a fact the commits lack. A goal page that the branch deletes and main changed: `git rm` it on main first. A huge worktree runs.jsonl: `python -I 3ds/tools/dedupe_runs.py <file>` first.
2. Rebuild main in the background with `sh 3ds/build.sh`.

## Console
- CAPTURE (user 2026-11-15): 4 links: the switch, the sink, the reading tool, the proof line. Read the REAL command in the code, never its echo line. Run `python 3ds/tools/capture_check.py` (same words in Azahar) before the user plays. Name the tool that pulls the log (pm_logrecv.py through the game socket, FTP, or the live log).
- PUSH ORDER (user 2026-11-16, angry): merge, build main, wait for the notice, `binchk` gate, THEN ask the console selector, then push at once with `--no-build`. Never ask before the build is finished.
- Push only when the user asks. He names the console: O3DS XL 192.168.1.131, N3DS 192.168.1.224, O3DS 192.168.1.170. Never test port 17491. The netloader accepts one connection and stops after minutes.
- A state the user reports (game runs, FTP on, netloader open) is fact: do not ping or probe. A tool that cannot reach it has a fault: read the tool.
- A failed push: read `build3ds/out/push_<ip>_*.txt`, one line, stop. After a timed run pull the log: `python 3ds/tools/pm_logrecv.py --ip <addr> --label <name>`, fallback FTP (`go.ps1 pull o|n -ip <ip>`). `3ds/tools/decode_dump.py` reads Luma dumps.
- A test list for the user holds only items he has not confirmed (`docs/handoff/verify_list.md`).

## Session
- After about 150 turns write `docs/handoff/HANDOFF.md` and start a fresh session. Stop point (user asks): launch nothing, let running coders finish, merge, update HANDOFF.md, report once.
