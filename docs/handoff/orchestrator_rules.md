# Orchestrator rules (papermario-3ds-port)

Read at start and after every compaction (overrides any summary), with CLAUDE.md, HANDOFF.md, plan_endgame.md, test_catalog.md.
Edit in place, never append. The why of each rule is in memory (feedback files) and git log, not here.

## Goal and done
Remove the display list system completely: no list builders, readers, walker state model or N64 list words (plan_endgame.md).
The epic is NOT DONE until the old system is gone from the source or proven absent from the final 3ds executable:
`3ds/tools/elfgate.py` prints `func 0 data 0 gfxnames 0` and all source counters (LISTCEN2 C1..C12) are 0. Nobody says "done" or
"stage complete" before that; a stage report gives the remaining counts. ASSETFREE, CIA and QTM stay parked.
Stop only for a user decision (AskUserQuestion) or a console action. No console push without the user.
Pre-approved by the user: the Sonnet and Opus coder rows of the chain, NPSO and CONV2 (Opus); one budget reset per row correction.

## Roles
- Orchestrator: launches agents, merges, cleans up, pushes on request, asks the user. It edits no game source, does not debug,
  writes no rows, reads no long reports. Never ends a session inside a chain. Writes HANDOFF.md only at a handoff.
- Oracle (Opus): reads code, writes only docs/handoff/row_*.md and plan files (oracle.md). Plans every fix by code read; for a
  data-format change it lists every reader first. One oracle, reused. A replacement oracle or any other Opus agent needs a user
  yes, and only when forced (hard context limit, visibly worse answers) or for a new work area, never on a token count. The usage
  line sums every turn: it is not the context. Never stop the oracle for its size.
- Coders: Haiku (coder.md) by default; Sonnet or Opus when the oracle gives a reason and the user says yes. At most 6 coders.
  Reuse an agent for related work soon after (cache about 5 min); rotate past about 150K or when the work leaves its files.

## Rows
- One row file per agent. Haiku rows 60 lines or fewer; Sonnet and Opus rows longer (design, reasons, readers, risks, gate). The
  launch prompt is 10 lines or fewer and points to the row. Launch description: "[model] TASK - what the row changes, plain words".
- Keep coders fed: below 6 coders with open work, ask the oracle for one refill batch and start every ready row.
- No merge conflicts (user): several agents may edit one file, never the same area. Every row names its PLACES (file plus function
  or region); docs/handoff/places.md lists the open rows' places; before a launch check the new row against it. Equal or overlapping
  places, or a rename/move/reformat of a shared region, make the later row wait and branch from the new main.
- Rows never edit shared registries (gen_gfx_tab.sh GCONV_SRC lines, azt_scenarios.json, bench_defs.json): per-row files (REGDIR
  rows) or tool-written (runs.jsonl, pushed_builds.txt). Until split, rows touching one run one at a time.
- Coders do not merge main to stay current. The orchestrator merges right after the hand-back, one row at a time, merge-tree
  check first. A merge conflict is a planning fault: stop and ask the oracle.
- Azahar: PM_AZ_SLOTS private copies (default 3), one mutex each; a list (`bench.py run a,b,c`) runs on one kept slot; plan a row's
  scenarios as one list. Only a tool fault stops a gate. A base pair must run on one slot.
- Row spec fault: after the oracle corrects the row, reset the budget once in the coder's worktree
  (`cd build3ds/wt/<row>; powershell -File 3ds/run_azahar.ps1 -ResetBudget <row>`) and tell the user. Tool faults never get a
  reset. The oracle sends the corrected row to the coder (relay it if the oracle cannot). Every compare is exact: one differing
  bit fails the gate, no rounding exception.
- Hand-back triage: a test-tool fault opens a tool row and the gate waits (CLAUDE.md 4); any other failure goes to the oracle at
  once. Decide alone only what changes no behaviour. Never relaunch an unchanged row. A bug row starts from the user's facts;
  no cause found: ask the user.

## Merge (sh 3ds/tools/merge_row.sh <row> <msgfile>; main must be clean)
1. Commit docs first: `git add -A -f docs/handoff 3ds/logs/perf/pushed_builds.txt`. A worktree runs.jsonl that is huge:
   `python -I 3ds/tools/dedupe_runs.py <that file>` first. Message file: memcheck line, gate values, hash.
2. Merge the hash the agent reports. The script checks conflicts (merge-tree), merges --no-ff, appends the worktree's new
   runs.jsonl lines, runs wt_gc and deletes the branch. When a branch changes merge_row.sh, run a copy of the script.
3. Rebuild main in the background with plain `sh 3ds/build.sh` (default lane; a lane build writes build3ds/lane-main/, which the
   push does not read). Never run a build or push in the foreground of the chat; report one line at the end.

## Status and questions
- Every turn starts with one line "Display list code remaining" in counts (newest LISTCEN2; before it exists: 3820 active builder
  calls +648 N64-only, 3254 gMainGfxPos uses +715, 22103 list data macros in 2107 Gfx arrays, 714 reader hits, 15 state functions),
  then one table: State | Agent | Row | Summary. State = RUNNING, QUEUED, BLOCKED-ON-USER (console actions only). Agent =
  [Haiku], [Sonnet], [Opus], [Oracle Opus], [Push], [Build]. Summary = the Agent description word for word.
- Ask only with AskUserQuestion (2-4 options, recommended first). Do not ask what a rule decides. A decision is never a status
  line. A design the user gave to the oracle is the oracle's job, not a blocker.
- Style: no flattery, no apologies, answer the question asked, with the number.

## Console
- Runs are scarce (the user opens the netloader each time). One console run per batch; compare with a stored run.
- Push only when the user asks; he names the console (O3DS XL 192.168.1.131, N3DS 192.168.1.224, O3DS 192.168.1.170).
  Build first, push right after the answer: the HBL netloader accepts one connection and stops listening after minutes, so never
  test port 17491 and never build between the question and the push. Default:
  `powershell -NoProfile -File 3ds/go.ps1 push o -ip <ip> -Interactive -Profile prof warp=mac_01:0 pos=0:0:200` in the background
  (the stale check by git hash is an open tool fault, so no -NoBuild until TOOLFIX merged).
- A failed push: read build3ds/out/push_<ip>_*.txt (raw 3dslink output), one line, stop. After a timed run (prof, fxtest) pull the
  log yourself when it has finished: `python 3ds/tools/pm_logrecv.py --ip <addr> --label <name>`, fallback FTP
  (`go.ps1 pull o|n -ip <ip>`); `3ds/tools/decode_dump.py` reads Luma dumps.
