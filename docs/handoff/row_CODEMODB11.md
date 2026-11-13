# Row CODEMODB11: codemod batch of 9 effect files (75 value calls to native records), coverage and drec gate over one fxtest window

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Batch form of 2026-11-13:
host tests, coverage and one drec window per batch; no per-file seed.)
## Facts (oracle, main c2d5565fd)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame):
  - src/effects/something_rotating.c: 10 0 0 6 | fxtest 117 | frame 412
  - src/effects/spiky_white_aura.c: 5 0 0 2 | fxtest 23 | frame 456
  - src/effects/spirit_card.c: 6 0 0 4 | fxtest 115 | frame 500
  - src/effects/star_spirits_energy.c: 12 0 6 3 | fxtest 123 | frame 544
  - src/effects/stars_burst.c: 5 0 0 2 | fxtest 28 | frame 588
  - src/effects/stars_orbiting.c: 7 0 0 2 | fxtest 41 | frame 632
  - src/effects/stat_change.c: 10 0 4 8 | fxtest 80 | frame 676
  - src/effects/sun.c: 10 0 0 2 | fxtest 122 | frame 720
  - src/effects/walking_dust.c: 10 0 0 2 | fxtest 2 | frame 764
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. `drec=412:9:44` records those frames; `fncov=412:384` counts the calls of the whole window (COVMAP2).
- gold.py <row> builds the fncov lane, runs the fncov benches of bench.d/<row>.json and runs covsites.py over cov.d/<row>.txt.
Start: `sh 3ds/tools/wt_new.sh CODEMODB11`. PM_ROW=CODEMODB11 PM_LANE=CODEMODB11 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/something_rotating.c, src/effects/spiky_white_aura.c, src/effects/spirit_card.c, src/effects/star_spirits_energy.c, src/effects/stars_burst.c, src/effects/stars_orbiting.c, src/effects/stat_change.c, src/effects/sun.c, src/effects/walking_dust.c. New azt.d/CODEMODB11.json, bench.d/CODEMODB11.json and cov.d/CODEMODB11.txt.
New names: codemodb11_drec, codemodb11_fncov. Host link: none.
## Exact edits
1. New azt.d/CODEMODB11.json with two copies of the fxpic_89 scenario (azt_scenarios.json; first_marker and warp event
   `warp: mac_01 entry 0`; needs_picture false; cap 45, deadline 40):
   - codemodb11_drec: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=117,23,115,123,28,41,80,122,2 loglevel=debug det drec=412:9:44`, until
     `drec f764 draws [1-9]\d* bytes \d+`.
   - codemodb11_fncov: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=117,23,115,123,28,41,80,122,2 fncov=412:384 loglevel=debug det`, until `fncov f\d+ done`.
   New bench.d/CODEMODB11.json: codemodb11_drec (kind azahar_pic, tags azahar det drec) and codemodb11_fncov (kind azahar_stream, tags azahar det
   fncov). New cov.d/CODEMODB11.txt: the 9 paths, one per line. Commit them.
2. `python 3ds/tools/codemod.py src/effects/something_rotating.c src/effects/spiky_white_aura.c src/effects/spirit_card.c src/effects/star_spirits_energy.c src/effects/stars_burst.c src/effects/stars_orbiting.c src/effects/stat_change.c src/effects/sun.c src/effects/walking_dust.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB11: codemod batch of 9 effect files (75 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/something_rotating.c src/effects/spiky_white_aura.c src/effects/spirit_card.c src/effects/star_spirits_energy.c src/effects/stars_burst.c src/effects/stars_orbiting.c src/effects/stat_change.c src/effects/sun.c src/effects/walking_dust.c`: value 0 for each file; keep, state and dl as in the table.
2. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB11 --why "CODEMODB11"`: PASS (the host unit tests).
3. Coverage: `python 3ds/tools/gold.py CODEMODB11` prints `pass`. Then `python 3ds/tools/covsites.py build3ds/out/CODEMODB11_codemodb11_fncov.log
   --lane CODEMODB11 src/effects/something_rotating.c src/effects/spiky_white_aura.c src/effects/spirit_card.c src/effects/star_spirits_energy.c src/effects/stars_burst.c src/effects/stars_orbiting.c src/effects/stat_change.c src/effects/sun.c src/effects/walking_dust.c`: each file `sites h/t` with 10 h >= 9 t. A file below that: revert its edits, take it out of cov.d and of
   the scene ids (fix it in the scenes too), and name it in the hand-back; the gate goes on with the others.
4. `bench.py base codemodb11_drec` (merge-base build), `bench.py run codemodb11_drec --row CODEMODB11 --why "CODEMODB11 drec gate"`, `bench.py cmp
   codemodb11_drec --row CODEMODB11`: drec.txt byte-equal.
5. One seeded fault for the batch (a value change, then undo): in src/effects/something_rotating.c:272 add ` + 20.0f` after `part->unk_1C` (the first transform of its draw function, row_SEEDS.md); it draws at frame 412. The ELF sha1 must differ from the real edit. Run
   codemodb11_drec again and cmp: it must differ.
6. Budget 8 Azahar starts. Counter: C1 -75, C2 -75.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
