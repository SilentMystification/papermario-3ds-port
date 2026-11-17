# ws_FXLT (Haiku): 2 v2 ops (NDN_TSZ, NDN_LIT); 48 effect lists move (C5 data -410, C6 data_arrays -48, C7 gfxtype -19)
Designer: the oracle of this page (SendMessage; the orchestrator gives the id). Worktree build3ds/wt/FXLT, branch FXLT. PM_ROW=PM_LANE=FXLT.
Start after Merge FXOPS (patch made on main fb734dc67e + ws_FXOPS.patch). Model: Haiku: the patch holds all code, the oracle ran it.
## Objective and counters (FACT: oracle dry run, worktree ORAFX2 = main fb734dc67e + ws_FXOPS.patch + this patch)
- nd_gfx_conv.c:379, :395 refuse a tile size before any SetTImg ("texture image outside the set"). The patch emits NDN_TSZ there.
- nd_gfx_conv.c:554 refuses a lit draw after a light unit. The patch emits NDN_LIT at each light unit and drops the refusal.
- nd_gtab_test.c replays both ops in the colour stream (model m_unit, table v_replay). New faults --fault-tsz, --fault-lit.
- gset.py moved 48 lists in 4 files, keep 0 (quizmo_stage 37, shape_spell 6, spirit_card 4, fright_jar 1).
- listcen totals after FXOPS -> after FXLT: data 1126 -> 716, data_arrays 85 -> 37, gfxtype 573 -> 554. Other totals: no change.
- Host run: `ND HOST TESTS PASS`, `fxlt ok`, `ntab tables 1965 draws 36655 bad 0`, `ntab moved 1954`, `lkey census ok`, gset ok 1917.
## Files (all in docs/handoff/ws_FXLT.patch). Edit only these 19.
3ds/include/nd_gtab.h, 3ds/tools/nd_gfx_conv.c, 3ds/tools/nd_gtab_test.c, 3ds/tests/host.d/fxlt.sh (new),
3ds/tools/gset.d/FXLT.txt (new), 3ds/tools/azt.d/FXLT.json (new), 3ds/tools/bench.d/FXLT.json (new),
src/effects/gfx/{fright_jar,quizmo_stage,shape_spell,spirit_card}.c, 3ds/tools/gfxin/src/effects/gfx/ (the same 4 names),
src/effects/{quizmo_stage,shape_spell,spirit_card,something_rotating}.c.
## Edit shape (the patch does it; use it to check a reject)
Before (src/effects/spirit_card.c:17, :21): `extern Gfx D_09004458_3FE908[];` and `NdLst D_E0112630[] = { D_09004458_3FE908, ...`.
After: `NDN_DECL(D_09004458_3FE908);` and `NdLst D_E0112630[] = { NDN(D_09004458_3FE908), ...` (same form as spirit_card.c:18).
Before (src/effects/gfx/shape_spell.c): `#include "effects/gfx/D_09001128_33B088.gfx.inc.c"`. After: only the twin (gfxin) has it.
## Loop (1 build, 2 Azahar starts; base runs not charged)
1. `python 3ds/tools/listcen.py > build3ds/out/fxlt_lc0.txt`.
2. `tr -d '\r' < docs/handoff/ws_FXLT.patch | git apply -`. A reject: stop, send the output to the oracle.
3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/fxlt_host.txt 2>&1`. Read the last 20 lines. Pass: `ND HOST TESTS PASS`,
   `fxlt ok`, `lkey census ok`, `gconv lists` with `bad 0`, no line `gconv: moved`.
4. `python 3ds/tools/listcen.py > build3ds/out/fxlt_lc1.txt`. Pass vs fxlt_lc0.txt: data -410, data_arrays -48, gfxtype -19,
   no other total line changes. Commit the 19 files: `FXLT 1: NDN_TSZ and NDN_LIT ops; 48 effect lists move`.
5. One build. Pass: rc 0, 0 compile errors, memcheck margin above 0.
6. `python 3ds/tools/bench.py base fxlt_drec`.
7. Gate: `python 3ds/tools/bench.py run fxlt_drec --row FXLT --why "FXLT gate"`. When it prints warm 0 (cold), run it 1 more time.
   Then `python 3ds/tools/bench.py cmp fxlt_drec --row FXLT --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal.
   Coverage (FACT, code read): pm_fxtest.c index 13 shape_spell, 72 quizmo_stage, 91 fright_jar, 115 spirit_card,
   117 something_rotating. Slots of 44 frames from frame 400 (pm_fxtest.c:205-206). drec=412:5:44 records frame 12 of each slot.
## Fail
- A failing test or a compile error: edit only the 19 files. Compare each with the patch. 2 tries for one cause.
  Then write the output to build3ds/out/ and send the path to the oracle.
- The base run fails or hangs, or a byte diff in the gate: stop. Send the output path to the oracle.
## Stop
The gate passes. The last commit `FXLT 2: gate` deletes ws_FXLT.md and ws_FXLT.patch.
## Hand-back (one SubagentHandback message)
Commits. listcen data, data_arrays, gfxtype before and after. memcheck margin. The cmp line.
