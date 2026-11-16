# ws_FXHOLD (Haiku): snowfall D_09000C00/C10 and chapter_change D_0900C328 move to their twins (C5 -24, C6 -3, gfxtype -4)
Designer: the oracle (SendMessage). Worktree build3ds/wt/FXHOLD, branch FXHOLD. PM_ROW=FXHOLD PM_LANE=FXHOLD. Start now.
No shared file with CGFX, DLSITE1, AXNS, LCNOBUILD, FXREN.
## Objective and counters
- FACT (build3ds/out/gset_ok.txt, main): D_09000C00_38DC70, D_09000C10_38DC80 and D_0900C328_3F8188 are gset ok.
- FACT (code read): gset.py does not move them. src/effects/gfx/snowfall.c:16 `gsSPBranchList(D_09000C10_38DC80)` holds the include
  (:10). The block :13-17 and the block src/effects/gfx/chapter_change.c:50-54 are src blocks (include mode moves only includes).
- FACT (oracle listcen run, main 401b0fcd10 with the patch): data -24, data_arrays -3, (main cf4298551f starts at 2219, 209)
  gfxtype -4. build, gfxpos, recdl, reader, model: 0. Bench difference: none (0 differing bits).
## Files (all in docs/handoff/ws_FXHOLD.patch). Edit only these 6.
src/effects/gfx/snowfall.c, src/effects/gfx/chapter_change.c, src/effects/snowfall.c, src/effects/chapter_change.c,
3ds/tools/gfxin/src/effects/gfx/snowfall.c, 3ds/tools/gfxin/src/effects/gfx/chapter_change.c.
## Edit shape (the patch does it; use it to check a reject)
Before (src/effects/snowfall.c:13, :200): `extern Gfx D_09000C00_38DC70[];` and `nd_draw_list(D_09000C00_38DC70, ND_SELF);`.
After: `NDN_DECL(D_09000C00_38DC70);` and `nd_draw_list(NDN(D_09000C00_38DC70), ND_SELF);` (same form as src/effects/fire_breath.c:205).
Before (src/effects/chapter_change.c:29, :66): `extern Gfx D_0900C328_3F8188[];` and `{ D_0900C328_3F8188,   0, 20, 104, 20 },`.
After: `NDN_DECL(D_0900C328_3F8188);` and `{ NDN(D_0900C328_3F8188),   0, 20, 104, 20 },` (same form as :65).
The 2 blocks and the snowfall include go to the end of their twin. The device gfx files lose them.
## Loop (1 build, 2 Azahar starts; base runs not charged)
1. `python 3ds/tools/listcen.py > build3ds/out/fxhold_lc0.txt`.
2. `tr -d '\r' < docs/handoff/ws_FXHOLD.patch | git apply -`. A reject: stop, send the output to the oracle.
3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/fxhold_host.txt 2>&1`. Read the last 20 lines.
   Pass: `ND HOST TESTS PASS`, `lkey census ok`, `gconv lists` line with `bad 0`, no line `gconv: moved`.
4. `python 3ds/tools/listcen.py > build3ds/out/fxhold_lc1.txt`. Pass vs fxhold_lc0.txt: total data -24, data_arrays -3, gfxtype -4,
   no other total line changes. Commit the 6 files: `FXHOLD 1: snowfall and chapter_change src blocks move to the twin`.
5. One build. Pass: rc 0, 0 compile errors, memcheck margin above 0.
6. `python 3ds/tools/bench.py base gold_63_drec` and `python 3ds/tools/bench.py base fxr_drec`.
7. Gate, for b in gold_63_drec fxr_drec: `python 3ds/tools/bench.py run $b --row FXHOLD --why "FXHOLD gate"`, then
   `python 3ds/tools/bench.py cmp $b --row FXHOLD --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each.
   Coverage (FACT, code read): gold_63_drec is fxtest=63 snowfall, frames 412-413. fxr_drec drec=412:31:44 has a frame in each of its
   31 slots. fxtest 113 is chapter_change type 0 (3ds/src/pm_fxtest.c:146). Type 0 draws D_E010E838 every frame
   (src/effects/chapter_change.c:248-249, :261). That table holds D_0900C328 (:66).
## Fail
- A failing test or a compile error: you may edit only the 6 files. Compare each with the patch. 2 tries for one cause.
  Then write the output to build3ds/out/ and send the path to the oracle.
- A byte diff in a gate: stop. Send the cmp output path to the oracle.
## Stop
Both gates pass. The last commit `FXHOLD 2: gate` deletes ws_FXHOLD.md and ws_FXHOLD.patch.
## Hand-back (one SubagentHandback message)
Commits. listcen data, data_arrays, gfxtype before and after. memcheck margin. The 2 cmp lines.
