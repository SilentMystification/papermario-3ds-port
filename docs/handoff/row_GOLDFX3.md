# Row GOLDFX3: golden drec bases and site coverage of sparkles.c, effect_3D.c and snowfall.c (phase G, base of CONV2G)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE2 2026-11-13. A copy of GOLDFX2,
whose gold verdict passed at 2a052d212: water_splash 11/11, music_note 6/6, star 12/12.)
## Facts (ORACLE2, main 5c30af943)
- Sites by the covsites.py rule (functions(), 3ds/tools/covsites.py:46-68): sparkles.c 9, effect_3D.c 10, snowfall.c 9, each in
  one appendGfx function. Total 28. The 90 % rule (gold.py NEED 9) needs 26, so each appendGfx must run.
- A merged codemod row (CODEMODFX12 effect_3D.c, CODEMODFX13 snowfall.c) removes its gDPPipeSync sites, so a total below 28
  with every site hit is also a pass. Codemod rows are byte-equal, so a base from either side of them is the same.
- fxtest ids (index in FXLIST, 3ds/src/pm_fxtest.c, from 0): sparkles 12 (:44), effect_3D 55 (:87), snowfall 63 (:95).
  `fxtest=12,55,63` runs them in the slots 400-443, 444-487 and 488-531 (SLOT 44, START 400, :179-180).
## Start
`sh 3ds/tools/wt_new.sh GOLDFX3`. PM_ROW=GOLDFX3 PM_LANE=GOLDFX3 on every build and bench call.
## Touch only
Files: new 3ds/tools/cov.d/GOLDFX3.txt, new 3ds/tools/azt.d/GOLDFX3.json, new 3ds/tools/bench.d/GOLDFX3.json. No code file.
New names: gold_fx3_fncov, gold_12_drec, gold_55_drec, gold_63_drec. Host link: none.
Needs: 3ds/tools/gold.py has NEED = 9
Needs: 3ds/tools/azt.d/COVMAP2.json has fxfncov_89
## Exact edits
1. cov.d/GOLDFX3.txt: three lines `src/effects/sparkles.c`, `src/effects/effect_3D.c`, `src/effects/snowfall.c`.
2. azt.d/GOLDFX3.json:
   - gold_fx3_fncov: a copy of fxfncov_89 (azt.d/COVMAP2.json) with `debug`
     `warp=mac_01:0 pos=0:0:200 fxtest=12,55,63 fncov=412:120 loglevel=debug det`, until `fncov f\d+ done`, cap 45, deadline 40.
   - gold_12_drec, gold_55_drec, gold_63_drec: copies of spot_mac01_drec (azt.d/HOSTCORE2.json) with `debug`
     `warp=mac_01:0 pos=0:0:200 fxtest=<id> loglevel=debug det drec=412:2`, until `drec f413 draws [1-9]\d* bytes \d+`,
     first_marker and warp event `warp: mac_01 entry 0`, cap 40, deadline 35, needs_picture false.
   bench.d/GOLDFX3.json: gold_fx3_fncov (azahar_stream, tags azahar hotcode fncov, as fxfncov_89 in bench.d/COVMAP2.json) and
   the 3 drec benches (azahar_pic, tags azahar det cold drec, as spot_mac01_drec in bench.d/HOSTCORE2.json).
3. Commit: `GOLDFX3: coverage list and golden drec scenes of sparkles, effect_3D, snowfall`.
## Gate (expected values)
1. `python 3ds/tools/gold.py GOLDFX3`: `cov src/effects/sparkles.c sites 9/9 funcs 1/1`, `effect_3D.c sites 10/10`,
   `snowfall.c sites 9/9` (or lower totals after a codemod merge, all hit), and `gold GOLDFX3 sites 28/28 pass`.
2. No seed in this row: gold.py and covsites.py proved their seeds in GMHARN and COVMAP. This row writes no code.
3. Golden bases: `python 3ds/tools/bench.py base gold_12_drec`, then `base gold_55_drec`, then `base gold_63_drec` (two runs
   each, drec.txt byte-equal). Hand back the `fxtest: live` line of each base log (the effect is alive at frame 412).
4. Budget 8 Azahar starts (1 coverage run, 6 base runs). Counter: none moves (phase G bases for CONV2G).
## Stop and end
A fail verdict, a cov miss line, a base pair that differs, `fxtest: live 0` in a base log, or a tool fault: stop, commit, hand
back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
