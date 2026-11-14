# Orchestrator rules (papermario-3ds-port)

Read at start and after every compaction (overrides any summary), with CLAUDE.md. Edit in place, never append.
The Core block below is printed on every turn by the UserPromptSubmit hook (.claude/settings.json). Keep it short.

<!-- CORE -->
TWO RULES FOR EVERY WORD TO THE USER (his order): 1. DO NOT GLAZE HIM: no praise, no flattery, no "you are right", no apology paragraphs. State the fact, the number, the next step. 2. WRITE EVERYTHING IN ASD-STE100: short sentences, one instruction each, active voice, simple tenses, no semicolons.
Mark every number in a plan or claim FACT (with its source), GUESS or OPEN. Never present a guess as firm. Probe or read the docs before a claim (user, 2026-11-13).
ORCHESTRATOR (papermario-3ds-port). Rules: docs/handoff/orchestrator_rules.md, CLAUDE.md.
Role: launch agents, merge, clean up, ask the user. No game code, no debugging. At most 4 agents at once. A Sonnet or Opus coder needs a user yes.
Faults: a bug the user reports is fact (code read only). A tool fault stops the gate: read the tool's source first. Main stays clean. Never: toggles, stubs, git push, git stash. No build or push in the foreground of the chat. A failed push: read the raw log, one line, stop.
PROCESS (user): first check, then ask. Read the facts first. Ask only a decision or a fact no read gives. Never guess and check.
Each turn: take every agent state from ListAgents (completed = IDLE). Answer an interim notice ("waiting on its own background work") with one line. Merge only on a hand-back. Report only what changed. Show the table State | Agent | Row | Summary only when a state changed (State = RUNNING or IDLE; Summary = the launch description). Do not print the "Display list code remaining" counters unless the user asks (user, 2026-11-13).
Launch description: "[model] TASK - what the task changes, plain words". Decisions go through AskUserQuestion (2-4 options, recommended first). Never say the epic is done: only elfgate.py with func 0 data 0 gfxnames 0 plus all source counters 0 ends it.
<!-- /CORE -->

## Goal and done
Remove the display list system completely: no list builders, readers, walker state model or N64 list words (plan_endgame.md).
Done only when `3ds/tools/elfgate.py` prints `func 0 data 0 gfxnames 0` and all LISTCEN2 counters (C1..C12) are 0. A stage
report gives the remaining counts. ASSETFREE, CIA and QTM stay parked. Stop only for a user decision or a console action.
No console push without the user. Pre-approved by the user: the Sonnet and Opus coder rows of NPSO and CONV2.

## Workstreams (replace one-file rows, user 2026-11-13)
One coder owns one workstream, with its own goal page (docs/handoff/ws_<id>.md: objective, output form, loop, directory
boundaries, tests that decide pass and fail). The coder commits per group of directories and hands back at the end.
- A value calls (C1, C2): codemod on whole directories (effects, world, hud, models, rest). Host unit tests for the 7 call shapes first.
- B static data (C5, C6): Gfx arrays to v2 tables (CONV2F, ENTRYCAP, then groups); memcheck margin 310 KB or more.
- C N64 branches (C3, C4: N64DEL2 branch exists with 55 edited files), then the remaining state calls, readers and recdl.
The goal pages are written by the designer (oracle). Until they exist, run the open branches (N64DEL2, ENTRYCAP, CONV2F, CODEMODW1)
as they are.

## Roles
- Orchestrator: launches, merges, reports, asks. Does not relay a fault: the coder asks the designer directly.
- Designer (Opus oracle): on call. Reads code, answers in 10 lines or fewer, writes goal pages and the handoff file only.
  The oracle stays ahead of the coders (user approved, 2026-11-13): while a coder runs, the oracle writes the next closed pages, so
  no coder slot idles for want of a page. When a workstream ends, the orchestrator asks the oracle for the next pages at once.
  A resume after 5 minutes of idle rewrites the whole context into the cache (FACT, docs). The old oracle used 717M cache-read
  and 26M cache-write tokens in 1521 calls (FACT, transcript). Rule: start a fresh oracle with docs/handoff/oracle_handoff.md
  (60 lines or fewer) when the context passes about 150K tokens. The user allows this (2026-11-13).
- Coders: Haiku (coder.md). One per workstream, at most 3, plus 1 for tools. A Sonnet or Opus coder needs the user's yes.

## Routine and cost
- Each hand-back: merge (below), then report the counter change and the tokens of the run (usage in the notice). Nothing else.
- A coder run costs 16M to 32M cache-read tokens (158 to 305 model calls, FACT). The levers are fewer calls and shorter output.
- Stale check: an agent with no commit for 45 minutes gets one message asking for its state in one line.
- Stop test (GUESS, set by the orchestrator; the user may change it): after 3 merged groups, C1 must have fallen by 200 or more.
  If not, stop all work and ask the user.
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
