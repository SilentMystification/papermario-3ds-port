# Row COVMAP2: the fncov census counts a window of frames (`fncov=<first>:<count>`), so one run covers a group of effects

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main cff324713)
- 3ds/src/pm_fncov.c: fn_init_once (:52-64) reads `fncov=<n>` into fn_want with atoi. pm_fncov_walk (:74-80) arms at the first
  walker entry with f >= fn_want (fn_f0 = f). It dumps at the first entry with `f > fn_f0`, so it counts one frame.
- fxtest runs the selected effects in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180, :235-262).
  `fxtest=89,33` runs water_splash at 400-443 and music_note at 444-487.
- Site counts by the COVMAP rule: water_splash.c 11 (water_splash_appendGfx), music_note.c 6 (music_note_appendGfx).
Blocked by COVMAP (covsites.py). Start: `sh 3ds/tools/wt_new.sh COVMAP2`. PM_ROW=COVMAP2 PM_LANE=COVMAP2 on every call.
## Touch only (places)
3ds/src/pm_fncov.c fn_init_once and pm_fncov_walk; new 3ds/tools/cov.d/COVMAP2.txt, azt.d/COVMAP2.json, bench.d/COVMAP2.json.
## Exact edits
1. pm_fncov.c: `static unsigned fn_cnt = 1u;`. In fn_init_once, after `fn_want = ... atoi(v)`: when v holds ':', set
   `fn_cnt = (unsigned)atoi(<text after ':'>)`. Use 1 when that value is 0. In pm_fncov_walk the dump test becomes
   `f >= fn_f0 + fn_cnt` (with fn_cnt 1 this is the old `f > fn_f0`).
2. cov.d/COVMAP2.txt: `src/effects/water_splash.c` and `src/effects/music_note.c`.
3. azt.d/COVMAP2.json: fxfncov_89_33, a copy of fxfncov_89 with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=89,33 fncov=412:76 loglevel=debug det`, cap 45, deadline 40. bench.d/COVMAP2.json:
   fxfncov_89_33, kind azahar_stream, tags azahar det fncov.
4. Commit: `COVMAP2: fncov=<first>:<count> counts a window of frames`.
## Gate (expected values)
1. Normal build and `PM_FNCOV=1 PM_LANE=COVMAP2 sh 3ds/build.sh` OK. nd_host PASS.
2. `bench.py run fxfncov_89_33 --row COVMAP2 --lane COVMAP2_fncov --why "COVMAP2 window"`, then covsites.py on its log with
   both files: `cov src/effects/water_splash.c sites 11/11 funcs 1/1` and `cov src/effects/music_note.c sites 6/6 funcs 1/1`.
3. Seed first (a value change, then undo): `fncov=412:1` in the scene. Then music_note shows `sites 0/6`.
4. Budget 8 Azahar starts. Counter: none moves (coverage tool, phase G).
## Stop and end
A coverage other than the gate, a seed that still shows 6/6, or a tool fault: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
