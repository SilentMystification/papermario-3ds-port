# papermario-3ds-port rules

Orchestrator: read docs/handoff/orchestrator_rules.md at the start and after every compaction. Code agents: read your row file
(docs/handoff/row_<id>.md) first. Rules are edited in place, never appended.

1. BUGS. A fault the user reports is fact. Find the cause by reading the code (producer, state, draw call, pixel) to a file, line
   and value, and fix each path that can cause it. No repro runs, no debug-log runs, no premise-check runs. No cause found: hand back
   the trace; the orchestrator asks the user.
2. NO VISUAL CHECKS. No agent looks at a shot, describes a picture or counts pixels. Proof is the code read, nd_host and the
   mechanical byte compare (`bench.py base <bench>`, then `bench.py cmp <bench> --row <row>`), on at most 2 scenes that use the
   changed shared draw code. The look is the user's console check.
3. TESTS. Every run goes through `python 3ds/tools/bench.py run <bench> --row <row> --why "<text>"` with PM_ROW=<row> and
   PM_LANE=<row>. Test only what the row changes. A scenario has an until-line (a log event of the subject), ends within 5 s of it,
   and has a cap of 10-25 s (45 s max). A new scenario and its bench go in one commit. Budget: 8 Azahar starts per row.
4. TOOL FAULTS STOP THE GATE. A base run that fails or hangs, an abnormal exit, a stale build, a shot copy failure or two runs that
   disagree: stop and hand back the exact output. Never skip a scene, change the base or reset the budget to get past it.
5. STOP RULE. A failed gate or an unexpected fact: stop, commit, hand back the values; do not choose a new approach. Hand back after
   90 minutes or 150 tool calls without a passed gate. 2 tries per named cause.
6. WORK. Grep and read line ranges. Write tool output to build3ds/out/ of your own worktree. Builds and benches run in the
   foreground (timeout up to 600000 ms); no Monitor or tail loops; stop every background task you start.
7. CODE. Commit each finished piece (changed files only). No feature toggles, no stubs, no git push, no git stash, no Claude
   attribution. Debug tools may stay. Untested code stays on a branch. Follow docs/handoff/hw_guidelines.md (ARM11: hot loops fit
   16 KB L1, no division, no double, sequential access).
8. WORKTREES. One per agent: build3ds/wt/<row>, branch <row>. The orchestrator merges and cleans up. Windows: repo-relative paths,
   the .ps1 tools, Git Bash for sh tools. build.sh generates map_restore_table.c and map_restore*.ld (never committed).
9. DOCS. State lives in a few files edited in place; git history is the log. A row file is deleted by the commit that completes the
   row. Numbers go into the bench database (runs.jsonl is append-only), not into prose. A new docs/handoff file only for a new
   subject, with its INDEX.md line.
