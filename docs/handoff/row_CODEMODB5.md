# Row CODEMODB5: codemod batch of 10 effect files (87 value calls to native records), coverage and drec gate over one fxtest window

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Batch form of 2026-11-13:
host tests, coverage and one drec window per batch; no per-file seed.)
## Facts (oracle, main c2d5565fd)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame):
  - src/effects/ice_pillar.c: 7 0 0 2 | fxtest 121 | frame 412
  - src/effects/ice_shard.c: 11 0 1 3 | fxtest 114 | frame 456
  - src/effects/light_rays.c: 9 0 0 2 | fxtest 47 | frame 500
  - src/effects/merlin_house_stars.c: 9 0 1 2 | fxtest 77 | frame 544
  - src/effects/moving_cloud.c: 7 0 0 2 | fxtest 109 | frame 588
  - src/effects/purple_ring.c: 7 0 0 2 | fxtest 26 | frame 632
  - src/effects/quizmo_assistant.c: 6 0 0 2 | fxtest 120 | frame 676
  - src/effects/quizmo_audience.c: 8 0 0 2 | fxtest 78 | frame 720
  - src/effects/red_impact.c: 12 0 4 2 | fxtest 98 | frame 764
  - src/effects/ring_blast.c: 11 0 4 2 | fxtest 31 | frame 808
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. `drec=412:10:44` records those frames; `fncov=412:428` counts the calls of the whole window (COVMAP2).
- gold.py <row> builds the fncov lane, runs the fncov benches of bench.d/<row>.json and runs covsites.py over cov.d/<row>.txt.
Start: `sh 3ds/tools/wt_new.sh CODEMODB5`. PM_ROW=CODEMODB5 PM_LANE=CODEMODB5 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/ice_pillar.c, src/effects/ice_shard.c, src/effects/light_rays.c, src/effects/merlin_house_stars.c, src/effects/moving_cloud.c, src/effects/purple_ring.c, src/effects/quizmo_assistant.c, src/effects/quizmo_audience.c, src/effects/red_impact.c, src/effects/ring_blast.c. New azt.d/CODEMODB5.json, bench.d/CODEMODB5.json and cov.d/CODEMODB5.txt.
New names: codemodb5_drec, codemodb5_fncov. Host link: none.
## Exact edits
1. New azt.d/CODEMODB5.json with two copies of the fxpic_89 scenario (azt_scenarios.json; first_marker and warp event
   `warp: mac_01 entry 0`; needs_picture false; cap 45, deadline 40):
   - codemodb5_drec: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=121,114,47,77,109,26,120,78,98,31 loglevel=debug det drec=412:10:44`, until
     `drec f808 draws [1-9]\d* bytes \d+`.
   - codemodb5_fncov: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=121,114,47,77,109,26,120,78,98,31 fncov=412:428 loglevel=debug det`, until `fncov f\d+ done`.
   New bench.d/CODEMODB5.json: codemodb5_drec (kind azahar_pic, tags azahar det drec) and codemodb5_fncov (kind azahar_stream, tags azahar det
   fncov). New cov.d/CODEMODB5.txt: the 10 paths, one per line. Commit them.
2. `python 3ds/tools/codemod.py src/effects/ice_pillar.c src/effects/ice_shard.c src/effects/light_rays.c src/effects/merlin_house_stars.c src/effects/moving_cloud.c src/effects/purple_ring.c src/effects/quizmo_assistant.c src/effects/quizmo_audience.c src/effects/red_impact.c src/effects/ring_blast.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB5: codemod batch of 10 effect files (87 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/ice_pillar.c src/effects/ice_shard.c src/effects/light_rays.c src/effects/merlin_house_stars.c src/effects/moving_cloud.c src/effects/purple_ring.c src/effects/quizmo_assistant.c src/effects/quizmo_audience.c src/effects/red_impact.c src/effects/ring_blast.c`: value 0 for each file; keep, state and dl as in the table.
2. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB5 --why "CODEMODB5"`: PASS (the host unit tests).
3. Coverage: `python 3ds/tools/gold.py CODEMODB5` prints `pass`. Then `python 3ds/tools/covsites.py build3ds/out/CODEMODB5_codemodb5_fncov.log
   --lane CODEMODB5 src/effects/ice_pillar.c src/effects/ice_shard.c src/effects/light_rays.c src/effects/merlin_house_stars.c src/effects/moving_cloud.c src/effects/purple_ring.c src/effects/quizmo_assistant.c src/effects/quizmo_audience.c src/effects/red_impact.c src/effects/ring_blast.c`: each file `sites h/t` with 10 h >= 9 t. A file below that: revert its edits, take it out of cov.d and of
   the scene ids (fix it in the scenes too), and name it in the hand-back; the gate goes on with the others.
4. `bench.py base codemodb5_drec` (merge-base build), `bench.py run codemodb5_drec --row CODEMODB5 --why "CODEMODB5 drec gate"`, `bench.py cmp
   codemodb5_drec --row CODEMODB5`: drec.txt byte-equal.
5. One seeded fault for the batch (a value change, then undo): in src/effects/ice_pillar.c:165 add ` + 20.0f` after `data->pos.y` (the first transform of its draw function, row_SEEDS.md); it draws at frame 412. The ELF sha1 must differ from the real edit. Run
   codemodb5_drec again and cmp: it must differ.
6. Budget 8 Azahar starts. Counter: C1 -87, C2 -87.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
