# Row CODEMODW2: codemod run on src/world/area_iwa/iwa_01/splash.c (4 value calls to native records), new scene spot_iwa01_det

Missing fact: no stored scene is known to draw this file. This row adds spot_iwa01_det. If the seed shows no difference, the drawing
is not in the shots: stop and hand back. The oracle then gives a position or a story value.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODW1.)
## Facts (ORACLE8, main cff324713)
- `python 3ds/tools/codemod.py --dry src/world/area_iwa/iwa_01/splash.c`: `value 4 keep 0 state 2 dl 1`. The value calls: :57 gDPSetPrimColor (0, 0, 230, 255, 255, 255), :63 gSPMatrix, :65 gSPPopMatrix, :69 gDPPipeSync. :64 gSPDisplayList stays (no-entry).
  listcen: build 7, gfxpos 7.
- N(gfx_build_splashes) is the CUSTOM_GFX_0 post builder (:97-99). iwa_01 main.c:58 runs N(EVS_MakeSplashes) at every load.
- The file has no TARGET_3DS line (N64DEL2 does not edit it). Its first include is `#include "iwa_01.h"`.
- Only this scene runs the file. spot_mac01_det and spot_mac01_fps60_det do not, so they are not gates (CLAUDE.md 2).
Start: `sh 3ds/tools/wt_new.sh CODEMODW2`. PM_ROW=CODEMODW2 PM_LANE=CODEMODW2 on every call.
## Touch only (places)
src/world/area_iwa/iwa_01/splash.c (the 4 value call lines, the include lines). New 3ds/tools/azt.d/CODEMODW2.json and 3ds/tools/bench.d/CODEMODW2.json.
## Exact edits
1. Commit 1 holds only the scene. azt.d/CODEMODW2.json spot_iwa01_det is a copy of the spot_mac01_det scenario. Its `debug` is
   `warp=iwa_01:0 shots shotwin=200:4 loglevel=debug det`. Its first_marker and warp event are `warp: iwa_01 entry 0`. Its until is
   `shot \d+ frame 203\b`. It has cap 40, deadline 35 and needs_picture true. bench.d/CODEMODW2.json: spot_iwa01_det, kind azahar_pic, as
   spot_mac01_det in bench_defs.json.
2. `python 3ds/tools/codemod.py src/world/area_iwa/iwa_01/splash.c`.
3. After `#include "iwa_01.h"` add `#include "nd_val.h"` and `#include "nd_xf.h"`.
4. Commit: `CODEMODW2: codemod run on iwa_01/splash.c (4 value calls to native records)`.
## Gate (expected values)
1. `codemod.py --dry` after the edit: `value 0 keep 0 state 2 dl 1`. listcen: build 7 -> 3, gfxpos 7 -> 3.
2. Build, memcheck OK. nd_host PASS.
3. At commit 1: `bench.py base spot_iwa01_det`. After edit 4: `bench.py run spot_iwa01_det --row CODEMODW2 --why "CODEMODW2 byte gate"` and
   `bench.py cmp spot_iwa01_det --row CODEMODW2`: every shot byte-equal.
4. Seeded fault first (a value change, then undo): in the nd_xf_push call of :63, `G_MTX_MUL` -> `G_MTX_LOAD`. The ELF sha1 must differ from the real edit. cmp must must
   show a difference. If it shows none: stop and hand back.
5. Budget 8 Azahar starts. Counter: C1 -4, C2 -4.
## Stop and end
A seed with no difference (hand back: the drawing is not in the shots), a cmp difference with the real edit, a base pair that
differs, or a tool fault: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
