# Orchestrator handoff

Written 2026-10-17 for a new orchestrator chat. Read this file, then HANDOFF.md, 3ds/SLICES.md and 3ds/QUESTIONS.md.

## 1. Role
- The orchestrator only orchestrates. It never writes code. It never reads code or project source.
- Subagents do all work: code, scans, reads, queue edits, notes.
- Launch ONE slice agent at a time for anything that builds or runs Azahar. The build directory is shared. Only one Azahar window can run. Run the agent in the background.
- Read only the agent report and the last lines of 3ds/PROGRESS.log.
- Relay each report to the user in plain terms: what was done, the numbers, what was not verified, the next step. Then launch the next slice.
- Read-only research agents (Explore type) may run in parallel with a build agent.
- Post a status to the user after every report. Never leave the user without feedback during a long run.
- Start no subagent the user did not expect. Stop at a finished slice when the context grows long. Write a short HANDOFF.md note and ask the user to start a new chat.

## 2. User decisions (2026-10)
- All subagents use the default model. No tiers.
- Never push to git. No Claude or Anthropic attribution in commits or PRs.
- NO STUBS. Never write a stub or stand-in for decomp code or data without asking. A port-layer shim is allowed only for real N64 OS or hardware functions with no game logic.
- Fix root causes. Never hide a fault with a guard.
- Q1 (a): merge the whole real battle tree (branch s19-full-battle), everything resident, no stand-in table.
- Q2 (a): keep needed assets resident in RAM. No SD streaming during play. The ROM file itself need not stay resident.
- Q4: merge battle first. Then link the demo maps.
- Q5: the obfuscation shims stay ignored (empty). The crash screen functions only if the debug output can be used (slice S31 checks). Else leave them.
- Q6: link all maps. Measure first. The memory ceiling stopped at 267 of 421.
- The game runs at exactly 30 fps (PC01). A 30/60 toggle comes later (F06).
- The O3DS is the hardware target. It should reach full speed. Performance work comes after everything works.
- ARM11 math audit: experiments allowed. Game-logic math changes need a replay check and the user approval.
- Use libctru natives for threading and sync.
- The log is non-blocking on core 1 (L01).
- Workload split: core 0 = logic plus the CPU part of the RSP work. Core 1 = audio. PICA GPU = rasterization plus vertex work. Consider ndsp for audio mixing.
- Testing: feature tests use the warp with -Turbo -Smoke and a fixed length. Never an open wait. Tours iterate on the failing set. Run the full tour once at the end. Run the full normal chain only for slices that touch boot, title, file select, intro or the map change path.

## 3. How to run
- Debug switches go in `debug3ds.txt` or `-Debug "..."`. Never leave `fast` in debug3ds.txt.
- Switches: `fast` (skip logos and the 30 Hz pad wait), `autoa`, `freshsave`, `shots`, `padscript=`, `padrand`, `warp=<map>:<entry>`, `story=`, `preset=` (intro yard toad toad_east toad_west osr kmr02), `partner=`, `loadslot=`, `smoke`, `smokeframes=`, `tour=@file`, `turbo`, `rmlog`, `rmoff`, `phase` (`phasecsv`, `phasegbi`), `ptrcheck`, `romstrict`, `romtrace`, `logstat`, `lognull`, `logflood`, `audiostat`, `stats`, `scripts`, `faulttest`, `crashtest_call`.
- Main command: `powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Debug "fast preset=toad"`. Other options: `-Timeout`, `-Stall`, `-Until`, `-Gdb -GdbScript`, `-TurboFactor`. Exit 0 = ok, 1 = failed, 2 = unmapped access or crash.
- Normal chain: `-Debug "fast autoa freshsave" -Stall 40`. Use run_azahar.ps1, not go.ps1.
- Never start azahar.exe with other arguments than the .3dsx path (modal dialog).
- Hardware skills: `pull-n`, `pull-o` (pull log and crash dumps), `push-a` (Azahar), `push-n` (N3DS 192.168.1.223), `push-o` (O3DS 192.168.1.168).
- Progress log: each agent appends one line per step to 3ds/PROGRESS.log (`HH:MM ID text`). Logs, traces and shots go under 3ds/logs/ (git-ignored). Never commit logs. Do not run `git checkout` on tracked log files.

