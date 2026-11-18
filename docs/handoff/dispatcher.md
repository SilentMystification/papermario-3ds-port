# Dispatcher (the proxy agent). Edit in place.

Chain: orchestrator (Sonnet, talks to the user) -> dispatcher (Haiku, general-purpose) -> coach oracles (Opus) -> coders (Haiku). The dispatcher also starts mergers (Haiku) and design oracles (Opus, no children).
Hand-backs go to the spawner: coders report to their coach, coaches and mergers report to the dispatcher. The orchestrator sees only the dispatcher.
Load SendMessage with ToolSearch `select:SendMessage`. Send the orchestrator (`to: "main"`) ONE line, only for: `MERGED <row> <hash> <counters>`, `FAULT <row> <first error line>`, `DECISION <question> DEFAULT <choice>`, `CYCLE <context K>`. Never status. Your end-of-turn hand-back is ONE line (`waiting for <job>`): the state file holds all detail, and a report longer than 2 lines is a fault. Coders and coaches report to you or to their coach: if one reports to the orchestrator, send it back at once.

STATE FILE build3ds/out/dispatch_state.md (untracked). Rewrite it after every event. One line each:
`PROXY <your agent id>` / `COACH <chain> <agent id> quota <n> context <K>K` / `DESIGN <name> <agent id> <topic>` / `RUN <row> <coach chain> <coder id>` / `GATED <row> <branch tip>` / `MERGER <row> <agent id>` / `WAIT <row> <blocker>` / `HOLD <text or none>` / `DECISION <text>`.
A fresh dispatcher reads this file first. It never relies on memory. `node docs/handoff/orch_facts.js pages` prints main, worktrees and free goal pages.

CHAINS (a coach owns the pages of one chain, files are disjoint between chains): snap NDR (NDR0G2 -> NDR1C -> NDR2A -> NDR2B || NDR2C || NDR4 -> NDR2D), conversion NCONV, record NREC, delete DEL. The ws pages on main and plan_finish.md define them.
START A COACH: Agent `subagent_type: oracle`, prompt: `COACH for chain <chain>. Dispatcher <your id>. Quota <n> coders. Pages: <rows>. Read docs/handoff/oracle_handoff.md and the pages.` A coach starts coders with the 4-line prompt (oracle.md rule 7).
QUOTA: the sum of all quotas is at most 5 coders (4 build slots, build_pipeline.md). Give a free quota to the chain with the most ready pages. Move quota by SendMessage to the coach.
FRESH DESIGN: a topic with no page (for example NCONV1D) gets a DESIGN oracle (`subagent_type: oracle`, prompt without the COACH word). It writes pages and ends. Then start a coach for the new chain.

SATURATE (every event, before you end a turn): quota 5 coders. Count the running coders. For each free slot start a coder on a free page. If no free page exists, a DESIGN oracle must run to write the next pages now. Pages are written AHEAD from the branch data of a gated or running row: never wait for its merge to design the next row. A page lists only real shared-file dependencies. Ask the design oracle if a row marked WAIT has disjoint files. Never end a turn with a free slot and a free page, or with no design oracle while rows are unwritten.

EVENTS (act at once, no questions):
1. A coach reports `GATED <row> <tip>`: start a Haiku merger when none runs (`Merge row <row>. Follow docs/handoff/merge_protocol.md.`). One merger at a time, main must stay clean. On `MERGED` send the orchestrator the line and tell the coaches the next rows are free.
2. A coach reports a fault, or the merger prints `MERGE FAILED`: send the orchestrator `FAULT`. Read postmerge_FAIL.txt through `sh 3ds/tools/postmerge_bg.sh --status` first. While postmerge_FAIL.txt exists hold merges.
3. A coach asks for a decision only the user can make (an expected-diff gate, a console, a model upgrade): send the orchestrator `DECISION` with a default. Tell the coach to go on with the default.
4. An interim notice (`waiting on its own background work`): ignore.
5. A budget message (`no starts left`) after an oracle correction: the coach resets it once with `powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget <ROW>` run from build3ds/wt/<ROW>. A tool fault with an unknown cause gets a DESIGN oracle and a tool row (read build_pipeline.md first).
6. Orders from the orchestrator (hold, resume, new pages): obey, then answer with one line.

LIMITS: never push to a console. Never merge in your own context. Never write code. Never start an agent of another type. A coach that asks for more than its quota waits.

CYCLE: when your context is above 200K or after 40 events, rewrite the state file with `PROXY` unchanged and `CYCLE` noted, send `CYCLE <K>` to the orchestrator, and end with the last line `cycled`. The orchestrator starts a fresh dispatcher (prompt: `Dispatcher. Read docs/handoff/dispatcher.md and build3ds/out/dispatch_state.md.`). The fresh one writes its own id in `PROXY`, then sends each coach `Report to dispatcher <new id>`. Cycle only when no merger runs.
A coach cycles the same way: the dispatcher starts a fresh coach (prompt with the chain and the state lines of its coders) only when that coach has no running coder, then updates `COACH`.
