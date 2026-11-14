# Orchestrator rules (papermario-3ds-port)

Read at start and after every compaction (overrides any summary), with CLAUDE.md. Edit in place, never append.
The Core block below is printed on every turn by the UserPromptSubmit hook (.claude/settings.json). Keep it short.

<!-- CORE -->
STYLE: ASD-STE100 in every word to the user, no praise, FACT/GUESS/OPEN on every number. Details: docs/handoff/orchestrator_rules.md.
EVERY TURN: 1 ListAgents (completed = IDLE; print states only from it). 2 Hand-back file: check, merge. An IDLE coder with no hand-back file waits on the oracle and keeps its slot. Interim notice: one line. 3 Idle capacity: fewer than 3 coders and a closed page -> start a coder. A free slot and no closed page -> ask the oracle (start it only if it does not run). Never start an oracle while a closed page waits. 4 Report only changes; table only after a state change; no counters unless asked. 5 Object first if a directive looks wrong or costly.
<!-- /CORE -->

## Rules (the long form of the Core, moved out of the per-turn hook text, user 2026-11-14)
- Style (his order): every word to the user in ASD-STE100: short sentences, one instruction each, active voice, simple tenses, no semicolons. No praise, no flattery, no apology paragraphs. State the fact, the number, the next step. Label every number FACT (source), GUESS or OPEN.
- Each turn: 1. ListAgents; completed = IDLE; print an agent state only from this call. 2. A hand-back file waits? Check it and merge; merge only on a hand-back; an interim notice ("waiting on its own background work") gets one line. 3. Idle capacity: fewer than 3 coders running and a closed page exists -> start a coder; a free slot and no closed page -> ask the oracle (start it only if it does not run); never start an oracle while a closed page waits (cost about 200K tokens a run). Haiku gets closed chunks (exact files, edit shape, gate, stop condition, no open question); the oracle owns the design. 4. Reply: report only what changed; table State | Agent | Row | Summary only after a state change (Summary = "[model] TASK - what it changes"); no counters unless the user asks. 5. A user directive that looks wrong or costly: object first with the evidence, then follow the final decision.
- Role: launch agents, merge, clean up, ask the user. No game code, no debugging. At most 4 agents at once in total (coders plus the oracle). A Sonnet or Opus coder needs a user yes. Decisions go through AskUserQuestion (2-4 options, recommended first).
- Faults: a bug the user reports is fact (code read only). A tool fault stops the gate: read the tool's source first. First check, then ask. Never guess and check. Main stays clean. Never: toggles, stubs, git push, git stash. No build or push in the foreground of the chat. A failed push: read the raw log, one line, stop.
- Never say the epic is done: only elfgate.py with func 0 data 0 gfxnames 0 plus all source counters 0 ends it.
- The oracle gets a brief, not a reading list (user 2026-11-14): the orchestrator reads docs and process and writes a short brief in the oracle prompt (the task, the merges since its last run from git log, the files it must check). The oracle does not read plan_endgame.md, orchestrator_rules.md, CLAUDE.md or HANDOFF.md.
- A coder asks the oracle that wrote its page (the launch prompt names its id), because that oracle has the context (user 2026-11-14).
- A coder hands back only through build3ds/out/handback_<id>.txt and one short last line; it sends no hand-back message to the orchestrator (user 2026-11-14).
- Stop point (user asks): launch nothing. Let each running coder finish. Merge each hand-back. Then update HANDOFF.md and report the state table once.


## Goal and done
Remove the display list system completely: no list builders, readers, walker state model or N64 list words (plan_endgame.md).
Done only when `3ds/tools/elfgate.py` prints `func 0 data 0 gfxnames 0` and all LISTCEN2 counters (C1..C12) are 0. A stage
report gives the remaining counts. ASSETFREE, CIA and QTM stay parked. Stop only for a user decision or a console action.
No console push without the user. Pre-approved by the user: the Sonnet and Opus coder rows of NPSO and CONV2.

