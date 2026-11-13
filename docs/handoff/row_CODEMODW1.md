# Row CODEMODW1: value calls record into the open state run (nd_val.c); codemod of mac_00 water_gfx.c becomes draw-equal

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE2 2026-11-13, correction 2. Work in
build3ds/wt/CODEMODW1 on top of a2c877bf5. Do not merge main. The orchestrator resets the budget once: row fault.)
## Cause (ORACLE2 code read)
- N(gfx_build_water) is the custom gfx pre builder of MODEL_water. Its range holds NDC_STATE runs, an NDC_MODEL record
  (mdl_draw_hidden_panel_surface, the POND form) and NDC_END. The native model draw accepts the record only when
  nd_r11_walk (3ds/src/nd_model.c:274-302) reads every command of the range. It knows NDC_END, NDC_JUMP, NDC_STATE and
  NDC_MODEL. Any other header gives :296 `default: return 0;`. Then nd_model_state_in returns why 3 (:330-332), and the record
  leaves the native draw.
- Before: gDPSetPrimColor(gMainGfxPos++) went through nd_gx_w (3ds/src/nd_rec.c:442-460) to :459 `nd_rec_state(w0, w1);`: one
  NDO_PRIM unit in the open NDC_STATE run. nd_r11_op accepts NDO_PRIM (nd_model.c:267).
- After: nd_val_prim records `nd_rec_napi(nd_val_run, u, 1u);` (3ds/src/nd_val.c:14). That is an NDC_NAPI header (nd_rec.c:274),
  so nd_r11_walk returns 0 and the water takes another path. Also, rec_take (nd_rec.c:166-168) closes the state run and resets
  the shadow, so the ops after the call are no longer deduplicated as before.
- The removed gDPPipeSync calls change nothing: nd_gx_w drops a sync (nd_rec.c:457).
- Shapes with the fault: nd_val_prim or nd_val_env in a model custom gfx builder whose range nd_r11_walk accepted before
  (W1 water_gfx.c, W3 waterfx.c). In W2 splash.c and W4 fountains.c the range has a matrix op, and nd_r11_op rejects NDO_MTX
  before and after the codemod. The nd_xf calls are not changed by this row.
- Fix: nd_val.c records the same typed unit through nd_rec_state, as the redirect does. The frame stream of a codemod value
  call is then the stream of the macro, byte for byte (the unit is the one ND_X0 of nd_op.h gives). No codemod rule change.
## Touch only
Files: 3ds/src/nd_val.c (header comment, includes, val_rec, nd_val_run), 3ds/include/nd_val.h (header comment, nd_val_run
prototype), src/world/area_mac/mac_00/water_gfx.c (already edited at fc6cbc44f).
New names: none (spot_mac00w_det is in this branch). Host link: none (no host test links nd_val.c).
Needs: 3ds/src/nd_val.c has nd_rec_napi(nd_val_run, u, 1u)
Seed: 3ds/src/nd_val.c:18-20 (nd_val_prim) is reached through water_gfx.c; the seed is the water_gfx.c value in gate 3, scene
spot_mac00w_det.
## Exact edits
1. nd_val.c: add `#include "nd_cmd.h"`. val_rec body becomes `nd_rec_state(NDO_W0(id, a), b);`. Delete the nd_val_run function.
   Header comment: "Each call records its typed unit into the open NDC_STATE run (nd_rec_state), the unit the nd_gbi.h redirect
   records for the macro, so the frame stream is the one the macro gave (CODEMODW1: custom gfx ranges read by nd_r11_walk)."
2. nd_val.h: delete the nd_val_run prototype. Header comment: "One typed unit per call into the open state run
   (nd_rec_state), as the macro's redirect".
3. Commit: `CODEMODW1: value calls record their typed unit into the state run (nd_rec_state), as the gbi redirect`.
## Gate (CLAUDE.md 2: nd_host and byte compare only)
1. `sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. `bench.py run nd_host --row CODEMODW1 --why "W1 host"`: PASS.
   `python 3ds/tools/codemod.py --dry src/world/area_mac/mac_00/water_gfx.c`: `value 0 keep 0 state 7 dl 1`.
2. The base of spot_mac00w_det at the merge-base exists (stable pair). Base the second scene:
   `python 3ds/tools/bench.py base fxpic_105` (embers.c, 2 nd_val calls, merged at fc17b7aad).
3. Seed first: water_gfx.c `nd_val_prim(0, 0, 0, 0, 128, 0)` becomes `nd_val_prim(0, 0, 0, 0, 64, 0)`. Build (ELF sha1 changes).
   `bench.py run spot_mac00w_det --row CODEMODW1 --why "W1 seed"`, `bench.py cmp spot_mac00w_det --row CODEMODW1`: must differ.
   Undo the seed.
4. `bench.py run spot_mac00w_det,fxpic_105 --row CODEMODW1 --why "W1 gate"`, then `bench.py cmp` of each: every shot byte-equal.
5. Budget 8 Azahar starts after the reset (base 2, seed 1, gate 2). Counter: C1 -3, C2 -3 (water_gfx.c, as fc6cbc44f).
## Stop and end
A cmp difference with the real edit, a seed with no difference, a base pair that differs, or a tool fault: stop, commit, hand
back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
