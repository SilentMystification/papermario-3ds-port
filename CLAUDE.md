# papermario-3ds-port rules

Orchestrator: read docs/handoff/orchestrator_rules.md at the start and after every compaction. Code agents: read your goal page
(docs/handoff/ws_<id>.md) first, then this file only. Edit rules in place, never append.

1. BUGS. A fault the user reports is fact. A state the user reports (the game runs, the console is online, FTP or the netloader is open) is fact too: never probe it to doubt it. When a tool cannot reach it, read the tool and the game code for the cause. Read the code (producer, state, draw call, pixel) to a file, line and value. Fix each
   path that can cause it. No repro runs, no debug-log runs, no premise-check runs. No cause found: hand back the trace. The
   orchestrator asks the user.
2. NO VISUAL CHECKS. No agent looks at a shot, describes a picture or counts pixels. Proof is the code read, the host unit tests
   (nd_host) and the mechanical byte compare (`bench.py base <bench>`, then `bench.py cmp <bench> --row <id>`).
3. TESTS. CAPTURE: before the user spends console time on a capture, run the same launch line once in Azahar and check that the expected log lines appear: `python 3ds/tools/capture_check.py --row <id> --proof "<lines>" <push game words>`. Push only after CAPTURE CHECK OK. State each link in the hand-back (switch, sink, tool, proof line). Read the real command in the code, never its echo line. The main proof is the host unit tests (`sh 3ds/tools/nd_host_test.sh`): they run in seconds. The emulator runs once per
   group of files, never per file. A gate costs at most 1 build and 2 Azahar runs. Every run uses
   `python 3ds/tools/bench.py run <bench> --row <id> --why "<text>"` with PM_ROW=<id> and PM_LANE=<id>. A scenario has an
   until-line (a log event of the subject), ends within 5 s of it, and has a cap of 10-25 s (45 s max). A new scenario and its
   bench go in one commit. Budget: 8 Azahar starts per task. Coders run all tests. The oracle runs none (user 2026-11-17).
4. TOOL FAULTS STOP THE GATE. These faults stop the gate: a base run that fails or hangs, an abnormal exit, a stale build, a shot
   copy failure, two runs that disagree. Hand back the exact output. Never skip a scene, change the base or reset the budget to
   pass. The orchestrator resets a task's budget once per designer correction (user, 2026-11-12). Every compare is exact: one
   differing bit fails the gate, no rounding exception (user, 2026-11-12).
5. STOP RULE. Fix a mechanical fault of the goal page or of a tool yourself and note it in the commit. Any other failed gate or
   unexpected fact: the coder does not diagnose (user 2026-11-17). Send the output path to the designer (oracle) with SendMessage,
   apply its reply and rerun (3 rounds per cause, then hand back). Go on with a next group that does not depend on the answer. Hand
   back only when the task is done, when only the user can decide, or after 90 minutes without a commit.
6. WORK. One command per step: write a script, not many small commands (every command re-reads your whole context). Grep and read
   line ranges. Write tool output to build3ds/out/ of your own worktree and read the last 20 lines. Run builds and benches in the
   foreground (timeout up to 600000 ms). No model polling: never sleep, tail --pid, Monitor or ping loops as separate calls. A job
   the harness moved to the background: wait for it in ONE foreground call of exactly this form: until test -f <done file>; do sleep 10; done
   (timeout 590000). Repeat that one call. Never end your turn while your own job runs: each stop wakes the orchestrator (user 2026-11-16).
   Stop every background task you start. Hand back with ONE SubagentHandback message (commit hash, gate values, counter change) and
   one short last line. No hand-back file.
7. CODE. Commit each finished piece (changed files only). No feature toggles, no stubs, no git push, no git stash. Claude
   attribution lines in commits are allowed (user, 2026-11-12). Debug tools may stay. Untested code stays on a branch. Follow
   docs/handoff/hw_guidelines.md (ARM11: hot loops fit 16 KB L1, no division, no double, sequential access).
8. WORKTREES. One per agent: build3ds/wt/<id>, branch <id>. The orchestrator merges and cleans up. Workstreams own disjoint
   directories, so merges do not conflict. Windows: repo-relative paths, the .ps1 tools, Git Bash for sh tools. build.sh generates
   map_restore_table.c and map_restore*.ld (never committed). Never edit a shared registry (gen_gfx_tab.sh GCONV_SRC lines,
   azt_scenarios.json, bench_defs.json) unless the goal page says so. Do not merge main into your branch unless told.
9. DOCS. State lives in a few files, edited in place. Git history is the log. The commit that completes a task deletes its goal
   page. Numbers go into the bench database (runs.jsonl is append-only), not into prose. Only the oracle and the orchestrator add a
   docs/handoff file. `ls docs/handoff` is the index (INDEX.md is gone, user 2026-11-16).
10. CLEAN MAIN. The main folder has no uncommitted or untracked files. Every file is committed or listed in .gitignore.
