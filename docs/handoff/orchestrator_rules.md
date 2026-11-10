# Orchestrator rules (papermario-3ds-port)

Read at the start and after every compaction; this file overrides any summary. Shared rules: CLAUDE.md. Hook: ORCH_RULES.txt.
Edit rules in place, never append. Start of a session: read docs/handoff/HANDOFF.md.

## Roles
- Orchestrator (Opus): launches agents, merges, cleans up, pushes on request, asks the user. It does not edit game source and does
  not debug. Keep its context small: do not read long reports or files.
- Work types: continuous work (a chain of rows that depend on each other, for example HC4f3 -> HC4e4 -> HOTORD) is finished in one
  session with the same agents; independent work (a playtest bug, a procedure fix) can move to any session. Do not end a session
  inside a chain.
- Session size: count the context from the session transcript at stopping points. Ask the user at about 300K, then once per
  further 100K, whether to start a new session. A new session starts only between chains (no agent running). Write HANDOFF.md
  only when a new session starts. Agents of one session cannot be resumed from another session: their context ends there.
- Oracle: one Opus agent at a time, fresh per batch, replaced past about 300K context. It reads code and writes only
  docs/handoff/row_*.md files (rows, findings), never code. It plans each fix by a code read (CLAUDE.md 1-2). For a change of a data
  format it first lists every reader by grep, in the row.
- Code agents: Haiku (coder.md). Sonnet or Opus only for a row where the oracle gives a reason and the user says yes. Only the user
  approves Opus agents. Reuse an agent in the same area and of the same model type for related work,
  when the follow-up comes soon (its cache lasts about 5 min) or its context holds the code read the next step needs. Never reuse
  across model types (a Haiku row gets a Haiku agent). Rotate: Haiku past about 150K, the oracle past about 300K, when the work
  leaves the agent's files, or at the end of a chain.

## Rows
- One row file per agent (60 lines or fewer), a prompt of 10 lines or fewer that points to it.
- Rows that run at the same time edit disjoint files (azt_scenarios.json and bench_defs.json count). At most 6 code agents.
- Handback triage: a test-tool fault opens a tool row and the gate waits (CLAUDE.md 4). Any other failure goes to the oracle at once.
  Decide yourself only what changes no behaviour (a name, a flag, a scope). Never relaunch an unchanged row.
- A bug row starts from the user facts. When the code read finds no cause, ask the user (AskUserQuestion).

## Worktrees and merge
- `sh 3ds/tools/wt_new.sh <row>` makes build3ds/wt/<row> on branch <row> from main.
- Merge the commit hash the agent reports, never the branch tip. `git merge-tree --write-tree --name-only main <hash>`: exit 1 =
  conflicts. Check for markers and attribution. Put the memcheck line in the merge message.
- After a merge: append the worktree's new runs.jsonl lines and commit runs.jsonl and pushed_builds.txt (CLAUDE.md 10: main stays
  clean), delete .wt_keep, `sh 3ds/tools/wt_gc.sh --apply`, `git branch -d`. Then
  build main in the background and run `python 3ds/tools/bench.py base --fill` in the main folder.

## Status and questions
- Every turn: RUNNING (with type: [Haiku], [Oracle Opus], [Push], [Build]) / READY-NOT-LAUNCHED / BLOCKED-ON-USER.
- Ask only with AskUserQuestion (2-4 options, recommended first). Do not ask what a rule decides.

## Console
- The user judges speed and look on the console. Push only when the user asks, with the multi-select (O3DS XL 192.168.1.131,
  N3DS 192.168.1.224, O3DS 192.168.1.170). Default: `powershell -NoProfile -File 3ds/go.ps1 push o -NoBuild -ip <ip> -Interactive
  -Profile prof warp=mac_01:0 pos=0:0:200`. Use `-Profile log` when the user tests a fault. A failed push gets one line, then stop.
