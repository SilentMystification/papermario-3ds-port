# papermario-3ds-port rules

**TWO RULES FOR EVERY WORD YOU WRITE FOR THE USER (his order): 1. DO NOT GLAZE HIM: no praise, no flattery, no "you are right", no
apology paragraphs; state the fact, the number, the next step. 2. WRITE EVERYTHING IN ASD-STE100 (chat, plans, rows, reports):
short sentences, one instruction each, active voice, simple tenses, no semicolons.**

Orchestrator: read docs/handoff/orchestrator_rules.md at the start and after every compaction. Code agents: read your row file
(docs/handoff/row_<id>.md) first. Edit rules in place, never append.

1. BUGS. A fault the user reports is fact. Read the code (producer, state, draw call, pixel) to a file, line and value. Fix each
   path that can cause it. No repro runs, no debug-log runs, no premise-check runs. No cause found: hand back the trace. The
   orchestrator asks the user.
2. NO VISUAL CHECKS. No agent looks at a shot, describes a picture or counts pixels. Proof is the code read, nd_host and the
   mechanical byte compare (`bench.py base <bench>`, then `bench.py cmp <bench> --row <row>`) on at most 2 scenes that use the
   changed shared draw code. The user checks the look on the console.
3. TESTS. Every run uses `python 3ds/tools/bench.py run <bench> --row <row> --why "<text>"` with PM_ROW=<row> and PM_LANE=<row>.
   Test only what the row changes. A scenario has an until-line (a log event of the subject), ends within 5 s of it, and has a cap
   of 10-25 s (45 s max). A new scenario and its bench go in one commit. Budget: 8 Azahar starts per row.
4. TOOL FAULTS STOP THE GATE. These faults stop the gate: a base run that fails or hangs, an abnormal exit, a stale build, a shot
   copy failure, two runs that disagree. Stop and hand back the exact output. Never skip a scene, change the base or reset the
   budget to pass. Exception (user, 2026-11-12): the orchestrator resets a row's budget once per oracle correction when the
   oracle corrected a row spec fault (not a tool fault). Every compare is exact: one differing bit fails the gate. There is no
   rounding exception (user, 2026-11-12).
5. STOP RULE. A failed gate or an unexpected fact: stop, commit, hand back the values. Do not choose a new approach. Hand back
   after 90 minutes or 150 tool calls without a passed gate. 2 tries per named cause.
6. WORK. Grep and read line ranges. Write tool output to build3ds/out/ of your own worktree. Run builds and benches in the
   foreground (timeout up to 600000 ms). No Monitor or tail loops. Stop every background task you start.
7. CODE. Commit each finished piece (changed files only). No feature toggles, no stubs, no git push, no git stash. Claude
   attribution lines in commits are allowed (user, 2026-11-12). Debug tools may stay. Untested code stays on a branch. Follow docs/handoff/hw_guidelines.md (ARM11: hot loops fit
   16 KB L1, no division, no double, sequential access).
8. WORKTREES. One per agent: build3ds/wt/<row>, branch <row>. The orchestrator merges and cleans up. Windows: repo-relative paths,
   the .ps1 tools, Git Bash for sh tools. build.sh generates map_restore_table.c and map_restore*.ld (never committed).
   Rows that run together may share a file only when they edit different places in it (the row names its places; docs/handoff/places.md
   lists the open ones), so merges do not conflict. Never edit a shared registry (gen_gfx_tab.sh GCONV_SRC lines, azt_scenarios.json,
   bench_defs.json): use your own per-row file when the row says so. Do not merge main into your branch unless told.
9. DOCS. State lives in a few files, edited in place. Git history is the log. The commit that completes a row deletes its row
   file. Numbers go into the bench database (runs.jsonl is append-only), not into prose. Add a docs/handoff file only for a new
   subject, with its INDEX.md line.
10. CLEAN MAIN. The main folder has no uncommitted or untracked files. Every file is committed or listed in .gitignore.
