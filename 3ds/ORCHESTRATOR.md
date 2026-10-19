# Orchestrator handoff

Written 2026-10-17 for a NEW orchestrator chat. Read this file, then HANDOFF.md, 3ds/SLICES.md and 3ds/QUESTIONS.md. The old version of this file is in docs/HISTORY.md.

## 1. Role and workflow
- The orchestrator only orchestrates. It never writes code. It never reads code or project source.
- Subagents do all work: code, scans, reads, queue edits, notes.
- RUN WORK IN PARALLEL (user decision 2026-10-18). Before each launch, look ahead in the queue and start every row that has no conflict. Do not limit the number of agents when work is independent.
- Build lanes: the main tree and lane B (docs/LANES.md, PM_LANE=laneB). One build agent per lane. Add a lane when two independent build rows wait. Azahar runs queue by the mutex in run_azahar.ps1 (one run at a time, a wait is normal).
- Graphics rows (pm_gbi.c, pm_gpu.c, pm_tex.c) run one at a time because they edit the same functions. Non-graphics rows run in the other lane.
- Read-only agents (plans, audits, reviews) run in parallel with build agents. They write one file under docs/plans/ and commit only that file.
- A closing step (merge, doc rows) belongs to the last slice of a lane. It does not get its own agent. Batch other doc edits: do small ones yourself, use one fresh docs agent per milestone.
- Each agent reports in 5 lines or fewer. The orchestrator reads only the report and the last 20 lines of 3ds/PROGRESS.log.
- Relay each report to the user in plain terms: what was done, the numbers, what was not verified, the next step.
- Post a status to the user after every report. Never leave the user without feedback.
- Launch the next slice automatically. Do not wait for the user. Work in large chunks until the queue is empty or a stop condition (section 7) holds.
- A question only the user can answer: add it to 3ds/QUESTIONS.md and continue with other work.
- Start no subagent the user did not expect. When the context grows long, stop at a finished slice, ask an agent for a short HANDOFF.md note, and ask the user to start a new chat.
- H05 FINDINGS (user amendment 2026-10-18): add each finding that needs no user decision as a queue row at once and work it at once. Do not wait for the user review. Only findings that need a user decision (button map, scope) wait.
- FULL RUNS (user decision 2026-10-18): run the full tour or the full golden set only when a change cannot be tested another way, and once before S28. Never run them per slice or per milestone by habit. The orchestrator does not schedule them unless a change needs them.
- HANDOFF POINT (user decision 2026-10-18): when the chat grows long, tell the user in one line that a handoff is due and refresh docs/ORCH_STATE.md. Do NOT stop work. Keep launching rows until the queue is empty or a stop condition (section 7) holds. The user may be asleep.
- Agents read AGENT_RULES.md, not this file. Batch doc edits into one docs agent per milestone. Reuse a running agent by SendMessage for a small follow-up.
- 10. SEND, DO NOT SPAWN: before every Agent call, check for a running or resumable agent in that tree. If one exists, use SendMessage. A row of about 30 changed lines or less never gets its own agent: send it to a live agent or bundle it with the next launch. The handoff text is written by the finishing code agent, never by a new agent.
- 11. NO AGREEMENT: never open a reply with agreement, praise or a concession ('you are right', 'agreed'). State the action taken and the result. If the plan changed, say so in the first line.

## 2. User decisions (2026-10)
- Never push to git. No Claude or Anthropic attribution in commits or PRs.
- NO STUBS. Never write a stub or stand-in for decomp code or data without asking. A port-layer shim is allowed only for real N64 OS or hardware functions with no game logic.
- Fix root causes. Never hide a fault with a guard.
- Q1 (a): the whole real battle tree is merged (done, M03). Q2 (a): needed assets stay resident in RAM, no SD streaming during play.
- Q4: demo maps linked (done). Q5: the obfuscation shims stay empty. The crash screen works only if the debug output can be used (row S31).
- Q6: link all maps. Measure first. All 421 maps are linked (S30b).
- The game runs at exactly 30 fps (PC01). A 30/60 toggle comes later (F06). Widescreen comes later (F07).
- The O3DS is the hardware target. It should reach full speed. Performance work comes after everything works.
- ARM11 math audit: experiments allowed. Game-logic math changes need a replay check and the user approval.
- Use libctru natives for threading and sync. The log is non-blocking on core 1 (L01).
- Workload split: core 0 = logic plus the CPU part of the RSP work. Core 1 = audio. PICA GPU = rasterization plus vertex work. Consider ndsp for audio mixing.
- Testing: feature tests use the warp with -Turbo -Smoke and a fixed length. Never an open wait. Tours iterate on the failing set. Run the full tour once at the end.
- SMALL HANDOFF.md: at most 55 lines. Each slice adds at most 3 lines to it. Detail goes to docs/HISTORY.md.
- History is kept in docs/ (HISTORY.md, SLICES_DONE.md, PROGRESS_ARCHIVE.log). Nothing is deleted. Move text, never drop it.
- Screenshot analysis is GENTLE. Use screenshots only when really needed. Prefer text evidence: draw logs, depth view, checksums, pixel diffs, summary scripts. When a picture is needed, use ONE contact sheet.
- Lighter checks: smoke on presets yard and toad with -Turbo -Smoke, shots of only the touched scenes. No 4-scene A/B and no full tour, unless the slice touches the map change path, boot, title, file select or intro. Iterate on failing sets. Run the full tour once at the end. Warp and turbo for feature tests.
- NARROW TESTS: test only what the change touches. Use one target scene or map, a fixed short length and one text check (log line, draw count or one pixel diff). Do not build large test setups. Do not skip a fault the narrow test finds: fix it. Say in the result which areas were not tested. Build or run checks per slice must be short.
- Memory rule (user decision 2026-10-18): no per-slice cap. The old 518 KB spare estimate was wrong because it assumed the N64 ROM and asset copies stay resident. Revisit memory limits and the strategy ONLY if the port actually runs out of memory (S28 measures it). Agents report the 3dsx size change.
- MODEL RULE (replaces the old "default model, no tiers"): never use `fable`. Use `opus` ONLY for planning: an agent whose job is to plan or design (the Plan agent type, a design or research-to-plan slice, a plan for a large feature such as widescreen, 60 fps, the GPU vertex transform or the audio redesign). Never use `opus` for coding, investigation, builds or checks. For all coding, investigation, shared-code changes, graphics, memory and audio work the highest tier is `sonnet`. After the handoff use `haiku` for MECHANICAL slices: measurement and listing slices, tours and check runs that report a table, documentation and queue edits, git cleanup, and link-and-build slices that follow an existing pattern with a clear check. Use `sonnet` for investigation, any change to shared decomp code, the GPU and graphics layer, memory layout, audio, and anything that needs judgment about the no-stub rule. Pass the `model` parameter of the Agent tool. A planning slice may use `opus`. Its output is a plan file or a design section in docs/ that a later sonnet or haiku agent executes. If a haiku agent reports BLOCKED or gives an unclear result, relaunch the slice on `sonnet` (not opus).
- OPT (user decision 2026-10-18): optimization code work starts only after S28. Planning is allowed before S28. A correctness fault in audio code (nok_01, row F01) is a bug fix, not optimization.
- S28 gate (user decision 2026-10-18): S28 waits until the Azahar queue is empty. Hardware validation checks only that the existing code works on the console. Port logic that Azahar can verify is not debugged on hardware.

