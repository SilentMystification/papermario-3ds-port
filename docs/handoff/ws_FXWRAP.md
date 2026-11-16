# ws_FXWRAP (Haiku): the fire_breath and peach_star_beam wrapper lists move to their twins (C5 data -27, C6 data_arrays -6, gfxtype -7)
Designer: the oracle (SendMessage). Worktree build3ds/wt/FXWRAP, branch FXWRAP. PM_ROW=FXWRAP PM_LANE=FXWRAP. Start now.
No shared file with MODELRM, CURT, DLSITE2, GMERLEE.
## Objective and counters
- FACT (FXREN, merged fbd0d0dda2): these 6 lists are gset ok: D_09000B88_373F68, D_09000B88_373F68_body, D_09000BA8_373F88,
  D_09005090_3EAFC0, D_09005090_3EAFC0_body, D_090050F0_3EB020. gset.py does not move them: the wrappers are src blocks
  (src/effects/gfx/fire_breath.c:9-17, peach_star_beam.c:57-65; include mode moves only includes).
- FACT (oracle listcen run, main fbd0d0dda2 with the patch): data 2044 -> 2017 (-27), data_arrays 193 -> 187 (-6),
  gfxtype 805 -> 798 (-7). build, gfxpos, recdl, reader, model: 0. Bench difference: none (0 differing bits).
## Files (all in docs/handoff/ws_FXWRAP.patch). Edit only these 6.
src/effects/gfx/fire_breath.c, src/effects/gfx/peach_star_beam.c, src/effects/fire_breath.c, src/effects/peach_star_beam.c,
3ds/tools/gfxin/src/effects/gfx/fire_breath.c, 3ds/tools/gfxin/src/effects/gfx/peach_star_beam.c.
## Edit shape (the patch does it; use it to check a reject)
The 9 lines `extern Gfx <next>[];` .. `#include "<next>.gfx.inc.c"` (the #define, the body include, the #undef, the wrapper)
go from the device gfx file to the end of its twin, in the same order.
Before (src/effects/peach_star_beam.c:14, :37, :184): `extern Gfx D_09005090_3EAFC0[];`, `    D_090050F0_3EB020, NDN(..`,
`nd_draw_list(D_09005090_3EAFC0, pe_peach_star_beam_179);`.
After: `NDN_DECL(D_09005090_3EAFC0);`, `    NDN(D_090050F0_3EB020), NDN(..`, `nd_draw_list(NDN(D_09005090_3EAFC0), pe_peach_star_beam_179);`.
Same shape: peach_star_beam.c:15, :229, fire_breath.c:10, :14 (3 table entries).
## Loop (1 build, 2 Azahar starts; base runs not charged)
1. `python 3ds/tools/listcen.py > build3ds/out/fxwrap_lc0.txt`.
2. `tr -d '\r' < docs/handoff/ws_FXWRAP.patch | git apply -`. A reject: stop, send the output to the oracle.
3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/fxwrap_host.txt 2>&1`. Read the last 20 lines.
   Pass: `ND HOST TESTS PASS`, `lkey census ok`, `gconv lists` line with `bad 0`, no line `gconv: moved`.
4. `python 3ds/tools/listcen.py > build3ds/out/fxwrap_lc1.txt`. Pass vs fxwrap_lc0.txt: total data -27, data_arrays -6,
   gfxtype -7, no other total line changes. Commit the 6 files: `FXWRAP 1: fire_breath and peach_star_beam wrappers move to the twin`.
5. One build. Pass: rc 0, 0 compile errors, memcheck margin above 0.
6. `python 3ds/tools/bench.py base fxr_drec` and `python 3ds/tools/bench.py base fxp_drec`.
7. Gate, for b in fxr_drec fxp_drec: `python 3ds/tools/bench.py run $b --row FXWRAP --why "FXWRAP gate"`, then
   `python 3ds/tools/bench.py cmp $b --row FXWRAP --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each.
   Coverage (FACT, code read): fxr_drec has fxtest 49 fire_breath (frame 896), fxp_drec has fxtest 112 peach_star_beam
   (3ds/src/pm_fxtest.c:82, :145). fire_breath.c:224 and peach_star_beam.c:184, :229 draw the lists every frame.
## Fail
- A failing test or a compile error: you may edit only the 6 files. Compare each with the patch. 2 tries for one cause.
  Then write the output to build3ds/out/ and send the path to the oracle.
- A byte diff in a gate: stop. Send the cmp output path to the oracle.
## Stop
Both gates pass. The last commit `FXWRAP 2: gate` deletes ws_FXWRAP.md and ws_FXWRAP.patch.
## Hand-back (one SubagentHandback message)
Commits. listcen data, data_arrays, gfxtype before and after. memcheck margin. The 2 cmp lines.
