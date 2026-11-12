# Row TOOLDEDUP: remove the repeated note lines from runs.jsonl (one time; needs the user's yes: runs.jsonl is append-only)

(Model: Haiku. One script run and one commit. Blocker: TOOLB4 merged (else the next merge grows the file again) and the user's
yes: CLAUDE.md 9 says runs.jsonl is append-only. This row removes only exact duplicate lines that have no run_id, so no run and
no note is lost: every distinct line stays once, in its first position.)

## Facts (ORACLE6, main 1fb48358e)
3ds/logs/perf/runs.jsonl: 1,773,559,010 bytes, 5,230,161 lines, 4,750 distinct. Four correction-note lines (FADV run 497 and
POND run 480, two spellings each) repeat 1,017,063 to 1,595,619 times each: merge_row.sh:82 copied all of main's notes again at
every merge (row_TOOLB4.md). bench.py holds runs.lock while it parses this file and every worktree's copy (append_runs
:302-324, rebuild_db :384), so other bench processes wait more than 120 s and fail ("runs.lock held for 120 s").

## Start
Main folder, no worktree (the file is main's). No bench process may run: the orchestrator runs it between coder batches.

## Exact edits
1. `python -c` script: read 3ds/logs/perf/runs.jsonl line by line; keep a line when it has `"run_id"` (run lines are never
   touched, duplicates of run lines included) or when it is the first copy of that exact text; write to runs.jsonl.new; replace
   the file. Print lines in, lines out, bytes out.
2. Worktree copies are not edited (coders run in them). Their notes are dropped at their merge by TOOLB4's awk. Until those
   trees are merged, append_runs still parses their big copies, so lock waits shrink as they merge.
3. `python 3ds/tools/bench.py rebuild`: no new warning line beyond the ones of the old file (the derived-id warnings of run lines
   stay as they were).
4. Commit main's file with `git add -f` ("runs.jsonl: drop 5.2M repeated note lines (TOOLDEDUP); no run line changed").

## Gate (expected values)
- Lines out about 4,750 plus the duplicated run lines (all run lines kept: `grep -c '"run_id"'` equal before and after: 4797 at
  writing); bytes out a few MB.
- `bench.py run nd_host --row TOOLDEDUP --why "TOOLDEDUP: append after the cleanup"` appends one run without a lock wait.

## Stop
A run-line count that differs before and after: stop, restore the file from git, hand back. Delete this row file in the
completing commit.
