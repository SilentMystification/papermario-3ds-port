# Orchestrator rules (papermario-3ds-port)

Read at the start and after every compaction. This file overrides any summary. Shared rules: CLAUDE.md. Hook: ORCH_RULES.txt.
Edit rules in place, never append. At the start of a session, read docs/handoff/HANDOFF.md and docs/handoff/test_catalog.md (the test harness).

## Standing goal (user, 2026-11-12)
Do not stop until the native pipeline is implemented and the old system (walker state model, N64 list logic, the old stream path)
is completely removed: plan_nativeapi.md stages S1 to S6. Each turn: keep 6 coders fed, merge, build main, ask the oracle for the
next batch. Stop only for a user decision (AskUserQuestion) or a console action. ASSETFREE, CIA and QTM stay parked.
Pre-approved by the user (2026-11-12, for the night run): all Sonnet and Opus coder rows of the chain (Sonnet S5, S6, S7, S8, S9
rows; Opus S6T, S5IMGFX, S7CONV, S7MDL); a fresh Opus oracle only when forced (the hard context limit is reached or its answers
visibly degrade) or when the work moves to a new area (user, 2026-11-12: no rotation on a token count, it saves no proven cost
and a fresh oracle re-reads about 300K tokens); one budget reset per row correction. No console push without the user: keep
merging passed rows, rebuild main, wait. Hardware-only risk rows (GPU state, texture binds) still merge when their Azahar gates pass.
Definition of done (user, 2026-11-12): the complete native-only path with ALL display list code removed (the game draws only
through native calls; no N64 list words, no walker state model, no list readers, no stream replay of list words anywhere).
A stage that leaves list code behind is not done. The last rows delete it and prove it by grep (no gSP/gDP list builders left in
the draw path, no ndc/ops readers of list words) plus the byte set and the console run.
The epic is NOT DONE (user, 2026-11-12, repeated) until the old system is not used at all: removed from the source, or proven
absent from the final 3ds executable (ELF symbol and section check). Nobody, orchestrator included, says "done" or "complete"
for the epic before that proof. A stage report says what is left, in counts, never "stage complete".

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
  oracle, an Opus code agent) needs a user yes: ask with AskUserQuestion and give the reason (the oracle hit the hard context limit or its
  answers visibly degrade, or the work moved to a new area where its context does not help). Never a token count alone (user, 2026-11-12). Ask the oracle for its own context count; the subagent usage line adds up every turn
  and is not the context. Never two oracles at once unless the user says so.
  Never stop or pause the oracle (user 2026-11-11): its context size is no reason to stop its work. Do not tell it to stop at a token count.
- Code agents: Haiku (coder.md). Sonnet or Opus only when the oracle gives a reason and the user says yes. Reuse an agent of the
  same model type for related work in its area when the follow-up comes soon (its cache lasts about 5 min) or its context holds the
  code read the next step needs. Rotate the agent past about 150K, when the work leaves its files, or at the end of a chain.

## Rows
- One row file per agent. Haiku rows: 60 lines or fewer. Sonnet and Opus rows may be longer (user, 2026-11-12): the oracle gives
  the design, the reasons, the files and the checks in full, and the model may choose its own steps inside the gate. The prompt is
  10 lines or fewer and points to the row.
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
- Row spec fault (user, 2026-11-12): after the oracle corrects the row, reset the budget once inside the coder's worktree
  (`cd build3ds/wt/<row>; powershell -File 3ds/run_azahar.ps1 -ResetBudget <row>`; the budget file is per worktree) and tell the
  user in the status. Tool faults never get a reset. The oracle sends the corrected row to the coder itself. A byte diff of
  at most 1 step per channel inside the effect area passes when the oracle names the code cause (rounding rule).
- Handback triage: a test-tool fault opens a tool row, and the gate waits (CLAUDE.md 4). Send any other failure to the oracle at
  once. Decide alone only what changes no behaviour (a name, a flag, a scope). Never relaunch an unchanged row.
- A bug row starts from the user facts. When the code read finds no cause, ask the user (AskUserQuestion).

## Worktrees and merge
- `sh 3ds/tools/wt_new.sh <row>` makes build3ds/wt/<row> on branch <row> from main.
- Merge the commit hash the agent reports, never the branch tip.
  1. `git merge-tree --write-tree --name-only main <hash>`: exit 1 means conflicts.
  2. Check the diff for conflict markers. Attribution lines are allowed (user, 2026-11-12).
  3. Merge with `--no-ff`. Put the memcheck line in the merge message.
  4. Append the new runs.jsonl lines of the worktree to main's runs.jsonl. Commit runs.jsonl and pushed_builds.txt with
     `git add -f` (CLAUDE.md 10: main stays clean).
  5. Delete .wt_keep, run `sh 3ds/tools/wt_gc.sh --apply`, run `git branch -d <row>`.
  6. Build main at once (foreground, then `python 3ds/tools/bench.py base --fill` in the main folder), so a current build is always ready.
     A push then starts at the user's answer with `-NoBuild` (the build is never started after the question). Before the question,
     check that the build matches HEAD (code); rebuild first if a merge came after it.

## Status and questions
- Every Agent launch: the description is "[model] TASK - summary", 10 words or fewer for the summary (for example "[Haiku] KEYMISS - key-miss census for the draw cache").
- Every turn, one markdown table (user, 2026-11-12) with the columns State | Agent | Row | Summary. State is RUNNING, QUEUED
  or BLOCKED-ON-USER. One line per agent or row. Agent = [Haiku], [Sonnet], [Opus], [Oracle Opus], [Push], [Build]. Summary = 10
  words or fewer (the same text as the Agent description). BLOCKED-ON-USER: console actions only.
- Ask only with AskUserQuestion (2-4 options, recommended first). Do not ask what a rule decides. A decision or blocker is never
  written as a status line: ask it as a question at once. BLOCKED-ON-USER lists only console actions that wait for the user.
  A design the user gave to the oracle is the oracle's job: it is not a blocker.

## Console
- Console runs are scarce (the user must open the netloader for each push). No row gate asks for more than one console run. Rows that
  need a console check share one run per batch, and the result is compared with a stored console run (never rerun the base, never repeat a
  run). Function call counts per class from the fncov census (FXFNCOV benches) are deterministic: they are the Azahar gate of a speed row.
  Azahar run time is never a speed gate.
- The user judges speed and look on the console. Push only when the user asks, with the multi-select (O3DS XL 192.168.1.131,
  N3DS 192.168.1.224, O3DS 192.168.1.170).
- After a push of a timed run (fxtest, prof): check the result yourself when the run time has passed (about 5 minutes for fxtest all):
  pull the log with `python 3ds/tools/pm_logrecv.py --ip <addr> --label <name>` (FTP as the fallback) and report. Do not wait for the user's signal.
- Default: `powershell -NoProfile -File 3ds/go.ps1 push o -NoBuild -ip <ip> -Interactive -Profile prof warp=mac_01:0 pos=0:0:200`.
  Use `-Profile log` when the user tests a fault. A failed push gets one line, then stop.
