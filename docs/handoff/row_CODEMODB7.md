# Row CODEMODB7: codemod batch of 9 effect files (73 value calls to native records), coverage and drec gate over one fxtest window

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Batch form of 2026-11-13:
host tests, coverage and one drec window per batch; no per-file seed.)
## Facts (oracle, main c2d5565fd)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame):
  - src/effects/stars_shimmer.c: 6 0 1 2 | fxtest 29 | frame 412
  - src/effects/stars_spread.c: 7 0 0 2 | fxtest 39 | frame 456
  - src/effects/static_status.c: 8 0 0 2 | fxtest 108 | frame 500
  - src/effects/steam_burst.c: 11 0 0 2 | fxtest 40 | frame 544
  - src/effects/stop_watch.c: 11 0 1 1 | fxtest 92 | frame 588
  - src/effects/sweat.c: 4 0 0 2 | fxtest 35 | frame 632
  - src/effects/throw_spiny.c: 6 0 0 2 | fxtest 94 | frame 676
  - src/effects/thunderbolt_ring.c: 11 0 0 2 | fxtest 82 | frame 720
  - src/effects/tubba_heart_attack.c: 9 0 0 2 | fxtest 96 | frame 764
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. `drec=412:9:44` records those frames; `fncov=412:384` counts the calls of the whole window (COVMAP2).
- gold.py <row> builds the fncov lane, runs the fncov benches of bench.d/<row>.json and runs covsites.py over cov.d/<row>.txt.
Start: `sh 3ds/tools/wt_new.sh CODEMODB7`. PM_ROW=CODEMODB7 PM_LANE=CODEMODB7 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/stars_shimmer.c, src/effects/stars_spread.c, src/effects/static_status.c, src/effects/steam_burst.c, src/effects/stop_watch.c, src/effects/sweat.c, src/effects/throw_spiny.c, src/effects/thunderbolt_ring.c, src/effects/tubba_heart_attack.c. New azt.d/CODEMODB7.json, bench.d/CODEMODB7.json and cov.d/CODEMODB7.txt.
New names: codemodb7_drec, codemodb7_fncov. Host link: none.
## Exact edits
1. New azt.d/CODEMODB7.json with two copies of the fxpic_89 scenario (azt_scenarios.json; first_marker and warp event
   `warp: mac_01 entry 0`; needs_picture false; cap 45, deadline 40):
   - codemodb7_drec: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=29,39,108,40,92,35,94,82,96 loglevel=debug det drec=412:9:44`, until
     `drec f764 draws [1-9]\d* bytes \d+`.
   - codemodb7_fncov: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=29,39,108,40,92,35,94,82,96 fncov=412:384 loglevel=debug det`, until `fncov f\d+ done`.
   New bench.d/CODEMODB7.json: codemodb7_drec (kind azahar_pic, tags azahar det drec) and codemodb7_fncov (kind azahar_stream, tags azahar det
   fncov). New cov.d/CODEMODB7.txt: the 9 paths, one per line. Commit them.
2. `python 3ds/tools/codemod.py src/effects/stars_shimmer.c src/effects/stars_spread.c src/effects/static_status.c src/effects/steam_burst.c src/effects/stop_watch.c src/effects/sweat.c src/effects/throw_spiny.c src/effects/thunderbolt_ring.c src/effects/tubba_heart_attack.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB7: codemod batch of 9 effect files (73 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/stars_shimmer.c src/effects/stars_spread.c src/effects/static_status.c src/effects/steam_burst.c src/effects/stop_watch.c src/effects/sweat.c src/effects/throw_spiny.c src/effects/thunderbolt_ring.c src/effects/tubba_heart_attack.c`: value 0 for each file; keep, state and dl as in the table.
2. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB7 --why "CODEMODB7"`: PASS (the host unit tests).
3. Coverage: `python 3ds/tools/gold.py CODEMODB7` prints `pass`. Then `python 3ds/tools/covsites.py build3ds/out/CODEMODB7_codemodb7_fncov.log
   --lane CODEMODB7 src/effects/stars_shimmer.c src/effects/stars_spread.c src/effects/static_status.c src/effects/steam_burst.c src/effects/stop_watch.c src/effects/sweat.c src/effects/throw_spiny.c src/effects/thunderbolt_ring.c src/effects/tubba_heart_attack.c`: each file `sites h/t` with 10 h >= 9 t. A file below that: revert its edits, take it out of cov.d and of
   the scene ids (fix it in the scenes too), and name it in the hand-back; the gate goes on with the others.
4. `bench.py base codemodb7_drec` (merge-base build), `bench.py run codemodb7_drec --row CODEMODB7 --why "CODEMODB7 drec gate"`, `bench.py cmp
   codemodb7_drec --row CODEMODB7`: drec.txt byte-equal.
5. One seeded fault for the batch (a value change, then undo): in src/effects/stars_shimmer.c:299 add ` + 20.0f` after `data->pos.y` (the first transform of its draw function, row_SEEDS.md); it draws at frame 412. The ELF sha1 must differ from the real edit. Run
   codemodb7_drec again and cmp: it must differ.
6. Budget 8 Azahar starts. Counter: C1 -73, C2 -73.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
