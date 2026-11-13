# Row CODEMODB10: codemod batch of 9 effect files (87 value calls to native records), coverage and drec gate over one fxtest window

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Batch form of 2026-11-13:
host tests, coverage and one drec window per batch; no per-file seed.)
## Facts (oracle, main c2d5565fd)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame):
  - src/effects/firework.c: 11 1 1 1 | fxtest 61 | frame 412
  - src/effects/got_item_outline.c: 11 0 5 2 | fxtest 22 | frame 456
  - src/effects/lightning.c: 13 0 0 2 | fxtest 48 | frame 500
  - src/effects/lightning_bolt.c: 6 0 2 1 | fxtest 88 | frame 544
  - src/effects/misc_particles.c: 8 0 0 2 | fxtest 107 | frame 588
  - src/effects/recover.c: 10 0 4 10 | fxtest 58 | frame 632
  - src/effects/rising_bubble.c: 9 0 0 4 | fxtest 30 | frame 676
  - src/effects/shattering_stones.c: 7 0 0 3 | fxtest 17 | frame 720
  - src/effects/shockwave.c: 12 0 6 2 | fxtest 32 | frame 764
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. `drec=412:9:44` records those frames; `fncov=412:384` counts the calls of the whole window (COVMAP2).
- gold.py <row> builds the fncov lane, runs the fncov benches of bench.d/<row>.json and runs covsites.py over cov.d/<row>.txt.
Start: `sh 3ds/tools/wt_new.sh CODEMODB10`. PM_ROW=CODEMODB10 PM_LANE=CODEMODB10 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/firework.c, src/effects/got_item_outline.c, src/effects/lightning.c, src/effects/lightning_bolt.c, src/effects/misc_particles.c, src/effects/recover.c, src/effects/rising_bubble.c, src/effects/shattering_stones.c, src/effects/shockwave.c. New azt.d/CODEMODB10.json, bench.d/CODEMODB10.json and cov.d/CODEMODB10.txt.
New names: codemodb10_drec, codemodb10_fncov. Host link: none.
## Exact edits
1. New azt.d/CODEMODB10.json with two copies of the fxpic_89 scenario (azt_scenarios.json; first_marker and warp event
   `warp: mac_01 entry 0`; needs_picture false; cap 45, deadline 40):
   - codemodb10_drec: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=61,22,48,88,107,58,30,17,32 loglevel=debug det drec=412:9:44`, until
     `drec f764 draws [1-9]\d* bytes \d+`.
   - codemodb10_fncov: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=61,22,48,88,107,58,30,17,32 fncov=412:384 loglevel=debug det`, until `fncov f\d+ done`.
   New bench.d/CODEMODB10.json: codemodb10_drec (kind azahar_pic, tags azahar det drec) and codemodb10_fncov (kind azahar_stream, tags azahar det
   fncov). New cov.d/CODEMODB10.txt: the 9 paths, one per line. Commit them.
2. `python 3ds/tools/codemod.py src/effects/firework.c src/effects/got_item_outline.c src/effects/lightning.c src/effects/lightning_bolt.c src/effects/misc_particles.c src/effects/recover.c src/effects/rising_bubble.c src/effects/shattering_stones.c src/effects/shockwave.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB10: codemod batch of 9 effect files (87 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/firework.c src/effects/got_item_outline.c src/effects/lightning.c src/effects/lightning_bolt.c src/effects/misc_particles.c src/effects/recover.c src/effects/rising_bubble.c src/effects/shattering_stones.c src/effects/shockwave.c`: value 0 for each file; keep, state and dl as in the table.
2. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB10 --why "CODEMODB10"`: PASS (the host unit tests).
3. Coverage: `python 3ds/tools/gold.py CODEMODB10` prints `pass`. Then `python 3ds/tools/covsites.py build3ds/out/CODEMODB10_codemodb10_fncov.log
   --lane CODEMODB10 src/effects/firework.c src/effects/got_item_outline.c src/effects/lightning.c src/effects/lightning_bolt.c src/effects/misc_particles.c src/effects/recover.c src/effects/rising_bubble.c src/effects/shattering_stones.c src/effects/shockwave.c`: each file `sites h/t` with 10 h >= 9 t. A file below that: revert its edits, take it out of cov.d and of
   the scene ids (fix it in the scenes too), and name it in the hand-back; the gate goes on with the others.
4. `bench.py base codemodb10_drec` (merge-base build), `bench.py run codemodb10_drec --row CODEMODB10 --why "CODEMODB10 drec gate"`, `bench.py cmp
   codemodb10_drec --row CODEMODB10`: drec.txt byte-equal.
5. One seeded fault for the batch (a value change, then undo): in src/effects/firework.c:203 add ` + 20.0f` after `part->unk_08` (the first transform of its draw function, row_SEEDS.md); it draws at frame 412. The ELF sha1 must differ from the real edit. Run
   codemodb10_drec again and cmp: it must differ.
6. Budget 8 Azahar starts. Counter: C1 -87, C2 -87.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
