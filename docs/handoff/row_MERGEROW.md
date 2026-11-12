# Row MERGEROW: one script that merges a finished row branch into main (tool, no game code)

Model: Haiku (a shell script that runs fixed git commands in a fixed order, plus a host test; no design choice).

## Start
`sh 3ds/tools/wt_new.sh MERGEROW`; PM_ROW=MERGEROW. No Azahar run, no game build.

## Files
3ds/tools/merge_row.sh (new), 3ds/tests/mergerow/run.sh (new host test). No other file.

## Script `sh 3ds/tools/merge_row.sh <row> <message-file>` (run in the main folder; Git Bash)
1. Refuse (exit 2, one line) when: the main folder has uncommitted or untracked files (`git status --porcelain` not empty); the branch
   `<row>` does not exist; the current branch is not main.
2. Conflict check without touching the tree: `git merge-tree --write-tree main <row>`; exit code 1 = conflicts. Print the conflicted
   paths. If the only conflict is `docs/handoff/row_<row>.md` (deleted on the branch, edited on main), go on; else exit 3.
3. Marker grep: `git diff main...<row>` added lines that match `^(<<<<<<<|=======|>>>>>>>)( |$)`: exit 4 with the lines.
4. `git merge --no-ff -F <message-file> <row>`. On the row-file conflict: `git rm -q docs/handoff/row_<row>.md` then
   `git commit --no-edit -F <message-file>`. Any other stop: `git merge --abort`, exit 5.
5. runs.jsonl: the lines of build3ds/wt/<row>/3ds/logs/perf/runs.jsonl that main's 3ds/logs/perf/runs.jsonl does not have (compare
   by the run_id field), appended in file order to main's file; commit "runs.jsonl: <row> runs" when any line was added.
6. `sh 3ds/tools/wt_gc.sh --only <row>` then `git branch -d <row>` (not -D: an unmerged branch stays and the script says so, exit 6).
7. Print one summary: merge commit hash, runs lines added, wt_gc result.

## Host test (3ds/tests/mergerow/run.sh)
A temp repo (mktemp -d, git init, main branch) with: (a) a clean branch -> merged, branch deleted; (b) a branch that deleted
docs/handoff/row_X.md while main edited it -> merged, file gone; (c) a real conflict in another file -> exit 3, main unchanged;
(d) a branch with a `<<<<<<< ` line -> exit 4; (e) a dirty main -> exit 2; (f) runs.jsonl: 2 new run_id lines appended, 1 known
skipped. wt_gc is stubbed by a PATH shim in the test (WT_GC override variable read by the script). Expected: `mergerow: 6/6 PASS`.

## Gate
`sh 3ds/tests/mergerow/run.sh` prints `mergerow: 6/6 PASS`. Commit the two files; delete this row file in the same commit.

## Stop
A case that fails twice: stop, commit, hand back the test output.
