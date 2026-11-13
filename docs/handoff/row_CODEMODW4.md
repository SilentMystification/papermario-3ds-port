# Row CODEMODW4: codemod run on src/world/area_hos/hos_04/fountains.c (2 value calls to native records), new scene spot_hos04_drec

Missing fact: no stored scene is known to draw this file. This row adds spot_hos04_drec. If the seed shows no difference, the drawing
is not drawn in frames 200-201: stop and hand back. The oracle then gives a position or a story value.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODW1.)
## Facts (ORACLE8, main cff324713)
- `python 3ds/tools/codemod.py --dry src/world/area_hos/hos_04/fountains.c`: `value 2 keep 0 state 0 dl 0`. The value calls: the 2 gSPMatrix calls of N(setup_gfx_fountain_water) (:9) and N(setup_gfx_fountain_stars) (:19).
  listcen: build 2, gfxpos 2.
- They are the CUSTOM_GFX_0 and CUSTOM_GFX_1 builders (:47-48). hos_04 main.c:115 runs N(EVS_SetupFountains).
- The file has no TARGET_3DS line (N64DEL2 does not edit it). Its first include is `#include "hos_04.h"`.
- Only this scene runs the file. spot_mac01_det and spot_mac01_fps60_det do not, so they are not gates (CLAUDE.md 2).
Start: `sh 3ds/tools/wt_new.sh CODEMODW4`. PM_ROW=CODEMODW4 PM_LANE=CODEMODW4 on every call.
## Touch only (places)
src/world/area_hos/hos_04/fountains.c (the 2 value call lines, the include lines). New 3ds/tools/azt.d/CODEMODW4.json and 3ds/tools/bench.d/CODEMODW4.json.
## Exact edits
1. Commit 1 holds only the scene. azt.d/CODEMODW4.json spot_hos04_drec is a copy of the spot_mac01_det scenario. Its `debug` is
   `warp=hos_04:0 loglevel=debug det drec=200:2`. Its first_marker and warp event are `warp: hos_04 entry 0`. Its until is
   `drec f201 draws [1-9]\d* bytes \d+`. It has cap 40, deadline 35 and needs_picture false. bench.d/CODEMODW4.json: spot_hos04_drec, kind azahar_pic, tags azahar det drec, as
   spot_mac01_drec (bench.d/HOSTCORE2.json).
2. `python 3ds/tools/codemod.py src/world/area_hos/hos_04/fountains.c`.
3. After `#include "hos_04.h"` add `#include "nd_xf.h"`.
4. Commit: `CODEMODW4: codemod run on hos_04/fountains.c (2 value calls to native records)`.
## Gate (expected values)
1. `codemod.py --dry` after the edit: `value 0 keep 0 state 0 dl 0`. listcen: build 2 -> 0, gfxpos 2 -> 0.
2. Build, memcheck OK. nd_host PASS.
3. At commit 1: `bench.py base spot_hos04_drec`. After edit 4: `bench.py run spot_hos04_drec --row CODEMODW4 --why "CODEMODW4 drec gate"` and
   `bench.py cmp spot_hos04_drec --row CODEMODW4`: drec.txt byte-equal.
4. Seeded fault (a value change, then undo): fountains.c:8 `guScale(&...matrixStack[...], (temp_f0 * 0.3) + 0.5, temp_f0,
   (temp_f0 * 0.3) + 0.5)` with the y argument `temp_f0 * 2.0f` (the transform of the fountain water; a LOAD/MUL flip can be
   dead, row_TEMPLATE.md). The ELF sha1 must differ from the real edit. cmp spot_hos04_drec must show a difference.
5. Budget 8 Azahar starts. Counter: C1 -2, C2 -2.
## Stop and end
A seed with no difference (hand back: the drawing is not drawn in frames 200-201), a cmp difference with the real edit, a base pair that
differs, or a tool fault: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