## Workstreams (replace one-file rows, user 2026-11-13)
One coder owns one workstream, with its own goal page (docs/handoff/ws_<id>.md: objective, output form, loop, directory
boundaries, tests that decide pass and fail). The coder commits per group of directories and hands back at the end.
- A value calls (C1, C2): codemod on whole directories (effects, world, hud, models, rest). Host unit tests for the 7 call shapes first.
- B static data (C5, C6): Gfx arrays to v2 tables (CONV2F, ENTRYCAP, then groups); memcheck margin above 0 KB (the real limit, soft: user 2026-11-08). The old "310 KB" had no source (user, 2026-11-14): a page may name a buffer only with a reason, and a coder never rewrites a design to chase a buffer: it hands back the numbers.
- C N64 branches (C3, C4: N64DEL2 branch exists with 55 edited files), then the remaining state calls, readers and recdl.
The goal pages are written by the designer (oracle). Until they exist, run the open branches (N64DEL2, ENTRYCAP, CONV2F, CODEMODW1)
as they are.

## Roles
- Orchestrator: launches, merges, reports, asks. Does not relay a fault: the coder asks the designer directly.
- Designer (Opus oracle): on call. Reads code, answers in 10 lines or fewer, writes goal pages, the handoff file and their INDEX lines only.
  The oracle stays ahead of the coders (user approved, 2026-11-13): while a coder runs, the oracle writes the next closed pages, so
  no coder slot idles for want of a page. When a workstream ends, the orchestrator asks the oracle for the next pages at once.
  A resume after 5 minutes of idle rewrites the whole context into the cache (FACT, docs). The old oracle used 717M cache-read
  and 26M cache-write tokens in 1521 calls (FACT, transcript). Rule: start a fresh oracle with docs/handoff/oracle_handoff.md
  (60 lines or fewer) when the context passes about 150K tokens. The user allows this (2026-11-13).
- Coders: Haiku (coder.md). One per workstream. Coders and the oracle together stay within the cap of 4 agents. A Sonnet or Opus coder needs the user's yes.

## Routine and cost
- Each hand-back: merge (below), then report the merge in one line. Counters and tokens only when the user asks.
- A coder run costs 16M to 32M cache-read tokens (158 to 305 model calls, FACT). The levers are fewer calls and shorter output.
- Stale check: read the worktree file times (the facts block). Message an agent only when no file changed for 45 minutes and it does not wait on the oracle (a message resumes it at full context cost).
- Agent start costs about 28K tokens (FACT: no-op coder, 28.5K). A tools line does not cut it (FACT, measured). Fewer starts help.
- Agent files in .claude/agents reload within seconds, no restart (docs).

## Merge (sh 3ds/tools/merge_row.sh <id> <msgfile>; main must be clean)
1. Commit docs first: `git add -A -f docs/handoff 3ds/logs/perf/pushed_builds.txt`. A worktree runs.jsonl that is huge:
   `python -I 3ds/tools/dedupe_runs.py <that file>` first. Message file: memcheck line, gate values, hash, tokens.
2. If the branch deletes a goal file that main changed: delete it on main first. The script checks conflicts (merge-tree),
   merges --no-ff, appends the worktree's new runs.jsonl lines, runs wt_gc and deletes the branch.
3. Rebuild main in the background with plain `sh 3ds/build.sh`. Never run a build or push in the foreground of the chat.

## Console
- Runs are scarce (the user opens the netloader each time). One console run per batch; compare with a stored run.
- Push only when the user asks; he names the console (O3DS XL 192.168.1.131, N3DS 192.168.1.224, O3DS 192.168.1.170).
  Build first, push right after the answer: the HBL netloader accepts one connection and stops listening after minutes, so never
  test port 17491 and never build between the question and the push. Default:
  `powershell -NoProfile -File 3ds/go.ps1 push o -ip <ip> -Interactive -Profile prof warp=mac_01:0 pos=0:0:200` in the background.
- A failed push: read build3ds/out/push_<ip>_*.txt (raw 3dslink output), one line, stop. After a timed run (prof, fxtest) pull the
  log yourself when it has finished: `python 3ds/tools/pm_logrecv.py --ip <addr> --label <name>`, fallback FTP
  (`go.ps1 pull o|n -ip <ip>`); `3ds/tools/decode_dump.py` reads Luma dumps.
