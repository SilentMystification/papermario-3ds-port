# ws_CHCONV: chapter_change joins the converter, its 16 included lists move to the twin (moves C5 data, C6 data_arrays, C7 gfxtype, C10 recdl)
Model: Haiku. PM_ROW, PM_LANE = CHCONV. Branch CHCONV, worktree build3ds/wt/CHCONV. Budget: 1 build, 8 Azahar starts (base runs not charged). Parallel with ws_MESHST (no shared file).
Owns: 3ds/tools/gconv_src.d/CHCONV.txt (new), 3ds/tools/gset.d/CHCONV.txt (new), src/effects/chapter_change.c, src/effects/gfx/chapter_change.c and
3ds/tools/gfxin/src/effects/gfx/chapter_change.c (gset.py writes the last two). Edit no other file. Never edit by hand a file that gset.py rewrote.
User decision 2026-11-15: convert chapter_change.c:163. The picture changes: the converter table draws the card as 7 draws with 2 textures in place of 1 draw with 1 texture
(FXENT3 merge cac60bf486; the join was taken back in 728f13752 for this reason only). The user checks the card on the console after the merge.
FACT (oracle dry run, a scratch worktree of main 1828f683b0, build3ds/out/oracle_ch_h.txt): `ND HOST TESTS PASS`, `gconv lists 1763 ... bad 0 moved 1562`, `gconv gset ok 1660 of 1763 lists`;
all 17 chapter_change lists are in gset_ok.txt. gset.py: `gset moved 16 files 1 keep 0`, `gset ok`. D_0900C328_3F8188 stays a device array (gfx/chapter_change.c:62, no include).
FACT (same dry run, listcen deltas): data -104, data_arrays -16, gfxtype -16, recdl -1. C1 and C2 do not change.
FACT: fxr_drec draws chapter_change (fxtest 113, azt.d/FXENT3.json:68) from frame 1512 (drec=412:31:44, 26th effect). No other drec bench draws it (fxp_drec list has no 113).
## Group 1: join and move (0 builds, commit `CHCONV 1: ...`)
1. `python 3ds/tools/listcen.py > build3ds/out/chconv_lc0.txt`.
2. New 3ds/tools/gconv_src.d/CHCONV.txt, 1 line: `chapter_change`.
3. chapter_change.c:163. Before `            nd_rec_dl(it->displayList);` After `            nd_draw_list(it->displayList, ND_SELF);` (the form of aura.c:309).
4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/chconv_h1.txt 2>&1`: the 3 lines of the FACT above. build3ds/out/gset_ok.txt holds D_0900C540_3F83A0.
5. New 3ds/tools/gset.d/CHCONV.txt, 1 line: `src/effects/gfx/chapter_change.c`. `python 3ds/tools/gset.py 3ds/tools/gset.d/CHCONV.txt build3ds/out/gset_ok.txt > build3ds/out/chconv_set.txt 2>&1`:
   the 2 gset lines of the FACT. Rewrite example (chapter_change.c:16): Before `extern Gfx D_0900BE40_3F7CA0[];` After `NDN_DECL(D_0900BE40_3F7CA0);`.
   The `#if VERSION_JP` externs stay. `git status --short` lists only the 5 owned files.
6. `sh 3ds/tools/nd_host_test.sh > build3ds/out/chconv_h2.txt 2>&1`: PASS, `bad 0`, no `gconv: moved` line. Commit.
## Group 2: gate (1 build, charged runs, commit `CHCONV 2: ...`)
7. One build: rc 0, memcheck margin above 0 KB. For B in fxr_drec spot_mac01_drec sbk14_drec fxp_drec: `python 3ds/tools/bench.py base $B --row CHCONV --commit $(git merge-base HEAD main)`,
   `python 3ds/tools/bench.py run $B --row CHCONV --why "CHCONV 2"`, `python 3ds/tools/bench.py cmp $B --row CHCONV --commit $(git merge-base HEAD main)`.
8. Pass rule. spot_mac01_drec, sbk14_drec, fxp_drec: byte-equal. fxr_drec: the cmp line is `drec draw <seq> DIFF line <L>` and the first difference is in frame 1512 or later:
   `awk -v L=<L> 'NR<=L && /^frame /{f=$2} END{print "first diff frame", f}' <main folder>/build3ds/basecache/$(git rev-parse $(git merge-base HEAD main))/fxr_drec/drec.txt` prints a value of 1512 or more.
   A first difference before frame 1512 fails the gate. fxr_drec DIFF only in a `t 0 type .. dim 01000200` line before 1512: run it once more (cold-start rule, oracle FACT pm_gpu.c:2565, :666).
9. `python 3ds/tools/listcen.py > build3ds/out/chconv_lc1.txt`: the deltas of the FACT. Delete this page. Commit.
Revert recipe (when the user finds the card wrong): `git revert -m 1 <the Merge CHCONV commit>` on main, one commit `Revert CHCONV: user picture`. It restores chapter_change.c, gfx/chapter_change.c,
deletes the twin and both CHCONV.txt files. Then nd_host PASS and one build. No bench (the revert is the base).
Fix rights: step 4 or 6 output differs from the FACT, or a build error: `git checkout -- src 3ds/tools/gfxin`, `git clean -fdq -- 3ds/tools/gfxin 3ds/tools/gset.d 3ds/tools/gconv_src.d`,
send the output path to the oracle (CLAUDE.md 5). Any other cmp DIFF: 2 tries per named cause, then the stop rule. Never change a base.
Stop: group 2 gated and committed. Hand-back build3ds/out/handback_CHCONV.txt, first 8 lines: commit hashes, gconv lists and gset ok, the gset lines, the 4 cmp results,
the fxr_drec first diff frame, C5 C6 C7 C10 (lc0, lc1), memcheck margin, Azahar starts used.
