# Row GOLDFX4: golden drec bases and site coverage of snaking_static.c, water_block.c and embers.c (phase G, base of CONV2H)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE2 2026-11-13. A copy of GOLDFX2,
whose gold verdict passed at 2a052d212: water_splash 11/11, music_note 6/6, star 12/12.)
## Facts (ORACLE2, main 5c30af943)
- Sites by the covsites.py rule (functions(), 3ds/tools/covsites.py:46-68): snaking_static.c 14, water_block.c 19, embers.c 12,
  each in one appendGfx function. Total 45. The 90 % rule (gold.py NEED 9) needs 41, so each appendGfx must run.
- A merged codemod row (CODEMODFX11 snaking_static.c) removes its gDPPipeSync sites, so a total below 45 with every site hit is
  also a pass. Codemod rows are byte-equal, so a base from either side of them is the same.
- fxtest ids (index in FXLIST, 3ds/src/pm_fxtest.c, from 0): snaking_static 81 (:113), water_block 84 (:116), embers 105 (:137).
  `fxtest=81,84,105` runs them in the slots 400-443, 444-487 and 488-531 (SLOT 44, START 400, :179-180).
## Start
`sh 3ds/tools/wt_new.sh GOLDFX4`. PM_ROW=GOLDFX4 PM_LANE=GOLDFX4 on every build and bench call.
## Touch only
Files: new 3ds/tools/cov.d/GOLDFX4.txt, new 3ds/tools/azt.d/GOLDFX4.json, new 3ds/tools/bench.d/GOLDFX4.json. No code file.
New names: gold_fx4_fncov, gold_81_drec, gold_84_drec, gold_105_drec. Host link: none.
Needs: 3ds/tools/gold.py has NEED = 9
Needs: 3ds/tools/azt.d/COVMAP2.json has fxfncov_89
## Exact edits
1. cov.d/GOLDFX4.txt: three lines `src/effects/snaking_static.c`, `src/effects/water_block.c`, `src/effects/embers.c`.
2. azt.d/GOLDFX4.json:
   - gold_fx4_fncov: a copy of fxfncov_89 (azt.d/COVMAP2.json) with `debug`
     `warp=mac_01:0 pos=0:0:200 fxtest=81,84,105 fncov=412:120 loglevel=debug det`, until `fncov f\d+ done`, cap 45, deadline 40.
   - gold_81_drec, gold_84_drec, gold_105_drec: copies of spot_mac01_drec (azt.d/HOSTCORE2.json) with `debug`
     `warp=mac_01:0 pos=0:0:200 fxtest=<id> loglevel=debug det drec=412:2`, until `drec f413 draws [1-9]\d* bytes \d+`,
     first_marker and warp event `warp: mac_01 entry 0`, cap 40, deadline 35, needs_picture false.
   bench.d/GOLDFX4.json: gold_fx4_fncov (azahar_stream, tags azahar hotcode fncov, as fxfncov_89 in bench.d/COVMAP2.json) and
   the 3 drec benches (azahar_pic, tags azahar det cold drec, as spot_mac01_drec in bench.d/HOSTCORE2.json).
3. Commit: `GOLDFX4: coverage list and golden drec scenes of snaking_static, water_block, embers`.
## Gate (expected values)
1. `python 3ds/tools/gold.py GOLDFX4`: `cov src/effects/snaking_static.c sites 14/14 funcs 1/1`, `water_block.c sites 19/19`,
   `embers.c sites 12/12` (or a lower total after a codemod merge, all hit), and `gold GOLDFX4 sites 45/45 pass`.
2. No seed in this row: gold.py and covsites.py proved their seeds in GMHARN and COVMAP. This row writes no code.
3. Golden bases: `python 3ds/tools/bench.py base gold_81_drec`, then `base gold_84_drec`, then `base gold_105_drec` (two runs
   each, drec.txt byte-equal). Hand back the `fxtest: live` line of each base log (the effect is alive at frame 412).
4. Budget 8 Azahar starts (1 coverage run, 6 base runs). Counter: none moves (phase G bases for CONV2H).
## Stop and end
A fail verdict, a cov miss line, a base pair that differs, `fxtest: live 0` in a base log, or a tool fault: stop, commit, hand
back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
