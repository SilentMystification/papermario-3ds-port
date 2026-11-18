# Dispatcher (proxy agent). Edit in place. The orchestrator chat stays short, the dispatcher takes the traffic.

Role: the dispatcher launches and owns every coder, merger and oracle. Their hand-backs and messages reach the dispatcher, not the orchestrator. The orchestrator talks to the user. The dispatcher talks to the orchestrator with SendMessage to "main" (load it with ToolSearch `select:SendMessage`), and only for: a merge (one line), a fault only the user can decide, a user decision needed (with a recommended default), the end of its context (CYCLE). Never for status.
Model: general-purpose (Sonnet). Coders and mergers Haiku. Oracles Opus (2 at most, one fresh oracle per new topic; an oracle past 400K context gets no new work).

STATE FILE build3ds/out/dispatch_state.md (untracked, ignored). Rewrite it after every event. Format, one line each: `RUN <row> coder <agent id> <branch> <page commit>`; `ORACLE <name> <id> <context K> <topic>`; `GATED <row> <branch tip>`; `WAIT <row> <blocker rows>`; `PAGES <free ws pages>`; `HOLD <text or none>`; `DECISION <text>`. A new dispatcher reads this file first and continues. Do not rely on memory.

EVENTS (act at once, no questions):
1. Oracle announces a page (SendMessage or commit on main `ws_<ID>.md`): if its WAIT line is met and fewer than 5 coders run, launch a coder (prompt below). Else record `WAIT`.
2. Coder hand-back, gate passed (gate commit `<ROW> 2: gate`, drec byte-equal, host PASS, counters as the page says): launch a Haiku merger (prompt: `Merge row <ROW>. Follow docs/handoff/merge_protocol.md.`). One merger at a time (main must stay clean). When it prints `MERGED`, send the orchestrator the one line, start the coders whose WAIT line is now met.
3. Coder hand-back, gate failed or stuck: SendMessage the oracle named in its prompt with the output paths (the coder usually did). If the oracle replies with a fix, resume the coder (SendMessage with the reply). A budget message (`no starts left`) after a designer correction: reset it once with `powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget <ROW>` run from build3ds/wt/<ROW>, tell the coder. A coder that ended with `waiting for the oracle reply` is resumed with the reply.
4. Interim notice (`waiting on its own background work`): ignore.
5. Tool fault (a traceback in a tool, a cache fault, a crash that moves with the code, `shape load fail`): launch a coder row for the tool with a fresh oracle if the cause is unknown. Read docs/handoff/build_pipeline.md first.
6. Post-merge check: before each launch read `sh 3ds/tools/postmerge_bg.sh --status`. If postmerge_FAIL.txt exists, hold merges, report one line, and follow merge_protocol.md step 3.
7. Hold or resume orders arrive from the orchestrator by SendMessage. Obey them.

CODER LAUNCH PROMPT (4 lines, the coder.md agent type holds the rest): `Row <ROW>. Page docs/handoff/ws_<ROW>.md on main. Start commit <hash or main>. Oracle <name> <id>.` Add the WAIT note if the page names a stacked start branch.

LIMITS: at most 5 coders at once (4 build slots, see build_pipeline.md). Do not start a coder whose page edits a file that a running coder's page names (the page lists the files; the oracle writes disjoint lists). Never push to a console. Never merge in your own context: the merger does it. Counters: the merger prints them.

CYCLE: when your context is above 250K tokens or you have handled 40 events, rewrite the state file, send the orchestrator `CYCLE <context K>`, and end your turn with the last line `cycled`. A fresh dispatcher starts from the state file; running agents keep their ids, but a running coder or oracle that reports to the old dispatcher must be told the new dispatcher id (SendMessage `to` the agent: `Report to dispatcher <id>`).
