# ws_CURT (Haiku): curtains.c loses 5 `&`, gset moves 11 lists to the twin, 1 list site to nd_draw_list (C5 -102, C6 -11, C1 -1, C2 -1)
Designer: the oracle (SendMessage). Worktree build3ds/wt/CURT, branch CURT. PM_ROW=CURT PM_LANE=CURT. Start now. No shared file with DLSITE2, MODELRM.
## Objective and counters
- FACT (code read, gset.py:14): a line with `&X` holds X. src/curtains.c:257, :276, :301, :302, :304 call `gSPDisplayList(gMainGfxPos++, &X)`.
  `&X` and `X` give the same address (X is an array). HOLD1 used this edit (commit 845f3005b1).
- FACT (build3ds/out/gset_ok.txt, main 4c02b18543): 11 of the 13 lists of src/curtains.c are gset ok. TheaterInitGfx and NoControllerGfx stay
  (ntab_unsup.txt). src/curtains.c is a converter source (3ds/tools/gconv_src.d/GSETR.txt:2). gen_gfx_tab.sh:15 reads the twin.
- FACT (oracle scratch run of the patch, gset.py and codemod.py on main 4c02b18543): `gset moved 11 files 1 keep 0`, `gset ok`.
  codemod --ntab rewrites 1 site (NoControllerSetupTexGfx is in ntab_tables.txt; TheaterGfx is an inherit list and stays).
  listcen of src/curtains.c: build 5 -> 4, gfxpos 5 -> 4, data 121 -> 19, data_arrays 13 -> 2, gfxtype 13 -> 2. recdl 0.
## Files. Edit only these 5.
In docs/handoff/ws_CURT.patch: src/curtains.c (5 lines), 3ds/tools/gset.d/CURT.txt, 3ds/tools/azt.d/CURT.json, 3ds/tools/bench.d/CURT.json.
Written by gset.py: src/curtains.c, 3ds/tools/gfxin/src/curtains.c (new). Written by codemod.py: src/curtains.c.
## Edit shapes
Before (src/curtains.c:276): `gSPDisplayList(gMainGfxPos++, &TheaterGfx);`. After the patch: `gSPDisplayList(gMainGfxPos++, TheaterGfx);`.
After gset: `gSPDisplayList(gMainGfxPos++, NDN(TheaterGfx));` and `NDN_DECL(TheaterGfx);` in place of the block (same form as src/screen_render_util.c:157).
After codemod (src/curtains.c, NoControllerSetupTexGfx line): `nd_draw_list(NDN(NoControllerSetupTexGfx), ND_SELF);` (same form as src/i_spy.c:51).
## Loop (1 build, 2 Azahar starts; base runs not charged)
1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/curt_host0.txt 2>&1` (writes gset_ok.txt, ntab_tables.txt). `python 3ds/tools/listcen.py > build3ds/out/curt_lc0.txt`.
2. `tr -d '\r' < docs/handoff/ws_CURT.patch | git apply -`. A reject: stop, send the output to the oracle.
3. `python 3ds/tools/gset.py 3ds/tools/gset.d/CURT.txt build3ds/out/gset_ok.txt > build3ds/out/curt_gset.txt`. Pass: `gset moved 11 files 1 keep 0`, `gset ok`.
4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/curt_host1.txt 2>&1`. Pass: `ND HOST TESTS PASS`, `lkey census ok`, `gconv lists` line with `bad 0`,
   no line `gconv: moved`. NoControllerSetupTexGfx is a line of build3ds/out/ntab_tables.txt.
5. `python 3ds/tools/codemod.py --ntab build3ds/out/ntab_tables.txt src/curtains.c`. Pass: `git diff src/curtains.c` has 1 new `nd_draw_list` line.
6. `python 3ds/tools/listcen.py > build3ds/out/curt_lc1.txt`. Pass vs curt_lc0.txt: build -1, gfxpos -1, data -102, data_arrays -11, gfxtype -11,
   no other total changes. Commit the 5 files: `CURT 1: curtains lists move to the twin (5 & removed, gset CURT, 1 site)`.
7. One build. Pass: rc 0, 0 compile errors, memcheck margin above 0.
8. `python 3ds/tools/bench.py base curt_title_drec`. Then `python 3ds/tools/bench.py run curt_title_drec --row CURT --why "CURT gate"` and
   `python 3ds/tools/bench.py cmp curt_title_drec --row CURT --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal.
   Coverage: the title calls set_curtain_scale(1.0f) each frame (FACT, src/state_title_screen.c:250), so render_curtains draws TheaterGfx
   (src/curtains.c:251-277). GUESS: the title shows in frames 360-540 after titleret=240. The no-controller lists do not draw (a pad is present).
## Fail
- A failing test or a compile error: you may edit only the 5 files. 2 tries for one cause. Then write the output to build3ds/out/ and send the path.
- A base run that fails, or a byte diff: stop. Send the output path to the oracle.
## Stop
The gate passes. The last commit `CURT 2: gate` deletes ws_CURT.md and ws_CURT.patch.
## Hand-back (one SubagentHandback message)
Commits. gset line. listcen build, gfxpos, data, data_arrays, gfxtype before and after. memcheck margin. The cmp line.
