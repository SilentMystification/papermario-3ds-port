# Row CODEMODW3: codemod run on src/world/area_flo/flo_10/waterfx.c (7 value calls to native records), new scene spot_flo10_drec

Missing fact: no stored scene is known to draw this file. This row adds spot_flo10_drec. If the seed shows no difference, the drawing
is not drawn in frames 200-201: stop and hand back. The oracle then gives a position or a story value.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODW1.)
## Facts (ORACLE8, main cff324713)
- `python 3ds/tools/codemod.py --dry src/world/area_flo/flo_10/waterfx.c`: `value 7 keep 0 state 21 dl 1`. The value calls: 3 gDPSetPrimColor (:71 and :87 `255, 255, 255, alpha`, :234 `248, 240, 240, 0`) and 4 gDPPipeSync (:235, :238, :251, :255). :237 gSPDisplayList stays (no-entry).
  listcen: build 29, gfxpos 29.
- flo_10 main.c:29 runs N(EVS_SetupWaterEffect) (:10). The :71 and :87 calls sit inside N(UnkModelFunc000) loops.
- The file has no TARGET_3DS line (N64DEL2 does not edit it). Its first include is `#include "flo_10.h"`.
- Only this scene runs the file. spot_mac01_det and spot_mac01_fps60_det do not, so they are not gates (CLAUDE.md 2).
Start: `sh 3ds/tools/wt_new.sh CODEMODW3`. PM_ROW=CODEMODW3 PM_LANE=CODEMODW3 on every call.
## Touch only (places)
src/world/area_flo/flo_10/waterfx.c (the 7 value call lines, the include lines). New 3ds/tools/azt.d/CODEMODW3.json and 3ds/tools/bench.d/CODEMODW3.json.
## Exact edits
1. Commit 1 holds only the scene. azt.d/CODEMODW3.json spot_flo10_drec is a copy of the spot_mac01_det scenario. Its `debug` is
   `warp=flo_10:0 loglevel=debug det drec=200:2`. Its first_marker and warp event are `warp: flo_10 entry 0`. Its until is
   `drec f201 draws [1-9]\d* bytes \d+`. It has cap 40, deadline 35 and needs_picture false. bench.d/CODEMODW3.json: spot_flo10_drec, kind azahar_pic, tags azahar det drec, as
   spot_mac01_drec (bench.d/HOSTCORE2.json).
2. `python 3ds/tools/codemod.py src/world/area_flo/flo_10/waterfx.c`.
3. After `#include "flo_10.h"` add `#include "nd_val.h"`.
4. Commit: `CODEMODW3: codemod run on flo_10/waterfx.c (7 value calls to native records)`.
## Gate (expected values)
1. `codemod.py --dry` after the edit: `value 0 keep 0 state 21 dl 1`. listcen: build 29 -> 22, gfxpos 29 -> 22.
2. Build, memcheck OK. nd_host PASS.
3. At commit 1: `bench.py base spot_flo10_drec`. After edit 4: `bench.py run spot_flo10_drec --row CODEMODW3 --why "CODEMODW3 drec gate"` and
   `bench.py cmp spot_flo10_drec --row CODEMODW3`: drec.txt byte-equal.
4. Seeded fault (a value change, then undo): the nd_val_prim call of :71 with the last argument `alpha` -> `alpha / 2` (PM_CC_10,
   macros.h:338, set at :250: colour TEXEL0, alpha PRIMITIVE; prim RGB is not read). Before the run, the sha1sum of build3ds/lane-CODEMODW3/pm_3ds.elf must
   differ from its value at the real edit. The cmp must differ.
5. Budget 8 Azahar starts. Counter: C1 -7, C2 -7.
## Stop and end
A seed with no difference (hand back: the drawing is not drawn in frames 200-201), a cmp difference with the real edit, a base pair that
differs, or a tool fault: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