## 4. Agent prompt template
```
Repo: c:\Users\Andrew\Documents\GitHub\papermario-3ds-port. Branch main. Do not git push. Add no Claude or Anthropic attribution to commits. Commit locally in plain English. Commit only your own files.
Read HANDOFF.md (sections "Do not", "Warp" and the result of your slice) and 3ds/SLICES.md first.
Append one line per step to 3ds/PROGRESS.log (format: HH:MM <ID> text). Write logs and shots under 3ds/logs/. Never commit them.
Rules:
- No stubs. Never write a stand-in for decomp code or data. If a stub looks like the only option, write BLOCKED: <reason> in HANDOFF.md and stop.
- Fix root causes. No guard that hides a fault.
- Testing: warp with -Turbo -Smoke -FaultStop and a fixed length. Run the full normal chain only if the slice touches boot, title, file select, intro or the map change path.
- Threading and sync use libctru natives.
- Write notes and replies in ASD-STE100 English: short sentences, one instruction each, active voice, no semicolons, no praise.
Overnight mode: if you need a user answer, add the question to 3ds/QUESTIONS.md and keep working on what you can do.
Task: <slice id and text from SLICES.md>
Verification: <the done check from SLICES.md>. Say what you saw by eye and what you did not check.
Write a result section in HANDOFF.md and mark the row in SLICES.md.
Report (5 lines or fewer): slice id; last commit hash; result; what was and was not checked by eye; BLOCKED or not.
```

## 5. State (2026-10-17)
- RUNNING when this file was written: S25 (Fast3D gaps, combiner audit, pm_comb.c). Its row in SLICES.md still shows `[ ]`. PROGRESS.log shows its commit af71c887b at 02:30. It may still run. Read its report in the S25 note in HANDOFF.md and the S25 row in SLICES.md before you launch anything.
- Queue, in order:
1. S25: finish, read the report. The battle part of S25 needs the merge.
2. PC01 (HIGH): exact 30 fps pacing in pm_frame_loop. Measure steps per second first. No input needed.
3. S28: hardware run on the O3DS and N3DS (boot line, audio cost, FPS). Needs the user: O3DS on, dspfirm.cdc on the SD card.
4. H03: derive the memory split from measured need. Needs the S28 boot line.
5. S31: check whether the crash screen can use the debug output (Q5). No row exists yet in SLICES.md. Create it.
6. S30b: link kpa, kzn, flo, jan, sam, pra (154 maps). Needs Q7.
7. F01 remainder: fault sweep paths (battle after the merge, pause, save, zones). Row is `[~]`.
8. F03: depth read back for `is_point_visible` (NULL z-buffer). No input needed.
9. T02: run_azahar.ps1 safe config (per-run config copy, mutex, try/finally). Low priority.
10. V02: wall over bushes. Not reproduced. Needs the user details (section 6).
11. Battle merge: branch s19-full-battle (Q1 a). Rebase first. `pm_title.c` was deleted on main, so the `battle` and `demobattle` hooks need a new place. Needs the memory answer (Q7).
12. S19b: link src/world/model_anim/kzn/*.c. After the merge.
13. S19d: real encounter start. After the merge.
14. S29: kkj_00 chain, title demo timer. After the merge and the demo maps (Q4).
15. Remaining 154 maps (same as S30b if not done earlier).
16. Performance phase (HANDOFF "Performance phase"): PO2 vertex once at G_VTX, GPU transform, audio on core 1 or ndsp, ARM11 math audit, F06 30/60 toggle plan. Re-measure on the O3DS.
17. Final re-audit of all port files (like S20): list stand-ins, confirm each is gone or approved.

## 6. Open questions for the user (from 3ds/QUESTIONS.md)
- Q7: fit all maps plus the battle tree? All 418 maps leave about 2.6 MB (O3DS estimate). The battle tree needs about 3.9 MB. Options: stream rare map data, shrink the resident ROM, accept less spare, or wait for S28.
- PO2: transform each vertex once at G_VTX (CPU only, small change). Not in QUESTIONS.md yet. Ask the user.
- V02: where exactly does the back wall render over the bushes? Map, moment, camera angle, device. A photo helps if it is hardware only.
- Is the O3DS at 192.168.1.168 on, with dspfirm.cdc on the SD card, for S28?
- QUESTIONS.md still lists Q1, Q2, Q4, Q5 as open. Section 2 holds the user decisions. Ask an agent to update QUESTIONS.md.

## 7. Start prompt for the new chat
```
Read 3ds/ORCHESTRATOR.md, HANDOFF.md, 3ds/SLICES.md and 3ds/QUESTIONS.md. You are the orchestrator only. Do not read or write code. Subagents do all work. First check the running agent state: read the last 20 lines of 3ds/PROGRESS.log and the open rows in 3ds/SLICES.md. Tell me the status in plain terms. Then launch the next slice with the template in 3ds/ORCHESTRATOR.md, one build agent at a time, in the background.
```
