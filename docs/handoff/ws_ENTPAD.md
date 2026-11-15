# ws_ENTPAD: the Padlock and UnusedBlock entity models join the converter and their lists move to the gfxin twin (moves C5 data, C6 data_arrays, C7 gfxtype)
Model: Haiku. PM_ROW, PM_LANE = ENTPAD. Branch ENTPAD, worktree build3ds/wt/ENTPAD. Budget: 1 build, 2 Azahar runs. Start now (main f2d6190604).
Owns: 3ds/tools/gconv_src.d/ENTPAD.txt (new), 3ds/tools/gset.d/ENTPAD.txt (new), src/entity/model/PadlockBlueFace.c, PadlockRedFace.c, PadlockRedFrame.c, UnusedBlock.c,
src/entity/default/Padlock.c, and the 4 new twins 3ds/tools/gfxin/src/entity/model/<same names>. gset.py writes the twins and the src files. Edit no other file. Never edit by hand a file that gset.py rewrote.
FACT (listcen, main f2d6190604): C5 data 12640, C6 data_arrays 1083, C7 gfxtype 1752.
FACT (oracle dry run on a copy of main f2d6190604, steps 1 to 3): the converter makes a v2 table for all 44 lists of the 4 files (10 + 10 + 9 + 15), gset_ok has all 44.
`gset moved 44 files 1 keep 0`, `gset ok`. listcen after: C5 12182 (-458), C6 1039 (-44), C7 1702 (-50). Converter after the move: `bad 0 moved 1000`, no `gconv: moved` line.
Not in this task: UltraBlock.c and UltraBlockContent.c (13 of 15 lists inherit state: ISEED2), SaveBlock.c (`#if VERSION_PAL` lists), titlemenu.c (its fr and es images are not in ver/us/build).
## Group 1: the converter set (0 builds, 0 runs, commit `ENTPAD 1: ...`)
1. New file 3ds/tools/gconv_src.d/ENTPAD.txt (LF), 4 lines: `src/entity/model/PadlockBlueFace.c` / `src/entity/model/PadlockRedFace.c` / `src/entity/model/PadlockRedFrame.c` /
   `src/entity/model/UnusedBlock.c`. Form: the same as 3ds/tools/gconv_src.d/LIT1.txt (one path per line).
2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/entpad_h1.txt 2>&1`. It must print `ND HOST TESTS PASS`. Then `grep -c -x -f <(grep -o '^Gfx [A-Za-z0-9_]*' src/entity/model/Padlock*.c src/entity/model/UnusedBlock.c | sed 's/.*Gfx //') build3ds/out/gset_ok.txt`
   must print 44. Commit.
## Group 2: the move and the gate (1 build, 2 runs, commit `ENTPAD 2: ...`)
3. `python 3ds/tools/listcen.py > build3ds/out/entpad_lc0.txt`. New file 3ds/tools/gset.d/ENTPAD.txt with the same 4 lines as step 1.
   `python 3ds/tools/gset.py 3ds/tools/gset.d/ENTPAD.txt build3ds/out/gset_ok.txt > build3ds/out/entpad_set.txt 2>&1`. The file must end with `gset moved 44 files 1 keep 0` and `gset ok`.
   Rewrite example (Padlock.c:12). Before `extern Gfx Entity_PadlockRedFrame_Render[];` After `NDN_DECL(Entity_PadlockRedFrame_Render);`.
   Rewrite example (Padlock.c:210). Before `    data->shackleGfx = Entity_PadlockRedFrame_RenderShackle;` After `    data->shackleGfx = NDN(Entity_PadlockRedFrame_RenderShackle);`.
   Device file example (UnusedBlock.c:2): gset adds `#include "nd_gtab.h"`. Each `Gfx X[] = { ... };` block goes to the twin, the device file keeps `NDN_DECL(X);`.
   `git status --short` lists only the 5 src files, the 4 twins and the 2 txt files.
4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/entpad_h2.txt 2>&1`: `ND HOST TESTS PASS`, `bad 0`, no `gconv: moved` line. One build: rc 0, `memcheck ... margin` above 0 KB.
5. `python 3ds/tools/bench.py base spot_mac01_drec --row ENTPAD --commit $(git merge-base HEAD main)`, `python 3ds/tools/bench.py run spot_mac01_drec --row ENTPAD --why "ENTPAD 2"`,
   `python 3ds/tools/bench.py cmp spot_mac01_drec --row ENTPAD --commit $(git merge-base HEAD main)`: byte-equal. GUESS: no drec bench shows a padlock. The host table compare of step 4 (nd_gtab_test) is the proof for the 44 lists.
6. `python 3ds/tools/listcen.py > build3ds/out/entpad_lc1.txt`. Delete this page and its INDEX.md line. Commit.
Fix rights: a host FAIL in step 2 or a count other than 44: `git checkout -- 3ds/tools/gconv_src.d`, send build3ds/out/entpad_h1.txt to the oracle (CLAUDE.md 5).
Step 3 output differs from the FACT, a `gconv: moved` line or a build error: `git checkout -- src 3ds/tools/gset.d`, `git clean -fdq -- 3ds/tools/gfxin 3ds/tools/gset.d`, send the output path to the oracle.
A cmp difference: 2 tries per named cause, then the stop rule (CLAUDE.md 5).
Stop: group 2 committed and gated. Hand-back build3ds/out/handback_ENTPAD.txt, first 8 lines: commit hashes, the count of step 2, the `gset moved` line, the cmp result,
C5, C6, C7 before and after (lc0, lc1), memcheck margin, Azahar runs used.
