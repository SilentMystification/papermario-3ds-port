# ws_FXTYPE (Haiku): effect list handle arrays get the type NdLst (C7)
Start: after Merge FXVROM (FACT: both edit src/effects/lil_oink.c). Branch from main after that merge. Worktree build3ds/wt/FXTYPE, branch FXTYPE.
Objective: C7 gfxtype -89. FACT (oracle dry run of block A on main 1e45bdcad9): `fxtype arrays ok 89 other 50 files 48`. Each changed line
holds one `Gfx` token. The array values do not change (same pointers): the draws do not change. PM_ROW=FXTYPE PM_LANE=FXTYPE on every bench call.
Files: 3ds/include/nd_gtab.h (1 line), 3ds/tools/fxtype.py (new), 3ds/tests/fxtype/a.c (new), 3ds/tests/host.d/fxtype.sh (new), and the
src/effects/*.c files that fxtype.py changes. Do not edit gset.py, nd_gfx_conv.c, nd_lkey.h, lkey_cen.py or any other file.
## Edit 1: nd_gtab.h
Before (nd_gtab.h:65-66): `#define NDG(sym) (&ndg_##sym)` then `#ifdef ND_GCONV_IN /* GSETB: ...`.
After: insert one line between them:
`typedef const void* NdLst; /* FXTYPE: a list handle (NDN(x) or a list name); nd_draw_list, nd_rec_dl and gSPDisplayList take it */`
## Edit 2: blocks A (fxtype.py), B (fixture a.c, LF) and C (fxtype.sh) of docs/handoff/ws_FXTYPE_code.md, exact text.
## Edit 3: `python 3ds/tools/fxtype.py src/effects/*.c > build3ds/out/fxtype_run.txt`
Example before (src/effects/aura.c:21): `Gfx* D_E0076E90[] = {`. After: `NdLst D_E0076E90[] = {`.
Its use stays (aura.c:356): `nd_draw_list(D_E0076E90[type], pe_aura_354);`.
Example before (src/effects/lil_oink.c:29): `Gfx* lil_oink_FramesGfx[][3] = {`. After: `NdLst lil_oink_FramesGfx[][3] = {`.
An array with another use stays `Gfx*` (example: src/effects/big_snowflakes.c D_E0060730, read by a local `Gfx* dlist2 = D_E0060730[i & 1];`).
## Loop (1 build, at most 8 Azahar starts; base runs not charged)
1. Edit 1 and 2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/fxtype_host1.txt 2>&1`. Pass: `ND HOST TESTS PASS` and `fxtype test ok`. Commit `FXTYPE 1`.
2. `python 3ds/tools/fxtype.py --dry src/effects/*.c | tail -1`. Expected: `fxtype arrays ok 89 other 50 files 48` (FACT on 1e45bdcad9).
   Another number: write it in the commit text and go on (the tool rule decides, not the number).
3. Edit 3. `git diff --stat` shows only src/effects/*.c files (the `files` count of step 2). Host test again (> build3ds/out/fxtype_host2.txt): PASS.
4. One build. A compile error in one effect file: `git checkout -- <file>` for that file only, note it in the commit. memcheck margin above 0
   (GUESS: no change, a type does not change the data size). `python 3ds/tools/listcen.py`: gfxtype drops by the ok count. Commit `FXTYPE 2` with the totals.
5. Gate. For each of spot_mac01_drec, sbk14_drec, fxr_drec, fxp_drec: `python 3ds/tools/bench.py base <b>`, then
   `python 3ds/tools/bench.py run <b> --row FXTYPE --why "FXTYPE gate"`, then `python 3ds/tools/bench.py cmp <b> --row FXTYPE --commit $(git merge-base HEAD main)`.
   Pass: 1/1 byte-equal each (one differing bit fails). A cold first fxr_drec cmp: one warm rerun (bench.py rule).
## Fix scope
fxtype.py, fxtype.sh, the fixture, the nd_gtab.h line. Do not edit an effect file by hand. Do not widen the use rule (OKUSE) of fxtype.py.
## Stop and hand-back
Stop: all 4 gates pass. Final commit `FXTYPE 3: gate` with the values. That commit deletes this page and ws_FXTYPE_code.md.
Hand-back: build3ds/out/handback_FXTYPE.txt (line 1 commit hash, then gate values, C7 change, memcheck margin).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle (SendMessage). Then stop.
