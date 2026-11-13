# Row GOLDENT1: golden drec base and site coverage of src/entity/sbk_omo/Tweester.c (phase G pilot, device drec form)

Blocked by COVMAP (cov.d lists, covsites.py). Release after the COVMAP merge.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main 0250d5221)
- src/entity/sbk_omo/Tweester.c: the largest call-site file of src/entity (listcen build 42). With the COVMAP site rule it has 42 sites in 4 straight-line functions: entity_Tweester_render_inner_whirl 3 (:24-40), render_outer_whirl 3 (:42-58), render_face 11 (:60-75), setupGfx 25 (:77-109).
- The Tweester of sbk_14 is made at 327 0 8 (src/world/area_sbk/sbk_14/entity.c:27). Its path loop is -65 0 217, -262 0 -64, 57 0 -286, 327 0 8 (:4-10). Entry 3 is at 0 0 475 (main.c:12). A touch sends Mario to sbk_05, but the path stays 258 units or more from entry 3. Missing fact: no run has shown that a warp to sbk_14 makes the Tweester at the default story. If the coverage is 0/42, stop and hand back.
- 90 % rule: at least 38 covered sites, or the row adds scenes. The drec switch is `drec=<first>:<count>` (pm_gpu.c:574).
  The bench kind is azahar_pic, with drec.txt compared as for spot_mac01_drec (bench.d/HOSTCORE2.json).
Start: `sh 3ds/tools/wt_new.sh GOLDENT1`. PM_ROW=GOLDENT1 PM_LANE=GOLDENT1 on every call.
## Touch only (places)
New 3ds/tools/cov.d/GOLDENT1.txt, 3ds/tools/azt.d/GOLDENT1.json and 3ds/tools/bench.d/GOLDENT1.json. No code file.
## Exact edits
1. cov.d/GOLDENT1.txt: `src/entity/sbk_omo/Tweester.c`.
2. azt.d/GOLDENT1.json has two scenarios, copies of fxfncov_89 and spot_mac01_drec:
   - fncov_sbk14_det: `debug` `warp=sbk_14:3 fncov=200 loglevel=debug det`, until `fncov f\d+ done`, first_marker and the warp event `warp: sbk_14 entry 3`, cap 45,
     deadline 40, needs_picture false.
   - sbk14_drec: `debug` `warp=sbk_14:3 loglevel=debug det drec=200:2`, until `drec f201 draws [1-9]\d* bytes \d+`, the same warp marker, cap 40, deadline 35, needs_picture false.
   bench.d/GOLDENT1.json: fncov_sbk14_det (kind azahar_stream, as fxfncov_89) and sbk14_drec (kind azahar_pic, as spot_mac01_drec).
3. Commit: `GOLDENT1: coverage list and golden drec scene of src/entity/sbk_omo/Tweester.c`.
## Gate (expected values)
1. `PM_FNCOV=1 PM_LANE=GOLDENT1 sh 3ds/build.sh` OK. `bench.py run fncov_sbk14_det --row GOLDENT1 --lane GOLDENT1_fncov --why
   "GOLDENT1 coverage"`, then `python 3ds/tools/covsites.py <its log> --lane GOLDENT1 src/entity/sbk_omo/Tweester.c`:
   `cov src/entity/sbk_omo/Tweester.c sites 42/42 funcs 4/4`. A value below 38: stop and hand back the cov miss lines.
2. No seed in this row: the gate is the covsites.py gate of COVMAP, and COVMAP proved its seed. This row writes no code.
3. Golden base: `bench.py base sbk14_drec` (BASEKEEP: two runs, drec.txt byte-equal). A differing pair is a tool fault.
4. Budget 8 Azahar starts. Counter: none moves (phase G: the base that the conversion rows of this file compare against).
## Stop and end
Coverage below the rule, a base pair that differs, or a tool fault: stop, commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit.
