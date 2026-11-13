# Row CODEMODB6: codemod batch of 9 effect files (76 value calls to native records), coverage and drec gate over one fxtest window

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Batch form of 2026-11-13:
host tests, coverage and one drec window per batch; no per-file seed.)
## Facts (oracle, main c2d5565fd)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame):
  - src/effects/shimmer_wave.c: 12 0 0 2 | fxtest 52 | frame 412
  - src/effects/shiny_flare.c: 7 0 0 2 | fxtest 102 | frame 456
  - src/effects/small_gold_sparkle.c: 9 0 0 3 | fxtest 67 | frame 500
  - src/effects/smoke_burst.c: 9 0 2 2 | fxtest 34 | frame 544
  - src/effects/smoke_impact.c: 13 0 2 2 | fxtest 24 | frame 588
  - src/effects/snowflake.c: 7 0 0 2 | fxtest 9 | frame 632
  - src/effects/snowman_doll.c: 7 0 0 2 | fxtest 90 | frame 676
  - src/effects/sparkles.c: 5 0 2 1 | fxtest 12 | frame 720
  - src/effects/squirt.c: 7 0 1 1 | fxtest 83 | frame 764
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. `drec=412:9:44` records those frames; `fncov=412:384` counts the calls of the whole window (COVMAP2).
- gold.py <row> builds the fncov lane, runs the fncov benches of bench.d/<row>.json and runs covsites.py over cov.d/<row>.txt.
Start: `sh 3ds/tools/wt_new.sh CODEMODB6`. PM_ROW=CODEMODB6 PM_LANE=CODEMODB6 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/shimmer_wave.c, src/effects/shiny_flare.c, src/effects/small_gold_sparkle.c, src/effects/smoke_burst.c, src/effects/smoke_impact.c, src/effects/snowflake.c, src/effects/snowman_doll.c, src/effects/sparkles.c, src/effects/squirt.c. New azt.d/CODEMODB6.json, bench.d/CODEMODB6.json and cov.d/CODEMODB6.txt.
New names: codemodb6_drec, codemodb6_fncov. Host link: none.
## Exact edits
1. New azt.d/CODEMODB6.json with two copies of the fxpic_89 scenario (azt_scenarios.json; first_marker and warp event
   `warp: mac_01 entry 0`; needs_picture false; cap 45, deadline 40):
   - codemodb6_drec: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=52,102,67,34,24,9,90,12,83 loglevel=debug det drec=412:9:44`, until
     `drec f764 draws [1-9]\d* bytes \d+`.
   - codemodb6_fncov: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=52,102,67,34,24,9,90,12,83 fncov=412:384 loglevel=debug det`, until `fncov f\d+ done`.
   New bench.d/CODEMODB6.json: codemodb6_drec (kind azahar_pic, tags azahar det drec) and codemodb6_fncov (kind azahar_stream, tags azahar det
   fncov). New cov.d/CODEMODB6.txt: the 9 paths, one per line. Commit them.
2. `python 3ds/tools/codemod.py src/effects/shimmer_wave.c src/effects/shiny_flare.c src/effects/small_gold_sparkle.c src/effects/smoke_burst.c src/effects/smoke_impact.c src/effects/snowflake.c src/effects/snowman_doll.c src/effects/sparkles.c src/effects/squirt.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB6: codemod batch of 9 effect files (76 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/shimmer_wave.c src/effects/shiny_flare.c src/effects/small_gold_sparkle.c src/effects/smoke_burst.c src/effects/smoke_impact.c src/effects/snowflake.c src/effects/snowman_doll.c src/effects/sparkles.c src/effects/squirt.c`: value 0 for each file; keep, state and dl as in the table.
2. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB6 --why "CODEMODB6"`: PASS (the host unit tests).
3. Coverage: `python 3ds/tools/gold.py CODEMODB6` prints `pass`. Then `python 3ds/tools/covsites.py build3ds/out/CODEMODB6_codemodb6_fncov.log
   --lane CODEMODB6 src/effects/shimmer_wave.c src/effects/shiny_flare.c src/effects/small_gold_sparkle.c src/effects/smoke_burst.c src/effects/smoke_impact.c src/effects/snowflake.c src/effects/snowman_doll.c src/effects/sparkles.c src/effects/squirt.c`: each file `sites h/t` with 10 h >= 9 t. A file below that: revert its edits, take it out of cov.d and of
   the scene ids (fix it in the scenes too), and name it in the hand-back; the gate goes on with the others.
4. `bench.py base codemodb6_drec` (merge-base build), `bench.py run codemodb6_drec --row CODEMODB6 --why "CODEMODB6 drec gate"`, `bench.py cmp
   codemodb6_drec --row CODEMODB6`: drec.txt byte-equal.
5. One seeded fault for the batch (a value change, then undo): in src/effects/shimmer_wave.c:202 add ` + 20.0f` after `part->unk_08` (the first transform of its draw function, row_SEEDS.md); it draws at frame 412. The ELF sha1 must differ from the real edit. Run
   codemodb6_drec again and cmp: it must differ.
6. Budget 8 Azahar starts. Counter: C1 -76, C2 -76.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
