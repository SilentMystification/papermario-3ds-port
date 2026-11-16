# ws_STALEOBJ (Haiku): the main build keeps objects of an older worktree source (STARTLR2 and MOON7 not in build 5877)
Designer to ask: the bug oracle (oracle 38, SendMessage; the orchestrator gives the id). Worktree build3ds/wt/STALEOBJ, branch STALEOBJ.
PM_ROW=STALEOBJ PM_LANE=STALEOBJ on every build and bench call. Edit only 3ds/build.sh (3 lines, see ws_STALEOBJ_code.md).
## Cause (FACT, code read and file times; read it, do not test it again)
- A build in a worktree without PM_LANE uses the shared volumes pm3ds-src and pm3ds-build (build.sh:18-19).
  FACT: build3ds/wt/MAPLAG/build3ds/src_volid.txt (01:52:29), wt/MOON7 (01:16:56), wt/STARTLR2 (01:10:33) exist.
- The sync keeps the host mtime of each copied file (build.sh:231 `tar -xf -`). Ninja rebuilds an object only when its source is newer.
- MAPLAG copies (mtime 01:24:11, before MOON7 and STARTLR2) were compiled at 01:53. Main files: pm_dbg_game.c and pm_moon.h 01:31:58,
  pm_input.c and pm_shoulder.h 01:33:10 (build3ds/src_manifest.txt). They are older than the objects, so build 5877 (02:52) did not compile them.
- FACT: 3ds/logs/builds/pm_3ds_5877_02899966ac.3dsx holds 'moon jump tap' (1) and not 're-entered JUMP, y' (0). The source has the reverse.
- These 4 files are the only stale ones (FACT: git diff 94911f5b38 02899966ac, main mtime before 01:52:29). FSKIP is in the build.
## Fix
Copied files get the time of the copy (`tar -xmf`). A new sync tag (prefix `m`) makes every old volume fail vol_match once, so the next build
of each folder copies all files and recompiles all (ccache serves unchanged content).
## Loop (1 build, 2 Azahar runs)
1. Make the 3 edits of ws_STALEOBJ_code.md. `sh 3ds/tools/nd_host_test.sh > build3ds/out/staleobj_host.txt 2>&1`. Pass: `ND HOST TESTS PASS`.
2. `PM_LANE=STALEOBJ PM_ROW=STALEOBJ sh 3ds/build.sh > build3ds/out/staleobj_b1.txt 2>&1`. Pass: rc 0, 0 compile errors,
   `build3ds/lane-STALEOBJ/src_volid.txt` starts with `m`, the line `STAGE sync-host ... (changed N, deleted 0)` has
   N = `wc -l < build3ds/lane-STALEOBJ/src_now.txt`.
3. The same command again, output to build3ds/out/staleobj_b2.txt. Pass: the line `--- build up to date`.
4. Fresh-code check: `grep -c -a -F 're-entered JUMP, y' build3ds/lane-STALEOBJ/pm_3ds.3dsx` gives 1 and
   `grep -c -a -F 'moon jump tap' build3ds/lane-STALEOBJ/pm_3ds.3dsx` gives 0.
   Seeded fault (self-test of the check): the same 2 greps on ../../../3ds/logs/builds/pm_3ds_5877_02899966ac.3dsx give 0 and 1.
5. Commit `STALEOBJ 1: build.sh sync stamps copied files with the copy time; sync tag m forces one full copy`.
6. Byte gate: `python 3ds/tools/bench.py base spot_mac01_drec`, `python 3ds/tools/bench.py run spot_mac01_drec --row STALEOBJ --why "STALEOBJ gate"`,
   `python 3ds/tools/bench.py cmp spot_mac01_drec --row STALEOBJ --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal. No counter (C1..C12) moves.
## Fix scope
Only the 3 lines of 3ds/build.sh. A tar error in step 2: send build3ds/out/staleobj_b1.txt to the oracle.
## Stop and hand-back
Stop: steps 1-6 pass. Final commit `STALEOBJ 2: gate` with the values. That commit deletes both ws_STALEOBJ pages and their INDEX.md line.
Hand-back: build3ds/out/handback_STALEOBJ.txt (line 1 commit hash, then the step 2 N, the step 4 counts, the seed counts, the cmp result).
Orchestrator after the merge: run `sh 3ds/build.sh` on main as usual. Pass: the sync line shows all files (full copy, one time) and
the main 3dsx greps give 1 and 0 (step 4). Then push. Rule: a worktree build without PM_LANE costs main one full recompile after this fix.
