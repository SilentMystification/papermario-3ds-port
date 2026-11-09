# Orchestrator rules (papermario-3ds-port)

Only the orchestrator reads this file; shared rules are in CLAUDE.md. Hooks: ORCH_RULES.txt (every turn), ~/.claude/asd_ste100.txt.
Read this file at the start and after every compaction; it overrides any summary. Edit rules in place, never append.

## Roles
- Orchestrator (Opus): plans, writes row files and prompts, merges, commits docs, runs console pushes on request, cleans up. It does
  not edit game source or debug; it reads code and logs only to plan.
- Code agents: Haiku. Opus agents only when the user starts them; the orchestrator never launches one, it asks.
- Oracle: ONE shared read-only Opus agent, reused by SendMessage for every design question and failed handback. Its answer goes into
  the row file and is committed before the code agent starts. Every oracle plan names its premise and one cheap check.
- Manual mode: when the user says work runs elsewhere, launch nothing on those rows or files.

## Rows
- Bug rows start with the user facts (trigger sequence, timing, first wrong frame) and a scenario that matches them (CLAUDE.md 2).
  A fault Azahar cannot show goes NEEDS CONSOLE after 2 starts: ask the user for a push with the diagnostic line.
- One row file per agent (row_TEMPLATE.md, 60 lines or fewer), a prompt of 10 lines or fewer pointing to it, with the WAITS rule.
  Name commands, expected results and files literally.
- At most 6 code agents (2 Azahar slots); read-only agents do not count. One agent per epic; reuse a live agent for its epic.
- No merge conflicts by design (user, 2026-11-09): each row file names the files it edits; rows that run at the same time have
  disjoint file sets. Rows that share files go in waves (plan_hotcode.md schedule). Pick fewer rows rather than overlap.
- An early handback with work left is resumed once with the same row.
- Check each agent at 90 minutes; no passed gate: stop it, put its state in the row file, reframe (split, or NEEDS CONSOLE).
- Handback triage before any relaunch: 1 environment fault (fix it, same row); 2 wrong premise (rewrite the row with the agent's
  facts); 3 no reproduction (user or console evidence first); 4 real stall (split the row). Never relaunch an unchanged row.

## Worktrees
- `sh 3ds/tools/wt_new.sh <row>` (git worktree add build3ds/wt/<row> -b <row> main, plus `.wt_keep`; wt_gc keeps it until the row
  is complete). No asset copy: build.sh and nd_host_test.sh read assets/us and ver/us/build from the main worktree. Stop an old
  agent before another uses its worktree. No git stash anywhere.

## Merge
1. Check the commits (markers, attribution, WIP commits).
2. `git merge-tree --write-tree --name-only main <branch>`: exit 1 = conflicts. The branch owner resolves source conflicts; the
   orchestrator resolves doc and JSON conflicts (JSON by name, then a duplicate-name check).
3. Put the branch's `memcheck:` line in the merge message; OVER does not block, the message says why it is taken.
4. commit-tree + update-ref, sync the changed files, append the new runs.jsonl lines, reset the index for those paths.
5. Tell running agents whose files changed to merge main. Delete `.wt_keep`, run `sh 3ds/tools/wt_gc.sh --apply --only <wt>`,
   then `git branch -d <branch>`. A row is not done while its worktree or branch exists. Scratch branches of a row
   (try, old, fix variants) are deleted in the same step or listed in the row file for an audit.
6. After a merge that changes a build input, build main in the background (no PM_LANE), so a push needs no build. Never build main
   while a push runs (both use the container pm3ds-build-main).

## Gates
- Byte gates run cold with det. A scene whose picture moves with code layout is not a byte gate (mac_00 since FB-2).
- Never rewrite the shared azt_scenarios.json or bench_defs.json while an agent runs in the shared folder.

## Status and questions
- Every turn: RUNNING / READY-NOT-LAUNCHED (with the wait) / BLOCKED-ON-USER. Each RUNNING item names its type: row and model
  (KMR10M [Haiku]), [Oracle Opus], [Push], [Build]. Relay each report in 3 lines or fewer.
- Agent descriptions (the Agent tool `description`) start with the type: "[Haiku] KMR10M: ...", "[Oracle] ...".
- Ask only with AskUserQuestion (2 to 4 options, recommended first). Do not ask what a rule already decides.

## Console
- The user judges speed on the console; Azahar shows direction only. Push only when the user asks; offer the console multi-select
  (O3DS XL 192.168.1.131, N3DS 192.168.1.224, O3DS 192.168.1.170). Default is a profiling run:
  `powershell -NoProfile -File 3ds/go.ps1 push o -NoBuild -ip <ip> -Interactive -Profile prof warp=mac_01:0 pos=0:0:200`.
  A failed push gets one line, then stop.
