# Orchestrator rules (papermario-3ds-port)

Read at the start and after every compaction. This file overrides any summary. Shared rules: CLAUDE.md. Hook: ORCH_RULES.txt.
Edit rules in place, never append. At the start of a session, read docs/handoff/HANDOFF.md and docs/handoff/test_catalog.md (the test harness).

## Roles
- Orchestrator: launches agents, merges, cleans up, pushes on request, asks the user. It does not edit game source and does not
  debug. Keep its context small: do not read long reports or long files.
- Continuous work (a chain of rows that depend on each other, for example HC4f3 -> HC4e4 -> HOTORD): finish it in one session with
  the same agents. Independent work (a playtest bug, a procedure fix) can move to any session. Never end a session inside a chain.
- Session size: count the context from the session transcript at stopping points. From about 300K, ask the user (once per
  further 100K) only at a stopping point where a new session would be right: between chains, no agent running. Never ask when the
  answer would be "not yet". Write HANDOFF.md
  only when a new session starts. Agents cannot resume in another session.
- Oracle (Opus): reads code and writes only docs/handoff/row_*.md files (rows, findings). It never writes code (oracle.md).
  It plans every fix by code read (CLAUDE.md 1-2). For a data-format change it first lists every reader. The orchestrator starts
  the first oracle of a session and reuses it for related questions, sent in groups. Any other Opus agent (a second or replacement
  oracle, an Opus code agent) needs a user yes: ask with AskUserQuestion and give the reason (the oracle context passed about 600K, or a
  new area where its context does not help). Ask the oracle for its own context count; the subagent usage line adds up every turn
  and is not the context. Never two oracles at once unless the user says so.
- Code agents: Haiku (coder.md). Sonnet or Opus only when the oracle gives a reason and the user says yes. Reuse an agent of the
  same model type for related work in its area when the follow-up comes soon (its cache lasts about 5 min) or its context holds the
  code read the next step needs. Rotate the agent past about 150K, when the work leaves its files, or at the end of a chain.

## Rows
- One row file per agent (60 lines or fewer). The prompt is 10 lines or fewer and points to the row.
- Keep the coders fed. The oracle writes rows in batches ahead of the coders (oracle.md 7). Each turn the orchestrator checks the
  number of running coders: below 6 with open work, it sends the oracle one "refill" request (a batch for all open work) and starts
  every ready row at once. It does not hand the oracle one job at a time.
- Avoid merge conflicts. Rows may run in parallel only when no merge conflict can result, and no row depends on the unmerged
  output of another. Rows may share a file when their edits are in different places (different functions, scenarios or blocks)
  and no edit moves or reformats the other's lines.
- Before a launch, compare the files and places that the running rows and the new row edit. A possible overlap of the same lines
  (or a rename, move or reformat of a shared file) makes the later row wait until the earlier row is merged. It then branches from
  the new main.
- Azahar scales by slots (test_catalog.md): PM_AZ_SLOTS private Azahar copies (default 3), one mutex each, so runs on different
  slots go in parallel. A warm restart takes about 1 s. A list (`bench.py run a,b,c`) runs scenarios one after the other on one
  kept slot. Rows with Azahar gates run in parallel, each on its own slot. Plan each row's scenarios as one list. A row waits for
  Azahar only when 3 slots are in use. Only a fault of the tool (two runs of one build differ, a silent start) stops a gate.
- A conflict at merge is a planning fault. Stop and ask the oracle. Do not resolve it by guessing.
- At most 6 code agents.
- Handback triage: a test-tool fault opens a tool row, and the gate waits (CLAUDE.md 4). Send any other failure to the oracle at
  once. Decide alone only what changes no behaviour (a name, a flag, a scope). Never relaunch an unchanged row.
- A bug row starts from the user facts. When the code read finds no cause, ask the user (AskUserQuestion).

## Worktrees and merge
- `sh 3ds/tools/wt_new.sh <row>` makes build3ds/wt/<row> on branch <row> from main.
- Merge the commit hash the agent reports, never the branch tip.
  1. `git merge-tree --write-tree --name-only main <hash>`: exit 1 means conflicts.
  2. Check the diff for conflict markers and attribution.
  3. Merge with `--no-ff`. Put the memcheck line in the merge message.
  4. Append the new runs.jsonl lines of the worktree to main's runs.jsonl. Commit runs.jsonl and pushed_builds.txt with
     `git add -f` (CLAUDE.md 10: main stays clean).
  5. Delete .wt_keep, run `sh 3ds/tools/wt_gc.sh --apply`, run `git branch -d <row>`.
  6. Build main in the background. Run `python 3ds/tools/bench.py base --fill` in the main folder.

## Status and questions
- Every turn: RUNNING (one line per agent, labelled with its model and row: [Haiku] HC8, [Sonnet] KZN04, [Opus] HC5, [Oracle Opus], [Push], [Build]) / READY-NOT-LAUNCHED / BLOCKED-ON-USER.
- Ask only with AskUserQuestion (2-4 options, recommended first). Do not ask what a rule decides.

## Console
- The user judges speed and look on the console. Push only when the user asks, with the multi-select (O3DS XL 192.168.1.131,
  N3DS 192.168.1.224, O3DS 192.168.1.170).
- Default: `powershell -NoProfile -File 3ds/go.ps1 push o -NoBuild -ip <ip> -Interactive -Profile prof warp=mac_01:0 pos=0:0:200`.
  Use `-Profile log` when the user tests a fault. A failed push gets one line, then stop.