## 3. Agent reading rules
Every agent prompt says "follow the reading rules in AGENT_RULES.md".
- Read the tail (last 40 lines) of logs. Never read a whole log.
- Read files with offset and limit.
- Use the summary scripts, not raw logs: `3ds/tools/phase_report.py`, `rom_trace_classify.py`, `rom_trace_check.py`, `check_data_layout.py`, and the tour table output.
- Read docs/INDEX.md first to find things. Do not search the tree.
- Read HANDOFF.md and the one slice row. Do not read docs/HISTORY.md. Use grep on it only when a topic needs it.
- No screenshots unless needed (section 2). One contact sheet at most.

## 4. How to run
- Debug switches go in `debug3ds.txt` or `-Debug "..."`. The list is in HANDOFF.md and docs/INDEX.md. Never leave `fast` in debug3ds.txt.
- Main command: `powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Debug "fast preset=toad"`. Exit 0 = ok, 1 = failed, 2 = unmapped access or crash.
- Normal chain: `-Debug "fast autoa freshsave" -Stall 40`. Use run_azahar.ps1, not go.ps1. Never start azahar.exe with other arguments than the .3dsx path.
- Hardware skills: `pull-n`, `pull-o`, `push-a`, `push-n` (N3DS 192.168.1.223), `push-o` (O3DS 192.168.1.168).
- Test budget: each row has 4 Azahar starts (PM_ROW, run_azahar.ps1). The orchestrator never gives a row more than one budget reset (`-ResetBudget <row>`) without a reason.
- Progress log: each agent appends one line per step to 3ds/PROGRESS.log (`HH:MM ID text`). Logs, traces and shots go under 3ds/logs/ (git-ignored).

## 5. Slim agent prompt template
```
Repo: c:\Users\Andrew\Documents\GitHub\papermario-3ds-port. Branch main.
Read 3ds/AGENT_RULES.md, HANDOFF.md only if the task needs it, and row <id> in 3ds/SLICES.md.
Task: <slice id and text>
Set PM_ROW=<row id> in every test shell.
Verification: <the done check of the row>. Say what you checked and what you did not check.
Append one line per step to 3ds/PROGRESS.log. Add at most 3 lines to HANDOFF.md. Put detail in docs/HISTORY.md. Mark the row done: move it to docs/SLICES_DONE.md.
Overnight mode: a question for the user goes to 3ds/QUESTIONS.md. Keep working on the rest.
No push. No Claude or Anthropic attribution. No stubs (write BLOCKED: <reason> in HANDOFF.md and stop). Commit locally, plain English, only your files.
Write notes and replies in ASD-STE100 English.
Report (5 lines or fewer): slice id; last commit hash; result; what was and was not checked; BLOCKED or not.
```
Pass `model` to the Agent tool by the MODEL RULE.

## 6. State
- The current state is in docs/ORCH_STATE.md. Read that file first. It holds the running work, the done list, the open queue and the resume steps.

## 7. Stop conditions
- A slice agent reports BLOCKED (a stub looks necessary).
- Two failed builds in a row for one slice.
- A regression: the smoke check, the normal chain or a tour gets worse.
- A question only the user can answer: queue it in QUESTIONS.md, then continue with other rows.
- S28 and any hardware step: wait for the user.
- The queue is empty.

## 8. Start prompt for the new chat
```
Read docs/ORCH_STATE.md first. Then read 3ds/ORCHESTRATOR.md and HANDOFF.md. You are the orchestrator only. Do not read or write code. Subagents do all work. Follow section 6 of docs/ORCH_STATE.md to resume, and the workflow and reading rules in 3ds/ORCHESTRATOR.md. Tell me the status in 10 lines. Ask me the open questions in docs/ORCH_STATE.md section 5.
```
