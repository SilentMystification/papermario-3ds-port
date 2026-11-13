# Row GOLDFX2: golden drec bases and site coverage of water_splash.c, music_note.c and star.c (phase G, device drec form)

Blocked by GMHARN (gold.py) and COVMAP2 (frame window).
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main cff324713)
- Sites by the COVMAP rule: water_splash.c 11, music_note.c 6, star.c 15, each in one appendGfx function. Total 32. The 90 %
  rule needs 29. These functions have branches inside (loops, ifs). The function count is the measure that the plan accepts.
- fxtest ids: water_splash 89, music_note 33, star 10. `fxtest=89,33,10` runs them at 400, 444 and 488 (44 frames each).
- These files are the CODEMODFX1 and FX2 files. Codemod rows are byte-equal, so a base from either side of them is the same.
Start: `sh 3ds/tools/wt_new.sh GOLDFX2`. PM_ROW=GOLDFX2 PM_LANE=GOLDFX2 on every call.
## Touch only (places)
New 3ds/tools/cov.d/GOLDFX2.txt, 3ds/tools/azt.d/GOLDFX2.json and 3ds/tools/bench.d/GOLDFX2.json. No code file.
## Exact edits
1. cov.d/GOLDFX2.txt: `src/effects/water_splash.c`, `src/effects/music_note.c`, `src/effects/star.c`.
2. azt.d/GOLDFX2.json:
   - gold_fx2_fncov: copy of fxfncov_89 with `debug` `warp=mac_01:0 pos=0:0:200 fxtest=89,33,10 fncov=412:120 loglevel=debug
     det`, until `fncov f\d+ done`, cap 45, deadline 40.
   - gold_89_drec, gold_33_drec, gold_10_drec: copies of spot_mac01_drec with `debug`
     `warp=mac_01:0 pos=0:0:200 fxtest=<id> loglevel=debug det drec=412:2`, until `drec f413 draws [1-9]\d* bytes \d+`,
     first_marker and warp event `warp: mac_01 entry 0`, cap 40, deadline 35, needs_picture false.
   bench.d/GOLDFX2.json: gold_fx2_fncov (azahar_stream, tags fncov) and the 3 drec benches (azahar_pic, tags drec).
3. Commit: `GOLDFX2: coverage list and golden drec scenes of water_splash, music_note, star`.
## Gate (expected values)
1. `python 3ds/tools/gold.py GOLDFX2`: `gold GOLDFX2 sites 32/32 pass` (29 or more passes; hand back any cov miss line).
2. No seed in this row: gold.py and covsites.py proved their seeds in GMHARN and COVMAP. This row writes no code.
3. Golden bases: `bench.py base gold_89_drec`, `base gold_33_drec`, `base gold_10_drec` (BASEKEEP pairs). A differing pair is a
   tool fault.
4. Budget 8 Azahar starts (1 coverage run, 6 base runs). Counter: none moves (phase G bases).
## Stop and end
A fail verdict, a base pair that differs, or a tool fault: stop, commit, hand back the exact output. Delete this row file at the end.
