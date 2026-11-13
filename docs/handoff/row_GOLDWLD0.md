# Row GOLDWLD0: golden drec base and site coverage of src/world/area_mac/mac_01/crystal_ball_gfx.c (phase G pilot, device drec form)

Missing fact (user decision): no scene runs 90 % of the sites. The pre and post builders (7 of the 20 sites) run only while
MODEL_mirrorball is on. Only the fortune event turns it on (src/world/area_mac/mac_01/npc/read_fortune.inc.c:142). crystal_ball_gfx.c
:228 turns it off at map setup. Which input does the user want: a padscript path to a paid fortune, or a story value plus a position
in Merluvlee's house? Until the answer, this row waits.
Blocked by COVMAP (cov.d lists, covsites.py). Rewritten 2026-11-13: it uses the fncov lane and edits no game file, so it does not
wait for N64DEL2.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main 0250d5221)
- Active sites by the COVMAP rule: 20. N(gfx_build_crystal_ball_pre) has 5 (:80-84), N(gfx_build_crystal_ball_post) 2 (:97, :101)
  and N(gfx_build_inside_crystal_ball) 13 (:167-170, :179, :182-185, :204, :207, :208, :213). The inside builder has a branch:
  :204-208 run only when the ball is on screen (`if (ulx >= 0 && uly >= 0 && ...)`, :203). The scene must show the ball, and
  the drec of the scene then holds the texture rectangle of :208.
- 90 % rule: 18 of 20. Inside alone gives 13 (65 %). Inside and pre give 18. All three give 20.
- The listcen count of 44 includes the 31 N64-only lines that N64DEL2 deletes. The coverage rule counts active lines only.
Start: `sh 3ds/tools/wt_new.sh GOLDWLD0`. PM_ROW=GOLDWLD0 PM_LANE=GOLDWLD0 on every call.
## Touch only (places)
New 3ds/tools/cov.d/GOLDWLD0.txt, 3ds/tools/azt.d/GOLDWLD0.json and 3ds/tools/bench.d/GOLDWLD0.json. No code file.
## Exact edits
1. cov.d/GOLDWLD0.txt: `src/world/area_mac/mac_01/crystal_ball_gfx.c`.
2. azt.d/GOLDWLD0.json, from the user's scene S (warp, story or padscript, position) and a frame F at which the fortune shows
   the ball:
   - fncov_crystal: `debug` `S fncov=F loglevel=debug det`, until `fncov f\d+ done`, cap 45, deadline 40, needs_picture false.
   - crystal_drec: `debug` `S loglevel=debug det drec=F:2`, until `drec f<F+1> draws [1-9]\d* bytes \d+`, cap 45, deadline 40.
   bench.d/GOLDWLD0.json: fncov_crystal (azahar_stream, as fxfncov_89) and crystal_drec (azahar_pic, as spot_mac01_drec).
3. Commit: `GOLDWLD0: coverage list and golden drec scene of crystal_ball_gfx.c`.
## Gate (expected values)
1. `PM_FNCOV=1 PM_LANE=GOLDWLD0 sh 3ds/build.sh` OK. `bench.py run fncov_crystal --row GOLDWLD0 --lane GOLDWLD0_fncov --why
   "GOLDWLD0 coverage"`, then `covsites.py <log> --lane GOLDWLD0 src/world/area_mac/mac_01/crystal_ball_gfx.c`:
   `sites 20/20 funcs 3/3`, or `sites 18/20` with the post builder as the only cov miss. Anything lower: stop, hand back.
2. No seed in this row: the gate is the covsites.py gate of COVMAP, and COVMAP proved its seed. This row writes no code.
3. Golden base: `bench.py base crystal_drec` (BASEKEEP: two runs, drec.txt byte-equal). A differing pair is a tool fault.
4. Budget 8 Azahar starts. Counter: none moves (phase G base for NWLD0A).
## Stop and end
Coverage below the rule, a base pair that differs, or a tool fault: stop, commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit.
