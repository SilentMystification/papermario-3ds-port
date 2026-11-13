# Row GOLDFX1: golden drec base and site coverage of src/effects/fright_jar.c (phase G pilot, device drec form)

Blocked by COVMAP (cov.d lists, covsites.py). Release after the COVMAP merge.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main 0250d5221)
- src/effects/fright_jar.c: the largest call-site file of src/effects (listcen build 35). With the COVMAP site rule it has 36 sites, all in fright_jar_appendGfx (src/effects/fright_jar.c:116-222, straight line apart from a loop of 10 whose switch runs every case once), so calls > 0 means 36/36.
- fxtest id 91 = fright_jar (pm_fxtest.c FXLIST :123, `fx_fright_jar(0, X, Y, Z, 1.5f, 60)`). The effect spawns at frame 400 (pm_fxtest.c:180).
- 90 % rule: at least 33 covered sites, or the row adds scenes. The drec switch is `drec=<first>:<count>` (pm_gpu.c:574).
  The bench kind is azahar_pic, with drec.txt compared as for spot_mac01_drec (bench.d/HOSTCORE2.json).
Start: `sh 3ds/tools/wt_new.sh GOLDFX1`. PM_ROW=GOLDFX1 PM_LANE=GOLDFX1 on every call.
## Touch only (places)
New 3ds/tools/cov.d/GOLDFX1.txt, 3ds/tools/azt.d/GOLDFX1.json and 3ds/tools/bench.d/GOLDFX1.json. No code file.
## Exact edits
1. cov.d/GOLDFX1.txt: `src/effects/fright_jar.c`.
2. azt.d/GOLDFX1.json has two scenarios, copies of fxfncov_89 and spot_mac01_drec:
   - fxfncov_91: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=91 fncov=420 loglevel=debug det`, until `fncov f\d+ done`, first_marker and the warp event `warp: mac_01 entry 0`, cap 45,
     deadline 40, needs_picture false.
   - fxpic_91_drec: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=91 loglevel=debug det drec=412:2`, until `drec f413 draws [1-9]\d* bytes \d+`, the same warp marker, cap 40, deadline 35, needs_picture false.
   bench.d/GOLDFX1.json: fxfncov_91 (kind azahar_stream, as fxfncov_89) and fxpic_91_drec (kind azahar_pic, as spot_mac01_drec).
3. Commit: `GOLDFX1: coverage list and golden drec scene of src/effects/fright_jar.c`.
## Gate (expected values)
1. `PM_FNCOV=1 PM_LANE=GOLDFX1 sh 3ds/build.sh` OK. `bench.py run fxfncov_91 --row GOLDFX1 --lane GOLDFX1_fncov --why
   "GOLDFX1 coverage"`, then `python 3ds/tools/covsites.py <its log> --lane GOLDFX1 src/effects/fright_jar.c`:
   `cov src/effects/fright_jar.c sites 36/36 funcs 1/1`. A value below 33: stop and hand back the cov miss lines.
2. No seed in this row: the gate is the covsites.py gate of COVMAP, and COVMAP proved its seed. This row writes no code.
3. Golden base: `bench.py base fxpic_91_drec` (BASEKEEP: two runs, drec.txt byte-equal). A differing pair is a tool fault.
4. Budget 8 Azahar starts. Counter: none moves (phase G: the base that the conversion rows of this file compare against).
## Stop and end
Coverage below the rule, a base pair that differs, or a tool fault: stop, commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit.
