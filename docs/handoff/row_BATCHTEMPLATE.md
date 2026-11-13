# Row {ID}: codemod batch of {N} effect files ({VALUES} value calls to native records), drec gate over one fxtest window

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Written by mkrow.py from
row_BATCHTEMPLATE.md and a seeds table; every seed has the verdict live from a probe row.)
## Facts (mkrow.py, main {MAIN})
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame | live seed):
{TABLE}
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. drec=412:{N}:44 records those frames (DRECSTEP frame lines).
Start: `sh 3ds/tools/wt_new.sh {ID}`. PM_ROW={ID} PM_LANE={ID} on every call.
## Touch only (places)
The value call lines and the include lines of: {FILES}. New azt.d/{ID}.json and bench.d/{ID}.json.
New names: {SCENE}. Host link: none.
{NEEDS}
## Exact edits
1. azt.d/{ID}.json {SCENE}: a copy of spot_mac01_drec (azt.d/HOSTCORE2.json) with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest={IDS} loglevel=debug det drec=412:{N}:44`, until `drec f{LAST} draws [1-9]\d* bytes \d+`,
   cap 45, deadline 40, needs_picture false. bench.d/{ID}.json: {SCENE}, kind azahar_pic, tags azahar det drec. Commit them.
2. `python 3ds/tools/codemod.py {PATHS}`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `{ID}: codemod batch of {N} effect files ({VALUES} value calls to native records)`.
## Gate (expected values)
0. Draw proof, from the log of the base run of step 4: for each id, `fxtest: live 1` after its spawn and `fxcenb <id> tri <T>`
   with T above the `fxcenb -1 tri` value. A file without it moves out of the batch (row_TEMPLATE.md fix rules).
1. `python 3ds/tools/codemod.py --dry {PATHS}`: value 0 for each file, keep, state and dl as in the table.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 2 and after: the build count of the batch falls by {VALUES}.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row {ID} --why "{ID}"`: PASS.
4. `bench.py base {SCENE}` (merge-base), `bench.py run {SCENE} --row {ID} --why "{ID} drec gate"`, `bench.py cmp {SCENE}
   --row {ID}`: drec.txt byte-equal.
5. Seeded fault: the seed of each file has the verdict live in its seeds table line (the Needs lines above; a probe row measured
   it with the same fxtest window before this row was written). This row runs no probe.
6. Budget 8 Azahar starts. Counter: C1 -{VALUES}, C2 -{VALUES}.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
