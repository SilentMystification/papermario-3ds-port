# Row CODEMODB8: codemod batch of 9 effect files (90 value calls to native records), coverage and drec gate over one fxtest window

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Batch form of 2026-11-13:
host tests, coverage and one drec window per batch; no per-file seed.)
## Facts (oracle, main c2d5565fd)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame):
  - src/effects/waterfall.c: 6 0 1 1 | fxtest 85 | frame 412
  - src/effects/energy_orb_wave.c: 10 0 5 5 | fxtest 76 | frame 456
  - src/effects/fire_breath.c: 10 0 0 4 | fxtest 49 | frame 500
  - src/effects/flame.c: 8 0 4 4 | fxtest 27 | frame 544
  - src/effects/radial_shimmer.c: 11 0 0 2 | fxtest 45 | frame 588
  - src/effects/shape_spell.c: 10 0 1 1 | fxtest 13 | frame 632
  - src/effects/smoke_ring.c: 12 0 0 2 | fxtest 18 | frame 676
  - src/effects/star_outline.c: 10 0 6 4 | fxtest 125 | frame 720
  - src/effects/water_fountain.c: 13 0 0 3 | fxtest 86 | frame 764
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. `drec=412:9:44` records those frames; `fncov=412:384` counts the calls of the whole window (COVMAP2).
- gold.py <row> builds the fncov lane, runs the fncov benches of bench.d/<row>.json and runs covsites.py over cov.d/<row>.txt.
Start: `sh 3ds/tools/wt_new.sh CODEMODB8`. PM_ROW=CODEMODB8 PM_LANE=CODEMODB8 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/waterfall.c, src/effects/energy_orb_wave.c, src/effects/fire_breath.c, src/effects/flame.c, src/effects/radial_shimmer.c, src/effects/shape_spell.c, src/effects/smoke_ring.c, src/effects/star_outline.c, src/effects/water_fountain.c. New azt.d/CODEMODB8.json, bench.d/CODEMODB8.json and cov.d/CODEMODB8.txt.
New names: codemodb8_drec, codemodb8_fncov. Host link: none.
## Exact edits
1. New azt.d/CODEMODB8.json with two copies of the fxpic_89 scenario (azt_scenarios.json; first_marker and warp event
   `warp: mac_01 entry 0`; needs_picture false; cap 45, deadline 40):
   - codemodb8_drec: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=85,76,49,27,45,13,18,125,86 loglevel=debug det drec=412:9:44`, until
     `drec f764 draws [1-9]\d* bytes \d+`.
   - codemodb8_fncov: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=85,76,49,27,45,13,18,125,86 fncov=412:384 loglevel=debug det`, until `fncov f\d+ done`.
   New bench.d/CODEMODB8.json: codemodb8_drec (kind azahar_pic, tags azahar det drec) and codemodb8_fncov (kind azahar_stream, tags azahar det
   fncov). New cov.d/CODEMODB8.txt: the 9 paths, one per line. Commit them.
2. `python 3ds/tools/codemod.py src/effects/waterfall.c src/effects/energy_orb_wave.c src/effects/fire_breath.c src/effects/flame.c src/effects/radial_shimmer.c src/effects/shape_spell.c src/effects/smoke_ring.c src/effects/star_outline.c src/effects/water_fountain.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB8: codemod batch of 9 effect files (90 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/waterfall.c src/effects/energy_orb_wave.c src/effects/fire_breath.c src/effects/flame.c src/effects/radial_shimmer.c src/effects/shape_spell.c src/effects/smoke_ring.c src/effects/star_outline.c src/effects/water_fountain.c`: value 0 for each file; keep, state and dl as in the table.
2. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB8 --why "CODEMODB8"`: PASS (the host unit tests).
3. Coverage: `python 3ds/tools/gold.py CODEMODB8` prints `pass`. Then `python 3ds/tools/covsites.py build3ds/out/CODEMODB8_codemodb8_fncov.log
   --lane CODEMODB8 src/effects/waterfall.c src/effects/energy_orb_wave.c src/effects/fire_breath.c src/effects/flame.c src/effects/radial_shimmer.c src/effects/shape_spell.c src/effects/smoke_ring.c src/effects/star_outline.c src/effects/water_fountain.c`: each file `sites h/t` with 10 h >= 9 t. A file below that: revert its edits, take it out of cov.d and of
   the scene ids (fix it in the scenes too), and name it in the hand-back; the gate goes on with the others.
4. `bench.py base codemodb8_drec` (merge-base build), `bench.py run codemodb8_drec --row CODEMODB8 --why "CODEMODB8 drec gate"`, `bench.py cmp
   codemodb8_drec --row CODEMODB8`: drec.txt byte-equal.
5. One seeded fault for the batch (a value change, then undo): in src/effects/waterfall.c:136 add ` + 20.0f` after `data->unk_08` (the first transform of its draw function, row_SEEDS.md); it draws at frame 412. The ELF sha1 must differ from the real edit. Run
   codemodb8_drec again and cmp: it must differ.
6. Budget 8 Azahar starts. Counter: C1 -90, C2 -90.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
