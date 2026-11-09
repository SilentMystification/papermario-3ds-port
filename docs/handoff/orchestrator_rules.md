# Orchestrator rules (papermario-3ds-port)

Only the orchestrator reads this file. Shared rules are in CLAUDE.md. Style hooks: ORCH_RULES.txt, ~/.claude/asd_ste100.txt.

## Roles
- Orchestrator (Opus): plans, writes row files and prompts, merges passed branches, commits docs, runs console pushes, cleans worktrees. It does not edit game source, debug or verify agent claims; it may read code and logs to plan.
- Code agents (Haiku): write code, run tests, investigate. Sonnet only for a large design problem named so in the row file. No churn.
- Opus agents: only the user spawns them (user, 2026-11-09). The orchestrator never launches an Opus agent; it asks the user. Code rows go to Haiku.
- Oracle (ONE shared Opus Plan agent, read only, reused by SendMessage, never several at once): every design decision and every failed handback (triage classes 2 to 4). Its answer goes into the row file under "## Oracle plan" and is committed before the agent restarts.
- Manual mode: when the user says manual work runs elsewhere, launch nothing on those rows or files.

## Before the first agent on a bug (user, 2026-11-07)
1. User facts: the exact trigger sequence (inputs, menus, choices), the first wrong frame and what it should look like. Write them at the top of the row file. A visual bug with no reproducing scenario gets the scenario first; no analysis of frames the user did not name. (FX1 spent two days on frames before the menu confirm; the break is after it.)
2. Reproduce: one det cold run that shows the wrong picture. No reproduction: ask the user (triage class 3), do not plan.
3. Code trace to a named file, line and value. Shots only confirm a named cause or serve as the final gate.
4. Every oracle plan names its premise and one cheap check (a log line or a code read); the agent runs that check first and returns at once if it fails.

## Handback triage (name the class before any relaunch)
1 environment fault: fix the environment, same row and model. 2 wrong premise: rewrite the row with the agent's facts. 3 no reproduction: get user or console evidence first. 4 real stall: split the row; a second stall on the smaller row goes to Sonnet. Never relaunch an unchanged row.

## Agents and worktrees
- At most 6 code agents (2 Azahar slots); read-only agents do not count. One agent per epic; reuse a live agent for its epic; a row of about 30 lines goes to a live agent.
- One agent, one worktree: `git worktree add build3ds/wt/<row> -b <row> main`. Before a new agent uses an existing worktree, stop the old one (TaskStop). Every build.sh and bench call in a worktree sets PM_LANE=<row>; without it the shared main volume is used and the main build breaks.
- No git stash anywhere (the stash list is shared by all worktrees).
- Done means the whole row set passes. A handback with work left is not allowed; the prompt says to wait for background runs in the foreground. An early handback is resumed once with the same row.
- An agent silent for over 2 hours: collect its state into the row file, stop it.

## Merge procedure
1. Check every commit in the range (markers, attribution trailers, WIP commits: squash those).
2. `git merge-tree --write-tree --name-only main <branch>`: exit code 1 means conflicts (paths on line 2 and later); never use line 1 then. The branch owner resolves source conflicts; the orchestrator resolves only doc and JSON conflicts (JSON by name: main plus the branch's new entries, then a duplicate-name check).
3. Memory line: `python 3ds/tools/memcheck.py --quiet --map build3ds/wt/<row>/build3ds/lane-<row>/pm_3ds.map` (the lane build of the branch tip; rebuild the lane if it is older) and put the `memcheck:` line in the merge commit message. OVER does not block the merge: the message says why the merge takes it (hw_guidelines.md, memory budget). Then commit-tree plus update-ref; sync the changed files into the shared folder; reset runs.jsonl in the index.
4. Tell running agents whose files changed to merge main.
5. Clean up: `sh 3ds/tools/wt_gc.sh --apply --only <merged worktree>` (it also deletes that lane's `pm_3ds_<lane>.3dsx` copies in the Azahar sdmc folder and the azslots, and its `build3ds/azt/shots/*/<lane>` folders). A partial merge of a running agent's branch makes the branch look merged: every agent worktree gets a `.wt_keep` file at creation, and wt_gc keeps it (core20 and ls3 were deleted mid-row on 2026-11-07/08). Delete `.wt_keep` when the row is complete.
6. Keep a push-ready build (user, 2026-11-08): after each merge that changes a build input, run the main build in the background (`sh 3ds/build.sh`, no PM_LANE, after gen_map_restore.py; restore map_restore_table.c). build3ds/pm_3ds.3dsx then matches main and a console push needs no build. Never run this build while a push runs: both use the docker container pm3ds-build-main (a push failed with exit 125 on 2026-11-08).

## Gates
- Byte gates run cold with det. A scene whose picture moves with code layout is not a byte gate until that cause is fixed (mac_00 since FB-2, row_FB.md MAC00).
- Never rewrite the shared azt_scenarios.json or bench_defs.json while an agent runs in the shared folder.

## Status
Every turn: RUNNING / READY-NOT-LAUNCHED (with the exact wait) / BLOCKED-ON-USER. Relay each report in 3 lines or fewer. Ask only with AskUserQuestion (2 to 4 options, recommended first). Do not ask what a rule already decides.

## Prompts
- One row file per agent (row_TEMPLATE.md, 60 lines or fewer); a prompt of 10 lines or fewer that points to it. Name commands, expected results and files literally.

## Console
- The user judges speed on the console; Azahar shows direction only. Push only when the user asks: `PM_ROW=PUSH sh 3ds/push_3ds.sh --no-build --ip 192.168.1.131 -t 90` (O3DS XL). A netloader refusal prints the title-mode hint. A failed push gets one line, then stop.
