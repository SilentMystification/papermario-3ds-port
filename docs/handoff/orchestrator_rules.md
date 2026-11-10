# Orchestrator rules (papermario-3ds-port)

Read at the start and after every compaction; this file overrides any summary. Shared rules: CLAUDE.md. Hook: ORCH_RULES.txt.
Edit rules in place, never append. Start of a session: read docs/handoff/HANDOFF.md.

## Roles
- Orchestrator (Opus): launches agents, merges, cleans up, pushes on request, asks the user. It does not edit game source and does
  not debug. Keep its context small: do not read long reports or files; /compact at about 450K, a fresh session per work block.
- Oracle: one Opus agent at a time, fresh per batch, replaced past about 300K context. It reads code and writes only
  docs/handoff/row_*.md files (rows, findings), never code. It plans each fix by a code read (CLAUDE.md 1-2). For a change of a data
  format it first lists every reader by grep, in the row. It marks a fault that Azahar cannot show as console-only.
- Code agents: Haiku (coder.md). Sonnet or Opus only for a row where the oracle gives a reason and the user says yes. Only the user
  approves Opus agents. Reuse a live agent by SendMessage for its follow-ups.

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
- After a merge: append the worktree's new runs.jsonl lines, delete .wt_keep, `sh 3ds/tools/wt_gc.sh --apply`, `git branch -d`. Then
  build main in the background and run `python 3ds/tools/bench.py base --fill` in the main folder.

## Status and questions
- Every turn: RUNNING (with type: [Haiku], [Oracle Opus], [Push], [Build]) / READY-NOT-LAUNCHED / BLOCKED-ON-USER.
- Ask only with AskUserQuestion (2-4 options, recommended first). Do not ask what a rule decides.

## Console
- The user judges speed and look on the console. Push only when the user asks, with the multi-select (O3DS XL 192.168.1.131,
  N3DS 192.168.1.224, O3DS 192.168.1.170). Default: `powershell -NoProfile -File 3ds/go.ps1 push o -NoBuild -ip <ip> -Interactive
  -Profile prof warp=mac_01:0 pos=0:0:200`. Use `-Profile log` when the user tests a fault. A failed push gets one line, then stop.
