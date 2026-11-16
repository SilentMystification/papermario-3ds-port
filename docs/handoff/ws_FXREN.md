# ws_FXREN (Haiku): gen_gfx_syms.py records the X_body list of fire_breath and peach_star_beam (gap c)
Designer: the oracle (SendMessage). Worktree build3ds/wt/FXREN, branch FXREN. PM_ROW=FXREN PM_LANE=FXREN.
Start after Merge AXNS (shared 3ds/tools/gen_gfx_syms.py). Create the worktree from main after that merge.
## Objective and counters
- FACT (code read): src/effects/gfx/fire_breath.c:10-16 and peach_star_beam.c:58-64 rename the asset list with `#define X X_body`
  before its include and wrap it. scan() of gen_gfx_syms.py does not apply the rename, so X_body has no symbol.
- FACT (oracle dry run, main 401b0fcd10, 241 sources): symbols 4323 -> 4325. Only new: D_09000B88_373F68_body, D_09005090_3EAFC0_body.
- FACT (oracle host run of the patch; before: build3ds/out/main_build_final.txt): gconv tables +4, inherit -1, unsup -1, itab -1,
  gset ok +6, lists +2, moved 0. The 6 new gset ok lists: D_09000B88_373F68, D_09000B88_373F68_body, D_09000BA8_373F88,
  D_09005090_3EAFC0, D_09005090_3EAFC0_body, D_090050F0_3EB020. The move of these lists is a later page.
- FACT: listcen change 0 (listcen.py does not read the converter output). Bench difference: none (0 differing bits).
## Files (all in docs/handoff/ws_FXREN.patch). Edit only these 2.
3ds/tools/gen_gfx_syms.py (REN, REN_RULE, renames_at, scan). 3ds/tests/host.d/gdefren.sh (new; seeded fault REN_RULE = False).
## Edit shape (the patch does it; use it to check a reject)
Before (gen_gfx_syms.py scan): `for inc in INC.findall(text):` .. `scan(p, out, moved, ns)`.
After: `for m in INC.finditer(text):` .. `scan(p, out, moved, ns, renames_at(text, m.start(), ren))`.
Each plain name in scan() goes through `ren.get(name, name)`.
## Loop (1 build, 2 Azahar starts; base runs not charged)
1. `tr -d '\r' < docs/handoff/ws_FXREN.patch | git apply -`. A reject: stop, send the output to the oracle.
2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/fxren_host.txt 2>&1`. Read the last 20 lines.
   Pass: `gdefren ok`, `gconvw ok`, `ND HOST TESTS PASS`. Commit the 2 files: `FXREN 1: gen_gfx_syms applies #define renames to an include`.
3. `grep -cE "^(D_09000B88_373F68|D_09000BA8_373F88|D_09005090_3EAFC0|D_090050F0_3EB020)(_body)?$" build3ds/out/gset_ok.txt`. Pass: 6.
   Other count: send fxren_host.txt to the oracle and go on with step 4.
4. One build. Pass: rc 0, 0 compile errors, memcheck margin above 0.
5. `python 3ds/tools/bench.py base fxr_drec` and `python 3ds/tools/bench.py base fxp_drec`.
6. Gate, for b in fxr_drec fxp_drec: `python 3ds/tools/bench.py run $b --row FXREN --why "FXREN gate"`, then
   `python 3ds/tools/bench.py cmp $b --row FXREN --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each.
   Coverage (FACT, code read): fxr_drec has fxtest 49 fire_breath, fxp_drec has fxtest 112 peach_star_beam (pm_fxtest.c:82, :145).
   drec=412:N:44 records one frame of each slot. fire_breath.c:224 and peach_star_beam.c:184, :229 draw the lists every frame.
## Fail
- A failing test: you may edit only the 2 files. Compare them with the patch. 2 tries for one cause. Then write the output to
  build3ds/out/ and send the path to the oracle.
- A byte diff in a gate: stop. Send the cmp output path to the oracle.
## Stop
Both gates pass. The last commit `FXREN 2: gate` deletes ws_FXREN.md and ws_FXREN.patch.
## Hand-back (one SubagentHandback message)
Commits. The 3 `gconv` lines (ntab tables, gset ok, lists) of fxren_host.txt. The step 3 count. memcheck margin. The 2 cmp lines.
