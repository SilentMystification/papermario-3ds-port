# Row {ID}: codemod batch of {N} effect files ({VALUES} value calls to native records), coverage and drec gate over one fxtest window

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Batch form of 2026-11-13:
host tests, coverage and one drec window per batch; no per-file seed.)
## Facts (oracle, main {MAIN})
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame):
{TABLE}
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. `drec=412:{N}:44` records those frames; `fncov=412:{SPAN}` counts the calls of the whole window (COVMAP2).
- gold.py <row> builds the fncov lane, runs the fncov benches of bench.d/<row>.json and runs covsites.py over cov.d/<row>.txt.
Start: `sh 3ds/tools/wt_new.sh {ID}`. PM_ROW={ID} PM_LANE={ID} on every call.
## Touch only (places)
The value call lines and the include lines of: {FILES}. New azt.d/{ID}.json, bench.d/{ID}.json and cov.d/{ID}.txt.
New names: {SCENE}, {FNCOV}. Host link: none.
## Exact edits
1. New azt.d/{ID}.json with two copies of the fxpic_89 scenario (azt_scenarios.json; first_marker and warp event
   `warp: mac_01 entry 0`; needs_picture false; cap 45, deadline 40):
   - {SCENE}: `debug` `warp=mac_01:0 pos=0:0:200 fxtest={IDS} loglevel=debug det drec=412:{N}:44`, until
     `drec f{LAST} draws [1-9]\d* bytes \d+`.
   - {FNCOV}: `debug` `warp=mac_01:0 pos=0:0:200 fxtest={IDS} fncov=412:{SPAN} loglevel=debug det`, until `fncov f\d+ done`.
   New bench.d/{ID}.json: {SCENE} (kind azahar_pic, tags azahar det drec) and {FNCOV} (kind azahar_stream, tags azahar det
   fncov). New cov.d/{ID}.txt: the {N} paths, one per line. Commit them.
2. `python 3ds/tools/codemod.py {PATHS}`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `{ID}: codemod batch of {N} effect files ({VALUES} value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry {PATHS}`: value 0 for each file; keep, state and dl as in the table.
2. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row {ID} --why "{ID}"`: PASS (the host unit tests).
3. Coverage: `python 3ds/tools/gold.py {ID}` prints `pass`. Then `python 3ds/tools/covsites.py build3ds/out/{ID}_{FNCOV}.log
   --lane {ID} {PATHS}`: each file `sites h/t` with 10 h >= 9 t. A file below that: revert its edits, take it out of cov.d and of
   the scene ids (fix it in the scenes too), and name it in the hand-back; the gate goes on with the others.
4. `bench.py base {SCENE}` (merge-base build), `bench.py run {SCENE} --row {ID} --why "{ID} drec gate"`, `bench.py cmp
   {SCENE} --row {ID}`: drec.txt byte-equal.
5. One seeded fault for the batch (a value change, then undo): {SEED}. The ELF sha1 must differ from the real edit. Run
   {SCENE} again and cmp: it must differ.
6. Budget 8 Azahar starts. Counter: C1 -{VALUES}, C2 -{VALUES}.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
