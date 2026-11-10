# Row BUILDCLEAN: cleanup in the life cycle (oracle batch 4, Haiku)

(`sh 3ds/tools/wt_new.sh BUILDCLEAN`. PM_ROW=PM_LANE=BUILDCLEAN. CLAUDE.md rules 1-7. No Azahar start. Delete this row file in the completing commit.)
## Touch only
3ds/tools/wt_gc.sh, 3ds/tools/bench.py (base_build_run), 3ds/tools/wt_new.sh (one mkdir).
## Edit 1, wt_gc.sh: new sweep after the lane loop (skipped with --only), same `run` wrapper, dry run prints
- Directories in build3ds/ matching `clean* wt_commit* wt_base* cen_src* wt_ui* ftp_* rvtmp final*_png jfix review nat` with no file newer than 12 h (`find -mmin -720`): `rm -rf`.
- Loose files in build3ds/ and build3ds/wt/ matching `*.txt *.log *.py *.png` whose name starts with `<NAME>_`, where NAME is not a folder in build3ds/wt/, and that are older than 12 h: rm. Never touch build_key.txt, hw_live_log_*.txt, crash_stack_addrs.txt, pm_3ds.*.
- Print the MB freed (du -sm before and after).
## Edit 2, wt_gc.sh: finished worktrees
- `.wt_keep` no longer keeps a worktree when its branch is merged into main and docs/handoff/row_<name>.md does not exist on main (the row is done): remove it as a merged one.
- Unmerged branches stay, but are printed with their du -sm size.
## Edit 3, wt_new.sh
`mkdir -p build3ds/wt/<row>/build3ds/out`. Agents write all tool output there: it goes away with the worktree.
## Edit 4, bench.py base_build_run: a lock per commit
- Before scratch_build: `lk = store_root() / (commit + ".lock")`.
- Take the lock with `os.open(lk, os.O_CREAT | os.O_EXCL | os.O_WRONLY)` and write the pid and the time.
- On FileExistsError: if the lock is older than 2 h, delete it and retry once. Otherwise wait in a loop (time.sleep(10), at most 40 min).
- When the lock is free, check `stored(commit, bd["name"])` again: if stored, print "base stored by another run" and return 0.
- Remove the lock in the `finally`.
- Reason: both builds use lane base_<short>, so they share the docker volumes pm3ds-src/build-base_<short> (tar "Cannot open", "File exists").
## Gate
`sh 3ds/tools/wt_gc.sh` dry run lists the stale kinds now in build3ds (AUDNAP_*, HC4_*, ftp_170*) and no live worktree.
`python -c` test: two threads call the lock helper for one commit; the second waits until the first releases.
## Orchestrator, with the user's yes
CLAUDE.md rule 3: "write tool output to files in build3ds/out/ of your worktree".
