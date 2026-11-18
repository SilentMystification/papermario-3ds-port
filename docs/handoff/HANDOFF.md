# HANDOFF (orchestrator, overwritten at each handoff)

State: main at the last commit (git log). The orchestrator chain changed: the orchestrator talks to the user, the dispatcher runs all agents.
Start: read this file, docs/handoff/orchestrator_rules.md (its CORE block is printed every turn by a hook), CLAUDE.md, docs/handoff/dispatcher.md, docs/handoff/push_runbook.md and the memory index. Then `date`, ListAgents.

## First actions
1. NO dispatcher agent exists any more. The orchestrator starts the coaches (cluster leads, one per topic), design oracles and mergers itself and keeps build3ds/out/dispatch_state.md (duties: docs/handoff/dispatcher.md). Chat: one line per merge, fault or decision.
2. SATURATE every turn: quota 5 coders, a lead writes pages ahead from branch data. Related tasks go to the lead of that topic (SendMessage resumes it), unrelated tasks to a new lead.
3. Read build3ds/out/dispatch_state.md and `git log` first, then ListAgents. Open cleanup (deletions only on the user's word): stale plans in docs/handoff, dead tools in 3ds/tools, duplicate memory notes, unmerged branches, leftover build3ds/wt folders (Permission denied).
4. No console check until the last row (S9GATE). Push only on the user's word.

## Work state
Goal: remove the display list system. Done only when `elfgate.py` prints `func 0 data 0 gfxnames 0` and all `listcen.py` totals are 0 (`sh 3ds/tools/epic_status.sh`). Plan: plan_finish.md. Oracle facts: oracle_handoff.md.
Ready pages on main: NDR0G2 (starts now), then NDR1C, NDR2A, NDR2B || NDR2C || NDR4, NDR2D. NCONV1D needs a design oracle first, then NCONV2, NREC1A-5 and the DEL rows are written.
User decisions: expected-diff gate for NDR0G2 and NDR1C (option A), cap 5 coders, Sonnet rows approved (NMDL0 NTGT NIMGM MAPCONV NCONV1D), UD4 keep nd_comb_core on core 1, plan scope = end proof only, dschk_base.txt write allowed for every NST2 row, Azahar runs need no permission.
Parked: ASSETFREE, CIA, QTM. Build pipeline: build_pipeline.md (repeat build 4 s, fresh lane 18 s, cold 93 s). Merge: merge_protocol.md (a Haiku merger).

## User rules that cost time before
- A user question is answered first. A state the user reports is fact. No double posts (the Stop hook only logs now). Short chat: one line per merge, no echo of notices.
- ASD-STE100 style in chat: sentences of 20 words or fewer, no semicolons, no agreement or apology phrases.
- No push without the user's word. No AskUserQuestion popup for a push. Hooks block the phrases `debug line` and `log line` in agent messages and block sleep loops.
