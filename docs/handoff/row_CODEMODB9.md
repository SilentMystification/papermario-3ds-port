# Row CODEMODB9: codemod batch of 9 effect files (74 value calls to native records), coverage and drec gate over one fxtest window

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Batch form of 2026-11-13:
host tests, coverage and one drec window per batch; no per-file seed.)
## Facts (oracle, main c2d5565fd)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame):
  - src/effects/windy_leaves.c: 4 0 0 2 | fxtest 37 | frame 412
  - src/effects/attack_result_text.c: 9 0 4 5 | fxtest 66 | frame 456
  - src/effects/breaking_junk.c: 7 0 0 2 | fxtest 118 | frame 500
  - src/effects/cloud_puff.c: 6 0 0 2 | fxtest 5 | frame 544
  - src/effects/cloud_trail.c: 6 0 0 2 | fxtest 6 | frame 588
  - src/effects/damage_stars.c: 7 0 0 2 | fxtest 19 | frame 632
  - src/effects/disable_x.c: 13 0 4 11 | fxtest 59 | frame 676
  - src/effects/effect_63.c: 9 0 0 2 | fxtest 93 | frame 720
  - src/effects/emote.c: 13 0 0 4 | fxtest 11 | frame 764
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. `drec=412:9:44` records those frames; `fncov=412:384` counts the calls of the whole window (COVMAP2).
- gold.py <row> builds the fncov lane, runs the fncov benches of bench.d/<row>.json and runs covsites.py over cov.d/<row>.txt.
Start: `sh 3ds/tools/wt_new.sh CODEMODB9`. PM_ROW=CODEMODB9 PM_LANE=CODEMODB9 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/windy_leaves.c, src/effects/attack_result_text.c, src/effects/breaking_junk.c, src/effects/cloud_puff.c, src/effects/cloud_trail.c, src/effects/damage_stars.c, src/effects/disable_x.c, src/effects/effect_63.c, src/effects/emote.c. New azt.d/CODEMODB9.json, bench.d/CODEMODB9.json and cov.d/CODEMODB9.txt.
New names: codemodb9_drec, codemodb9_fncov. Host link: none.
## Exact edits
1. New azt.d/CODEMODB9.json with two copies of the fxpic_89 scenario (azt_scenarios.json; first_marker and warp event
   `warp: mac_01 entry 0`; needs_picture false; cap 45, deadline 40):
   - codemodb9_drec: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=37,66,118,5,6,19,59,93,11 loglevel=debug det drec=412:9:44`, until
     `drec f764 draws [1-9]\d* bytes \d+`.
   - codemodb9_fncov: `debug` `warp=mac_01:0 pos=0:0:200 fxtest=37,66,118,5,6,19,59,93,11 fncov=412:384 loglevel=debug det`, until `fncov f\d+ done`.
   New bench.d/CODEMODB9.json: codemodb9_drec (kind azahar_pic, tags azahar det drec) and codemodb9_fncov (kind azahar_stream, tags azahar det
   fncov). New cov.d/CODEMODB9.txt: the 9 paths, one per line. Commit them.
2. `python 3ds/tools/codemod.py src/effects/windy_leaves.c src/effects/attack_result_text.c src/effects/breaking_junk.c src/effects/cloud_puff.c src/effects/cloud_trail.c src/effects/damage_stars.c src/effects/disable_x.c src/effects/effect_63.c src/effects/emote.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB9: codemod batch of 9 effect files (74 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/windy_leaves.c src/effects/attack_result_text.c src/effects/breaking_junk.c src/effects/cloud_puff.c src/effects/cloud_trail.c src/effects/damage_stars.c src/effects/disable_x.c src/effects/effect_63.c src/effects/emote.c`: value 0 for each file; keep, state and dl as in the table.
2. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB9 --why "CODEMODB9"`: PASS (the host unit tests).
3. Coverage: `python 3ds/tools/gold.py CODEMODB9` prints `pass`. Then `python 3ds/tools/covsites.py build3ds/out/CODEMODB9_codemodb9_fncov.log
   --lane CODEMODB9 src/effects/windy_leaves.c src/effects/attack_result_text.c src/effects/breaking_junk.c src/effects/cloud_puff.c src/effects/cloud_trail.c src/effects/damage_stars.c src/effects/disable_x.c src/effects/effect_63.c src/effects/emote.c`: each file `sites h/t` with 10 h >= 9 t. A file below that: revert its edits, take it out of cov.d and of
   the scene ids (fix it in the scenes too), and name it in the hand-back; the gate goes on with the others.
4. `bench.py base codemodb9_drec` (merge-base build), `bench.py run codemodb9_drec --row CODEMODB9 --why "CODEMODB9 drec gate"`, `bench.py cmp
   codemodb9_drec --row CODEMODB9`: drec.txt byte-equal.
5. One seeded fault for the batch (a value change, then undo): in src/effects/windy_leaves.c:248 add ` + 20.0f` after `part->unk_04.y` (the first transform of its draw function, row_SEEDS.md); it draws at frame 412. The ELF sha1 must differ from the real edit. Run
   codemodb9_drec again and cmp: it must differ.
6. Budget 8 Azahar starts. Counter: C1 -74, C2 -74.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
