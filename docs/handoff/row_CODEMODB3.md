# Row CODEMODB3: codemod batch of 10 effect files (76 value calls to native records), coverage and drec gate over one fxtest window

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Batch form of 2026-11-13:
host tests, coverage and one drec window per batch; no per-file seed.)
## Facts (oracle, main c2d5565fd)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame):
  - src/effects/drop_leaves.c: 4 0 0 2 | fxtest 15 | frame 412
  - src/effects/effect_46.c: 7 0 0 2 | fxtest 64 | frame 456
  - src/effects/effect_65.c: 6 0 2 1 | fxtest 95 | frame 500
  - src/effects/effect_75.c: 9 0 0 2 | fxtest 110 | frame 544
  - src/effects/effect_86.c: 7 0 0 2 | fxtest 126 | frame 588
  - src/effects/ending_decals.c: 9 0 4 2 | fxtest 46 | frame 632
  - src/effects/energy_in_out.c: 9 0 0 2 | fxtest 100 | frame 676
  - src/effects/energy_shockwave.c: 9 0 0 2 | fxtest 51 | frame 720
  - src/effects/falling_leaves.c: 4 0 0 2 | fxtest 38 | frame 764
  - src/effects/fire_flower.c: 12 0 2 4 | fxtest 57 | frame 808
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. `drec=412:10:44` records those frames; `fncov=412:428` counts the calls of the whole window (COVMAP2).
- gold.py <row> builds the fncov lane, runs the fncov benches of bench.d/<row>.json and runs covsites.py over cov.d/<row>.txt.
Start: `sh 3ds/tools/wt_new.sh CODEMODB3`. PM_ROW=CODEMODB3 PM_LANE=CODEMODB3 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/drop_leaves.c, src/effects/effect_46.c, src/effects/effect_65.c, src/effects/effect_75.c, src/effects/effect_86.c, src/effects/ending_decals.c, src/effects/energy_in_out.c, src/effects/energy_shockwave.c, src/effects/falling_leaves.c, src/effects/fire_flower.c. New azt.d/CODEMODB3.json, bench.d/CODEMODB3.json and cov.d/CODEMODB3.txt.
New names: codemodb3_drec, codemodb3_fncov. Host link: none.
## Exact edits
1. New azt.d/CODEMODB3.json with two copies of the fxpic_89 scenario (azt_scenarios.json; first_marker and warp event
   `warp: mac_01 entry 0`; needs_picture false; cap 45, deadline 40):
   - codemodb3_drec: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=15,64,95,110,126,46,100,51,38,57 loglevel=debug det drec=412:10:44`, until
     `drec f808 draws [1-9]\d* bytes \d+`.
   - codemodb3_fncov: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=15,64,95,110,126,46,100,51,38,57 fncov=412:428 loglevel=debug det`, until `fncov f\d+ done`.
   New bench.d/CODEMODB3.json: codemodb3_drec (kind azahar_pic, tags azahar det drec) and codemodb3_fncov (kind azahar_stream, tags azahar det
   fncov). New cov.d/CODEMODB3.txt: the 10 paths, one per line. Commit them.
2. `python 3ds/tools/codemod.py src/effects/drop_leaves.c src/effects/effect_46.c src/effects/effect_65.c src/effects/effect_75.c src/effects/effect_86.c src/effects/ending_decals.c src/effects/energy_in_out.c src/effects/energy_shockwave.c src/effects/falling_leaves.c src/effects/fire_flower.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB3: codemod batch of 10 effect files (76 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/drop_leaves.c src/effects/effect_46.c src/effects/effect_65.c src/effects/effect_75.c src/effects/effect_86.c src/effects/ending_decals.c src/effects/energy_in_out.c src/effects/energy_shockwave.c src/effects/falling_leaves.c src/effects/fire_flower.c`: value 0 for each file; keep, state and dl as in the table.
2. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB3 --why "CODEMODB3"`: PASS (the host unit tests).
3. Coverage: `python 3ds/tools/gold.py CODEMODB3` prints `pass`. Then `python 3ds/tools/covsites.py build3ds/out/CODEMODB3_codemodb3_fncov.log
   --lane CODEMODB3 src/effects/drop_leaves.c src/effects/effect_46.c src/effects/effect_65.c src/effects/effect_75.c src/effects/effect_86.c src/effects/ending_decals.c src/effects/energy_in_out.c src/effects/energy_shockwave.c src/effects/falling_leaves.c src/effects/fire_flower.c`: each file `sites h/t` with 10 h >= 9 t. A file below that: revert its edits, take it out of cov.d and of
   the scene ids (fix it in the scenes too), and name it in the hand-back; the gate goes on with the others.
4. `bench.py base codemodb3_drec` (merge-base build), `bench.py run codemodb3_drec --row CODEMODB3 --why "CODEMODB3 drec gate"`, `bench.py cmp
   codemodb3_drec --row CODEMODB3`: drec.txt byte-equal.
5. One seeded fault for the batch (a value change, then undo): in src/effects/drop_leaves.c:141 add ` + 20.0f` after `part->unk_08` (the first transform of its draw function, row_SEEDS.md); it draws at frame 412. The ELF sha1 must differ from the real edit. Run
   codemodb3_drec again and cmp: it must differ.
6. Budget 8 Azahar starts. Counter: C1 -76, C2 -76.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
