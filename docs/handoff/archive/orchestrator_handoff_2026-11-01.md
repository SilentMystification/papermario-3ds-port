# Orchestrator handoff (2026-11-01)

For the next orchestrator. Read CLAUDE.md (rules 1 to 9) first, then this file.
Chat in ASD-STE100. Status line each turn: RUNNING / READY-NOT-LAUNCHED / BLOCKED-ON-USER. Expand every acronym and row code. No praise. Do not stop: act on results, check agents directly if a report is overdue.

## 1. State
- Branch main, HEAD f2e5ee7ee (docs). Ahead of origin; the user pushes, agents never push.
- Uncommitted in the tree (do not commit blindly):
  - 3ds/src/pm_gbi.c: an UNFINISHED edit from the stopped UC2 agent (word-75 log check). Resume it or revert it with Edit. Not tested.
  - 3ds/src/map_restore_table.c: the user file. Never commit, never discard.
  - 3ds/logs/perf/pushed_builds.txt, 3ds/tools/azt_scenarios.json, docs/handoff/test_catalog.md: hunks from several agents. Commit only the hunks you own.
- No Azahar running. No agent running.
- Goal: O3DS 30 fps (33.4 ms). Console is the judge; Azahar gives direction only. No culling of any kind (default game behavior).

## 2. The display-list path (current)
Chosen path: stateful records on the COPY path (records copied into the frame command buffer, no jump).
- Jump path CLOSED: docs/handoff/decision_jump_closed.md. Console stopped after the map load (area 5 map 5, frame 327) in every variant (JT5 to JT11). Azahar passed. Research found no public example.
- Passed and committed:
  - SR1 db05981f1: version counters (no picture change).
  - SR2 70a34af9a: static matrix skip (pictures equal).
  - RC1 (re-applied as RK1 a057a7ef1): record capture. Window 2 cap 0.
  - RK2 2f1ab87bc: texture versions, arena reuse, churn guard. Window 2 cap 4.
  - UC1 d463ff287: forced uniform shape and patch walker. Gate changed by the user's agent decision: pt check on yard (72), not mac01t_rec (captures empty there). Pictures equal.
  - UC2 check 41a82a858: replay check (hit, bad, miss on the rec line). Debug tool, stays.
- Failed, mismatch still open: UC2 mask (index-address word, reverted). Bad counts: mac01t_rec 9700, yard 4222 (must be 0).
- Current root cause (CONFIRMED by log): first differing record word on yard_rec is j=75, register 0x104 (alpha test state). Walk value 0x00000000 (record), frame value 0x00000110. Diagnosis: docs/handoff/diag_reg104.md (INFERRED). Next: confirm word 75 with one log line, then put the alpha-test state into RecKey (pm_gbi.c ~3352), then gate.
- Dropped: RC1 original gate (capture count too high, then fixed by RK1). EPO1, SORT1, MTXM (memo table; user to decide), GEN0 (op count unchanged), DBGF, HOOK1, MCEN, G2T (every-op check slower).
- Next rows after UC2 passes: UC3 (replay, uniform patch at replay), then the console A/B push for the copy path.

## 3. Earlier results (kept)
- LB1, LB1b, MR1, TBREP (52d082f32, kept; skip count 19.8 vs written gate 20), BAT1 (c4ebe5451), FLH (589021dd3), TEXH (08b1848ed), LAY2 (9bb97f3fb, no gain), TST1 (88d689bcd, no gain), OS1 (reverted; no speed gain on console).
- Console fps with logging off: 24 to 28 (memory: project_console-fps-logging-off.md). Phase-on numbers run slower than play; judge speed with logging off.
- Profiling mode: PF1 6656d5870, PF2 286dcac01, PF3 9491c1222. Switch `prof` (default N = 30). Console has not yet confirmed the counter fields.

## 4. Tools and infrastructure (new)
- Push: `PM_ROW=PUSH sh .../3ds/push_3ds.sh --no-build --ip 192.168.1.170 <switches>`. Switches reach the game; `prof`, `pmu`, `fsplit` lines need 300 counted drawn frames. Logging must be On (options panel).
- The orchestrator RUNS console pushes itself now (user instruction). Tell the user if the console is not ready. Build from HEAD first, then push.
- Live log: build3ds/hw_live_log_170.txt. It is buffered; a hard stop loses the last lines. FTP pull is the fallback (port 5000). It was refused at times.
- Reader: 3ds/tools/livelog.py ends on exit lines, watchdog line, or 5 s after close, or 60 s of silence (--silence). Does not send quit on silence.
- quit: 3ds/tools/quitgame.py sends `quit`; the game runs its normal exit path. Launcher return to homebrew menu is unverified; Y press is still needed after each push (NL1 unsolved: no libctru API; Luma hb:ldr route unconfirmed).
- Launch arguments override saved options (CFG1, 32c5635d3): argument > options.ini > default.
- Azahar: scenarios close Azahar at their until line. Do not leave Azahar open unless the user asks, and close it when asked. Check `tasklist | grep azahar` before a start.
- Remote harness: plan only (docs/handoff/plan_console_harness.md, H1 first row). Scope: log-only tests, no screenshots. Rule 6 (agent-run pushes) to change only after the harness works unattended (user decision).

## 5. Open user decisions (ask each once, plainly)
1. MTXM (a larger memo table would break the 16 KB cache rule): dropped in favor of SR2 (static matrix skip). Confirm?
2. Keep TBREP (skip count 19.8 vs 20 gate)? Recommended: keep.
3. The no-culling wording: is the existing group skip (dlc_box_out, ~50 percent of groups skipped, picture unchanged) allowed? The user's last answer was "no culling unless it changes the game"; confirm.
4. Battle capture for BATTLE (post-battle slowdown check): yes or no.
5. Fountain water photo and Toad Town ground flicker photo.
6. Revert or resume the unfinished pm_gbi.c word-75 edit (see section 1).

## 6. Rules the user has set (hard lessons)
- Do not stop. When a report is overdue, check the agent (ListAgents, a direct message) and act. Do not wait on a background wait for minutes.
- Do not leave Azahar or the game running after a run unless asked. Close only the process you started (by PID).
- Do not give the user a command to run; run the push yourself and report in one line if it fails (PUSH FAILED / NO LOG, stage).
- Do not wait for a test to "end" on the console. Read the current log when asked.
- One code agent at a time. Diagnoses go to fresh agents. Gates are not skipped; an incomplete gate is INCOMPLETE, not failed. Budget reset with -ResetBudget in the same turn.
- No attribution, no stubs, no toggles left in finished code. Debug tools may stay (behind their switch).
- Expand every acronym and row code. Full task names with the code.
- No culling of models, groups or lists (user rule), unless the user explicitly changes it.
- Chat: ASD-STE100. No praise. No opening agreement. State the action and the result.

## 7. Next steps (in order)
1. Resume or revert the unfinished pm_gbi.c word-75 edit (section 1).
2. Confirm word 75 (one log line, one yard_rec start, leave Azahar closed after the run).
3. Put the alpha-test state into RecKey, gate (bad 0 per window on yard and mac01t_rec, miss unchanged, pictures equal to HEAD).
4. UC3 (replay), then the copy-path console A/B push (run the push yourself after the build; tell the user if the console is not ready).
5. Remaining user decisions (section 5).
