# Row CODEMODB2: codemod batch of 10 effect files (77 value calls to native records), coverage and drec gate over one fxtest window

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Batch form of 2026-11-13:
host tests, coverage and one drec window per batch; no per-file seed.)
## Facts (oracle, main c2d5565fd)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame):
  - src/effects/aura.c: 10 0 0 5 | fxtest 53 | frame 412
  - src/effects/balloon.c: 5 0 0 2 | fxtest 69 | frame 456
  - src/effects/big_snowflakes.c: 4 0 0 2 | fxtest 42 | frame 500
  - src/effects/blast.c: 6 1 0 3 | fxtest 56 | frame 544
  - src/effects/bombette_breaking.c: 9 0 0 3 | fxtest 60 | frame 588
  - src/effects/butterflies.c: 6 0 1 3 | fxtest 79 | frame 632
  - src/effects/chomp_drop.c: 7 0 1 6 | fxtest 71 | frame 676
  - src/effects/cold_breath.c: 12 0 2 2 | fxtest 104 | frame 720
  - src/effects/confetti.c: 9 0 3 2 | fxtest 62 | frame 764
  - src/effects/debuff.c: 9 0 0 2 | fxtest 43 | frame 808
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. `drec=412:10:44` records those frames; `fncov=412:428` counts the calls of the whole window (COVMAP2).
- gold.py <row> builds the fncov lane, runs the fncov benches of bench.d/<row>.json and runs covsites.py over cov.d/<row>.txt.
Start: `sh 3ds/tools/wt_new.sh CODEMODB2`. PM_ROW=CODEMODB2 PM_LANE=CODEMODB2 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/aura.c, src/effects/balloon.c, src/effects/big_snowflakes.c, src/effects/blast.c, src/effects/bombette_breaking.c, src/effects/butterflies.c, src/effects/chomp_drop.c, src/effects/cold_breath.c, src/effects/confetti.c, src/effects/debuff.c. New azt.d/CODEMODB2.json, bench.d/CODEMODB2.json and cov.d/CODEMODB2.txt.
New names: codemodb2_drec, codemodb2_fncov. Host link: none.
## Exact edits
1. New azt.d/CODEMODB2.json with two copies of the fxpic_89 scenario (azt_scenarios.json; first_marker and warp event
   `warp: mac_01 entry 0`; needs_picture false; cap 45, deadline 40):
   - codemodb2_drec: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=53,69,42,56,60,79,71,104,62,43 loglevel=debug det drec=412:10:44`, until
     `drec f808 draws [1-9]\d* bytes \d+`.
   - codemodb2_fncov: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=53,69,42,56,60,79,71,104,62,43 fncov=412:428 loglevel=debug det`, until `fncov f\d+ done`.
   New bench.d/CODEMODB2.json: codemodb2_drec (kind azahar_pic, tags azahar det drec) and codemodb2_fncov (kind azahar_stream, tags azahar det
   fncov). New cov.d/CODEMODB2.txt: the 10 paths, one per line. Commit them.
2. `python 3ds/tools/codemod.py src/effects/aura.c src/effects/balloon.c src/effects/big_snowflakes.c src/effects/blast.c src/effects/bombette_breaking.c src/effects/butterflies.c src/effects/chomp_drop.c src/effects/cold_breath.c src/effects/confetti.c src/effects/debuff.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB2: codemod batch of 10 effect files (77 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/aura.c src/effects/balloon.c src/effects/big_snowflakes.c src/effects/blast.c src/effects/bombette_breaking.c src/effects/butterflies.c src/effects/chomp_drop.c src/effects/cold_breath.c src/effects/confetti.c src/effects/debuff.c`: value 0 for each file; keep, state and dl as in the table.
2. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB2 --why "CODEMODB2"`: PASS (the host unit tests).
3. Coverage: `python 3ds/tools/gold.py CODEMODB2` prints `pass`. Then `python 3ds/tools/covsites.py build3ds/out/CODEMODB2_codemodb2_fncov.log
   --lane CODEMODB2 src/effects/aura.c src/effects/balloon.c src/effects/big_snowflakes.c src/effects/blast.c src/effects/bombette_breaking.c src/effects/butterflies.c src/effects/chomp_drop.c src/effects/cold_breath.c src/effects/confetti.c src/effects/debuff.c`: each file `sites h/t` with 10 h >= 9 t. A file below that: revert its edits, take it out of cov.d and of
   the scene ids (fix it in the scenes too), and name it in the hand-back; the gate goes on with the others.
4. `bench.py base codemodb2_drec` (merge-base build), `bench.py run codemodb2_drec --row CODEMODB2 --why "CODEMODB2 drec gate"`, `bench.py cmp
   codemodb2_drec --row CODEMODB2`: drec.txt byte-equal.
5. One seeded fault for the batch (a value change, then undo): in src/effects/aura.c:307 add ` + 20.0f` after `data->posB.y` (the first transform of its draw function, row_SEEDS.md); it draws at frame 412. The ELF sha1 must differ from the real edit. Run
   codemodb2_drec again and cmp: it must differ.
6. Budget 8 Azahar starts. Counter: C1 -77, C2 -77.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
