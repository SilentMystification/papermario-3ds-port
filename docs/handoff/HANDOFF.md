# HANDOFF (orchestrator, overwritten at each handoff)

State: main at the last commit (git log). The orchestrator chain changed: the orchestrator talks to the user, the dispatcher runs all agents.
Start: read this file, docs/handoff/orchestrator_rules.md (its CORE block is printed every turn by a hook), CLAUDE.md, docs/handoff/dispatcher.md, docs/handoff/push_runbook.md and the memory index. Then `date`, ListAgents.

## First actions
1. No dispatcher exists yet (build3ds/out/dispatch_state.md is absent). Start one: Agent, `subagent_type: general-purpose`, `model: haiku`, prompt `Dispatcher. Read docs/handoff/dispatcher.md and build3ds/out/dispatch_state.md (create it).` Then write the first orders by SendMessage: HOLD none, start coach NDR with quota 1 (row NDR0G2).
2. The chain was tested at depth 3 with Haiku agents (proxy, lead, coder) and worked. The oracle type now has the Agent tool (COACH rule 7). The coach oracle path itself is untested: the first real run is NDR0G2.
3. The user wants a cleanup row (stale plans in docs/handoff, 14 worktrees of 8 GB in build3ds/wt, 36 branches, 698 files in build3ds/out, duplicate memory notes). The user has not given the word for deletions.
4. No console check until the last row (S9GATE). The O3DS build check list is docs/handoff/verify_list.md. Consoles: O3DS 192.168.1.170, N3DS .224, O3DS XL .131. Push only on the user's word.

## Work state
Goal: remove the display list system. Done only when `elfgate.py` prints `func 0 data 0 gfxnames 0` and all `listcen.py` totals are 0 (`sh 3ds/tools/epic_status.sh`). Plan: plan_finish.md. Oracle facts: oracle_handoff.md.
Ready pages on main: NDR0G2 (starts now), then NDR1C, NDR2A, NDR2B || NDR2C || NDR4, NDR2D. NCONV1D needs a design oracle first, then NCONV2, NREC1A-5 and the DEL rows are written.
User decisions: expected-diff gate for NDR0G2 and NDR1C (option A), cap 5 coders, Sonnet rows approved (NMDL0 NTGT NIMGM MAPCONV NCONV1D), UD4 keep nd_comb_core on core 1, plan scope = end proof only, dschk_base.txt write allowed for every NST2 row, Azahar runs need no permission.
Parked: ASSETFREE, CIA, QTM. Build pipeline: build_pipeline.md (repeat build 4 s, fresh lane 18 s, cold 93 s). Merge: merge_protocol.md (a Haiku merger).

## User rules that cost time before
- A user question is answered first. A state the user reports is fact. No double posts (the Stop hook only logs now). Short chat: one line per merge, no echo of notices.
- ASD-STE100 style in chat: sentences of 20 words or fewer, no semicolons, no agreement or apology phrases.
- No push without the user's word. No AskUserQuestion popup for a push. Hooks block the phrases `debug line` and `log line` in agent messages and block sleep loops.
