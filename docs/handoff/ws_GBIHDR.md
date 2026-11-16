# ws_GBIHDR (Haiku): gbiprune.py deletes the 132 gbi macros that no file uses (C1 build, C3 data, C5 gfxtype)
Designer to ask: oracle DL3 (SendMessage; the orchestrator gives the id). Worktree build3ds/wt/GBIHDR, branch GBIHDR. PM_ROW=GBIHDR PM_LANE=GBIHDR.
Start now. Parallel with GSETR, CGFX, DLSITE1, MODELRM, HOLD1, PRA31 (no shared file). Code of the new files: ws_GBIHDR_code.md.
Objective (FACT, oracle dry run on main a0115f743c, the tool of ws_GBIHDR_code.md): `gbiprune dead 132 lines 778`
(gbi.h 115 blocks 724 lines, gs2dex.h 32 blocks 42 lines, gt.h 3 blocks 12 lines, gbi_custom.h 0). listcen: build 722 -> 566 (-156),
data 3016 -> 2823 (-193), gfxtype 885 -> 853 (-32). data_arrays, gfxpos, recdl: no change. A later main can give larger numbers: write the real ones.
Files: new 3ds/tools/gbiprune.py, new 3ds/tests/host.d/gbiprune.sh, new 3ds/tests/gbiprune/case/include/PR/gbi.h, new 3ds/tests/gbiprune/case/src/user.c,
and the headers the tool writes (include/PR/gbi.h, include/PR/gs2dex.h, include/PR/gt.h). Do not edit a header by hand. Do not edit any other file.
Do not run gen_nolist.py: 3ds/include/nd_nolist.h keeps the poison lines of the deleted names (an #undef of an unknown name is legal C).
## Edit shape (the tool writes it)
Before (include/PR/gbi.h:2982): `#define	gDPSetBlendMask(pkt, mask)      gDPNoOp(pkt)`. After: the line is gone.
A block with backslash lines goes as a whole. Lines outside a dead block do not change.
## Loop (1 build, at most 2 Azahar starts; base runs not charged)
1. Write the 4 new files from ws_GBIHDR_code.md (LF line ends). `sh 3ds/tools/nd_host_test.sh > build3ds/out/gbi_host0.txt 2>&1`.
   Pass: `ND HOST TESTS PASS` and the line `gbiprune ok seed ok`. Commit `GBIHDR 1: gbiprune.py and host test`.
2. `python 3ds/tools/listcen.py > build3ds/out/gbi_lc0.txt` (the base).
3. `python 3ds/tools/gbiprune.py --dry > build3ds/out/gbi_dry.txt`. Pass: `gbiprune dead 132 lines 778` on a branch from a0115f743c.
   A branch from a later main: dead 132 or more. A smaller number: stop and send the path to oracle DL3.
4. `python 3ds/tools/gbiprune.py > build3ds/out/gbi_run.txt`. `git diff --stat`: only the 3 headers. `git diff | grep '^+[^+]'` prints nothing.
5. Host test again (> build3ds/out/gbi_host1.txt). Pass: `ND HOST TESTS PASS`, `lkey census ok`, conv2c line `bad 0`.
   Run `python 3ds/tools/gbiprune.py --dry` again. Pass: `gbiprune dead 0 lines 0`. Commit `GBIHDR 2: delete 132 unused gbi macros`.
6. One build. Pass: rc 0, 0 compile errors, memcheck margin above 0. `python 3ds/tools/listcen.py > build3ds/out/gbi_lc1.txt`.
   Pass vs gbi_lc0.txt: build -156, data -193, gfxtype -32 (or the change that step 3 predicts on a later main), other classes the same.
7. Gate: `bench.py base spot_mac01_drec`, `bench.py run spot_mac01_drec --row GBIHDR --why "GBIHDR gate"`,
   `bench.py cmp spot_mac01_drec --row GBIHDR --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal.
## Fix scope
A compile error `implicit declaration of function 'gXXX'` or `'gsXXX' undeclared` names a macro: a user file outside the census uses it.
Add that file path (repo-relative) to USER_EXTRA in gbiprune.py, run `git checkout HEAD -- include/PR`, then steps 4-5 again. Write the file in the commit.
## Stop and hand-back
Stop: the gate passes. Final commit `GBIHDR 3: gate` with the values deletes ws_GBIHDR.md and ws_GBIHDR_code.md. Hand-back:
build3ds/out/handback_GBIHDR.txt (line 1 commit hash, then gate value, gbiprune dead and lines, listcen build data gfxtype, memcheck).
Merge note for the orchestrator: a branch merged later that uses a deleted macro fails to compile. Fix: restore that block from git history.
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to oracle DL3 (SendMessage). Then stop.
