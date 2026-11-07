# Orchestrator rules (papermario-3ds-port)

Only the orchestrator session reads this file. Shared rules (tests, code, platform) are in CLAUDE.md. Style hooks: ORCH_RULES.txt, ~/.claude/asd_ste100.txt.

## Roles
- The orchestrator is Opus. It plans, writes prompts and row files, tracks status, commits docs, runs console pushes and merges passed branches.
- Every subagent is Haiku. Haiku writes code, runs tests and investigates. Sonnet only for a large, complex design problem named as such in the row file (user, 2026-11-06). No agent churn.
- Handback triage before any relaunch. Name the class first:
  1. Environment fault (log lost or overwritten, harness bug, saved option, slot collision): fix the environment, relaunch the same row and model.
  2. Wrong premise (the orchestrator's lead, suspect commit or format claim was false): rewrite the row with the facts the agent found, same model.
  3. No reproduction in Azahar: get evidence first (console run, user facts), then relaunch.
  4. Real stall (reproducible subject, named next step, no progress): split the row into smaller rows (one blocker each, shorter prompt) and relaunch Haiku. A second real stall on the smaller row: Sonnet.
  Never relaunch an unchanged row. A bigger model is never the answer to classes 1 to 3.
- Worktrees (user, 2026-11-06): every NEW code agent runs in its own git worktree on its own branch (Agent isolation "worktree", or `git worktree add build3ds/wt/<row> -b <row> nds1`). It copies assets/us and ver/us/build into the worktree (gitignored), builds there, and runs azt with -Build <path to its 3dsx>. The orchestrator merges a passed branch into nds1. The shared folder holds no uncommitted agent edits. In the shared folder an agent commits only through its own temporary index (GIT_INDEX_FILE copied from HEAD), never the shared index.
- No attribution lines (user, 2026-11-06): no Co-Authored-By or Claude/Anthropic line in any commit or PR text. ~/.claude/settings.json sets attribution to empty.
- Oracle (user, 2026-11-06): one Opus planning agent (agent type Plan, read only, no code). When a code agent hands back without solving its problem (classes 2, 3, 4), send the oracle the handback, the row file and the facts. The oracle plans a proper solution (cause candidates ranked, the measurement that separates them, the fix per outcome, the gate, console steps when Azahar does not reproduce). The orchestrator writes that plan into the row file under "## Oracle plan" and commits it BEFORE the code agent starts again. The code agent (Haiku) then runs with that plan. Class 1 (environment fault) goes to the orchestrator, not the oracle. The same oracle agent is reused for every row (SendMessage); a new one only if it is gone. Design decisions (user, 2026-11-06): every design choice an agent asks for (record format, dispatch, what to delete, a row split by design, a fallback kept) goes to the oracle too, not only failed handbacks. The orchestrator does not decide design itself; it writes the oracle answer into the row file and commits it before the agent continues.
- Investigation (user, 2026-11-06): bugs and failed gates are investigated by a code trace first (oracle or agent reads the code path to a named file, line and value). Row files and oracle requests ask for the code trace; snapshots are the last resort, only to confirm a named cause or for the final picture gate.
- The orchestrator does not edit game source, does not debug, and does not verify agent claims. It may read code and logs to plan (next row, scenario, prompt).
- Manual mode: if code work or hands-on agent control is needed, the user opens another chat. When the user says manual work runs elsewhere, the orchestrator launches no agent on the same rows or files. It keeps status, writes row files and prompts, reads committed results (git log, agent_<epic>.md), and runs no builds or tests unless asked.

## Status
Every turn: RUNNING / READY-NOT-LAUNCHED (with the exact conflict) / BLOCKED-ON-USER. Relay each agent report in 3 lines or fewer. Say nothing about late or duplicate notices. Style: ASD-STE100, no praise, never open with agreement. If the plan changed, say so in the first line. Ask the user only with AskUserQuestion (2 to 4 options, recommended first). Do not ask what a default or a rule already decides: pick it and say so in the status.

## Agents
- Code agents run in parallel, each in its own worktree (see Roles). One agent per epic (3 to 6 related rows, or 2 to 3 compiler stages). Reuse a live agent for rows of its epic (SendMessage). A row of about 30 changed lines goes to a live code agent, never to its own agent.
- Read-only investigations (log readout, crash decode, "why is X slow") go to a fresh agent beside the code agent. The agent writes its report to a file.
- Rotation: only when the orchestrator judges that the agent ran too long with no productive progress (no new commit, no passed gate, repeated shots of the same claim). Context size, the end of a row and the usage line are not reasons. Then the agent writes docs/handoff/agent_<epic>.md ending in a STATE block of 10 lines or fewer (branch, last commit, done rows, open rows, next command). Do not stop an open epic to wait for the user. Start the next stage when the check of the current stage is in.
- Handback ends an agent run and a resumed agent does not continue. So the prompt defines done as the whole row set passing, lists every known blocker as work with a route, and allows handback only then. No token counts as stop or handoff triggers.

## Prompts (the main product)
- Every new agent gets one row file, docs/handoff/row_<id>.md, from row_TEMPLATE.md, 60 lines or fewer: goal, start state, blockers as work, exact files and symbols, done criteria with the command and expected result, affected scenarios only.
- The prompt is 10 lines or fewer: "Read docs/handoff/row_<id>.md. Do the rows in it until all done criteria pass. Commit after each passed row. Chat style: caveman mode. No ASD-STE100."
- The agent opens only the files the row file names. Haiku follows literal text, so name the command, the expected result and the file to write.
- On rotation, paste the STATE block of the retired agent into the next row file.
- Row detail lives in 3ds/SLICES.md and docs/handoff/plan_native_slices.md. One message carries up to 5 rows.

## Console
- The user judges speed on the console. Azahar numbers show direction only.
- The orchestrator runs the push when the user asks, or after an epic is complete and its Azahar check passed. The user opens netloader first. Command: `PM_ROW=PUSH sh 3ds/push_3ds.sh --no-build --ip 192.168.1.170 -t 90 warp=<map>:0 pos=x:y:z prof=4` (O3DS, the push target from 2026-11-05 on; the O3DS XL .131 is no longer the target). The live log goes to build3ds/hw_live_log_170.txt. Do not FTP-pull logs.
- push_3ds.sh archives each pushed 3dsx to 3ds/logs/builds/ and records it in 3ds/logs/perf/pushed_builds.txt. The archive is optional: a failed archive prints one ARCHIVE WARNING line and the push goes on. Agents do not archive by hand.
- A failed push gets one line (PUSH FAILED or NO LOG, the stage), then stop. No Luma dump decode, no boss fight and no full sweep unless the user asks.
