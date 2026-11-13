# Row CODEMODB1: codemod batch of 9 effect files (109 value calls to native records), coverage and drec gate over one fxtest window

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Batch form of 2026-11-13:
host tests, coverage and one drec window per batch; no per-file seed.)
## Facts (oracle, main c2d5565fd)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame):
  - src/effects/effect_3D.c: 5 3 0 2 | fxtest 55 | frame 412
  - src/effects/snowfall.c: 4 3 0 2 | fxtest 63 | frame 456
  - src/effects/pink_sparkles.c: 3 3 0 2 | fxtest 124 | frame 500
  - src/effects/radiating_energy_orb.c: 20 0 2 7 | fxtest 73 | frame 544
  - src/effects/quizmo_stage.c: 20 0 1 13 | fxtest 72 | frame 588
  - src/effects/peach_star_beam.c: 15 0 0 8 | fxtest 112 | frame 632
  - src/effects/whirlwind.c: 14 0 3 1 | fxtest 97 | frame 676
  - src/effects/landing_dust.c: 14 0 0 3 | fxtest 1 | frame 720
  - src/effects/damage_indicator.c: 14 0 4 15 | fxtest 25 | frame 764
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. `drec=412:9:44` records those frames; `fncov=412:384` counts the calls of the whole window (COVMAP2).
- gold.py <row> builds the fncov lane, runs the fncov benches of bench.d/<row>.json and runs covsites.py over cov.d/<row>.txt.
Start: `sh 3ds/tools/wt_new.sh CODEMODB1`. PM_ROW=CODEMODB1 PM_LANE=CODEMODB1 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/effect_3D.c, src/effects/snowfall.c, src/effects/pink_sparkles.c, src/effects/radiating_energy_orb.c, src/effects/quizmo_stage.c, src/effects/peach_star_beam.c, src/effects/whirlwind.c, src/effects/landing_dust.c, src/effects/damage_indicator.c. New azt.d/CODEMODB1.json, bench.d/CODEMODB1.json and cov.d/CODEMODB1.txt.
New names: codemodb1_drec, codemodb1_fncov. Host link: none.
## Exact edits
1. New azt.d/CODEMODB1.json with two copies of the fxpic_89 scenario (azt_scenarios.json; first_marker and warp event
   `warp: mac_01 entry 0`; needs_picture false; cap 45, deadline 40):
   - codemodb1_drec: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=55,63,124,73,72,112,97,1,25 loglevel=debug det drec=412:9:44`, until
     `drec f764 draws [1-9]\d* bytes \d+`.
   - codemodb1_fncov: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=55,63,124,73,72,112,97,1,25 fncov=412:384 loglevel=debug det`, until `fncov f\d+ done`.
   New bench.d/CODEMODB1.json: codemodb1_drec (kind azahar_pic, tags azahar det drec) and codemodb1_fncov (kind azahar_stream, tags azahar det
   fncov). New cov.d/CODEMODB1.txt: the 9 paths, one per line. Commit them.
2. `python 3ds/tools/codemod.py src/effects/effect_3D.c src/effects/snowfall.c src/effects/pink_sparkles.c src/effects/radiating_energy_orb.c src/effects/quizmo_stage.c src/effects/peach_star_beam.c src/effects/whirlwind.c src/effects/landing_dust.c src/effects/damage_indicator.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB1: codemod batch of 9 effect files (109 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/effect_3D.c src/effects/snowfall.c src/effects/pink_sparkles.c src/effects/radiating_energy_orb.c src/effects/quizmo_stage.c src/effects/peach_star_beam.c src/effects/whirlwind.c src/effects/landing_dust.c src/effects/damage_indicator.c`: value 0 for each file; keep, state and dl as in the table.
2. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB1 --why "CODEMODB1"`: PASS (the host unit tests).
3. Coverage: `python 3ds/tools/gold.py CODEMODB1` prints `pass`. Then `python 3ds/tools/covsites.py build3ds/out/CODEMODB1_codemodb1_fncov.log
   --lane CODEMODB1 src/effects/effect_3D.c src/effects/snowfall.c src/effects/pink_sparkles.c src/effects/radiating_energy_orb.c src/effects/quizmo_stage.c src/effects/peach_star_beam.c src/effects/whirlwind.c src/effects/landing_dust.c src/effects/damage_indicator.c`: each file `sites h/t` with 10 h >= 9 t. A file below that: revert its edits, take it out of cov.d and of
   the scene ids (fix it in the scenes too), and name it in the hand-back; the gate goes on with the others.
4. `bench.py base codemodb1_drec` (merge-base build), `bench.py run codemodb1_drec --row CODEMODB1 --why "CODEMODB1 drec gate"`, `bench.py cmp
   codemodb1_drec --row CODEMODB1`: drec.txt byte-equal.
5. One seeded fault for the batch (a value change, then undo): in src/effects/effect_3D.c:229 add ` + 20.0f` after `part->pos.y` (the first transform of its draw function, row_SEEDS.md); it draws at frame 412. The ELF sha1 must differ from the real edit. Run
   codemodb1_drec again and cmp: it must differ.
6. Budget 8 Azahar starts. Counter: C1 -109, C2 -109.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
