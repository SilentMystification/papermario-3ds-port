# Merge protocol (a Haiku agent runs it, never the orchestrator chat). Edit in place.

Input: the row name. The branch <ROW> holds a gate commit (`<ROW> 2: gate`). Output: ONE line `MERGED <ROW> <hash> build B data D gfxtype G reader R model M` or `MERGE FAILED <ROW> <reason>`. No other text.
Work in the main folder c:/Users/Andrew/Documents/GitHub/papermario-3ds-port. Run every command that may take over 10 s as one command with a timeout of 400000.

1. Main must be clean (`git status --short` empty). If an oracle edits a file, wait and look again 3 times. Do not commit or delete files of other agents.
2. If the branch deletes docs/handoff/ws_<ROW>.md (it does when the gate commit exists) and main changed that page: `git rm -q docs/handoff/ws_<ROW>.md` and commit `main: drop ws_<ROW> page before the merge of branch <ROW>`.
3. `sh 3ds/tools/merge_row.sh <ROW>`. Its line `merge_row: base adopt: ...` (bench.py base --adopt) is output only and never fails the merge. Exit 8 (post-merge check failed): read build3ds/out/postmerge_FAIL.txt and the output file it names. A tool fault (traceback in a tool, not a game test): report `MERGE FAILED` with the first error line. Never delete the failure file.
4. Conflicts (merge_row reports conflicts in files): `git merge <ROW> --no-ff --no-commit`, then
   - 3ds/tools/fxrest.d/dschk_base.txt: take the branch version (`git checkout --theirs`) when the row page asked for a new base, else keep main (`--ours`).
   - 3ds/tests/host.d/*.sh LIBS lines: keep the union of the stub files of both sides (nd_xf_link_stub.c, nd_texwin.c, nd_host_stub.c, nd_snap_link_stub.c).
   - docs/handoff/ws_*.md: keep main for a page the branch did not delete.
   - Any other conflict: `git merge --abort` and report `MERGE FAILED` with the file names.
   After resolving run `PM_HOSTSEL=all sh 3ds/tools/nd_host_test.sh` (one run, alone): it must print ND HOST TESTS PASS. Then `git commit`, `python 3ds/tools/bench.py base --adopt <ROW> --commit $(git rev-parse HEAD) --parent $(git rev-parse HEAD^1)`, `git worktree remove --force build3ds/wt/<ROW>`, `git branch -d <ROW>`, `sh 3ds/tools/postmerge_bg.sh --queue`.
5. Purge old queued post-merge jobs: keep only the newest file in build3ds/out/postmerge_q.
6. Counters: `python -I 3ds/tools/listcen.py | grep -E "total (build|data|gfxtype|reader|model)\b"`. Print the one-line result.
