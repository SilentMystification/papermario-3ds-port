# Row CODEMODW1: codemod run on src/world/area_mac/mac_00/water_gfx.c (3 value calls to native records), the first world file

PARKED (ORACLE8, 2026-11-13). The real edit changed the picture (fc6cbc44f: 4/4 shots differ in one band). No cause is read yet.
Trace (code read): the builder N(gfx_build_water) runs inside build_custom_gfx (model.c:4319-4329), between nd_rec_mark and nd_rec_end
(nd_cgfx_one, model.c:4309-4318). The range replays at the model draw through nd_range_run (nd_walk.c:608, nd_stream_run). Before:
gDPSetPrimColor went through the redirect (rec_op_t, nd_rec.c:351: the state shadow sh_drop, then st_put into the open NDC_STATE
run). After: nd_val_prim writes one NDC_NAPI record (nd_rec.c:269). rec_take closes the open state run and calls sh_all, which
resets the shadow (nd_rec.c:166-168). The walker side runs the same setter (nd_set_prim, nd_set.c:74, with sync_combine). The
removed gDPPipeSync were dropped by the redirect before too (nd_rec.c:371). So the difference is in how the range replays a
NAPI record inside an NDC_STATE sequence, or in the shadow reset. The call shape: a value call inside a model custom gfx builder
(SetCustomGfxBuilders). W2, W3 and W4 have the same shape, so they are parked too. Effect files (render tasks) do not have it:
FX1, FX3 and FX7 kept byte or drec equality with the real edit. The orchestrator asks the user before more work on this shape.


Correction 1 (ORACLE8, 2026-11-13): spot_mac00_det exists (azt_scenarios.json:11278, an INTERIOR scene at -135 0 85; it is not
known to show the water). The row scene is renamed spot_mac00w_drec. On branch CODEMODW1 make one new commit that renames the name in
azt.d/CODEMODW1.json and bench.d/CODEMODW1.json (no amend). Base spot_mac00w_drec at the merge-base (no new switch). Missing fact:
no stored scene is known to draw MODEL_water. This row adds spot_mac00w_drec. If the seed shows no difference, the
water is not drawn in frames 200-201: stop and hand back. The oracle then gives a position.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODFX1, merged 177ac50be.)
## Facts (ORACLE8, main 0250d5221)
- `python 3ds/tools/codemod.py --dry src/world/area_mac/mac_00/water_gfx.c`: `value 3 keep 0 state 7 dl 1`. The 3 value calls
  are :14 gDPSetPrimColor (0, 0, 0, 0, 128, 0), :15 gDPPipeSync and :20 gDPPipeSync. The :19 gSPDisplayList stays (no-entry).
  listcen: build 11, gfxpos 12.
- The file has no TARGET_3DS line (it is not an N64DEL2 place). Its includes are `#include "mac_00.h"` and `#include "model.h"`.
- N(gfx_build_water) is the CUSTOM_GFX_0 builder of MODEL_water (:28-31). mac_00 main.c:96 starts it on every mac_00 load.
- The mac_00 scenes in azt_scenarios.json (bg_edge_mac00 and others) use warp=mac_00:0 pos=-57:0:76 and a padscript, and they
  are not det. This row adds a still det scene.
- spot_mac01_det and spot_mac01_fps60_det do not run mac_00 code, so they are not gates of this row (CLAUDE.md 2: only scenes
  that use the changed code).
Start: `sh 3ds/tools/wt_new.sh CODEMODW1`. PM_ROW=CODEMODW1 PM_LANE=CODEMODW1 on every call.
## Touch only (places)
src/world/area_mac/mac_00/water_gfx.c (the 3 value call lines, 2 include lines). New 3ds/tools/azt.d/CODEMODW1.json and
new 3ds/tools/bench.d/CODEMODW1.json.
## Exact edits
1. Commit 1 holds only the scene. azt.d/CODEMODW1.json spot_mac00w_drec is a copy of the spot_mac01_det scenario. Its `debug` is
   `warp=mac_00:0 pos=-57:0:76 loglevel=debug det drec=200:2`. Its first_marker and warp event are
   `warp: mac_00 entry 0`. Its until is `drec f201 draws [1-9]\d* bytes \d+`. It has cap 40, deadline 35 and needs_picture false.
   bench.d/CODEMODW1.json: spot_mac00w_drec, kind azahar_pic, tags azahar det drec, as spot_mac01_drec (bench.d/HOSTCORE2.json).
2. `python 3ds/tools/codemod.py src/world/area_mac/mac_00/water_gfx.c`.
3. After `#include "model.h"` add `#include "nd_val.h"`. nd_xf.h is not needed: the file has no matrix call.
4. Commit: `CODEMODW1: codemod run on mac_00 water_gfx.c (3 value calls to native records)`.
## Gate (expected values)
1. `codemod.py --dry` after the edit: `value 0 keep 0 state 7 dl 1`. listcen --group src/world/area_mac/mac_00: water_gfx.c
   build 11 -> 8, gfxpos 12 -> 9.
2. Build, memcheck OK. nd_host PASS.
3. At commit 1: `bench.py base spot_mac00w_drec`. After edit 4: `bench.py run spot_mac00w_drec --row CODEMODW1 --why "CODEMODW1
   drec gate"` and `bench.py cmp spot_mac00w_drec --row CODEMODW1`: drec.txt byte-equal.
4. Seeded fault (a value change, then undo): the nd_val_prim call of :14 with its fifth argument 128 -> 64 (blue). The combine
   PM_CC_PRIM_NO_ALPHA (macros.h:329) gives the prim RGB, and the cycle 2 blender GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)
   (:12) writes that colour. The alpha is not read. The ELF sha1 must differ. The cmp must differ.
5. Budget 8 Azahar starts. Counter: C1 -3, C2 -3.
## Stop and end
A seed with no difference (hand back: the water is not drawn in frames 200-201), a cmp difference with the real edit, a base pair that
differs, or a tool fault: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
