# ws_GPOSREST (Haiku): the last mechanical gMainGfxPos sites become native forms (C1 C2)
Designer: the oracle (SendMessage). Worktree build3ds/wt/GPOSREST, branch GPOSREST, from main 7eb679a916 or later. PM_ROW=GPOSREST PM_LANE=GPOSREST.
## Objective and counters
- FACT (listcen, main 7eb679a916): C1 build 405, C2 gfxpos 126.
- FACT (oracle: the patch applied to a copy, listcen BUILD_RX and GFXPOS_RX): src build -11, gfxpos -29. Pass values: C1 394, C2 97. Other counters do not change.
- The patch docs/handoff/ws_GPOSREST.patch holds every edit (16 files). `git apply --check` passes on main 7eb679a916 (FACT).
## Edit shapes in the patch (real code)
1. codemod (`codemod.py --state`, `--geo`, `--tex`) on src/model.c is_point_visible, draw_box.c:697, sprite.c:218, effects/underwater.c:248, effects/water_splash.c:212.
   Before (src/model.c:3965): `gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);` After: `nd_pso_cyc(G_CYC_1CYCLE);`. gDPPipeSync (model.c:3979) is removed.
2. The G_PM_ND marks. New form NDCTL_ND / nd_mark in 3ds/include/nd_at.h, next to NDCTL_CALL. 9 sites: background_gfx.c:373, motion_blur_flame.c:161 :250,
   underwater.c:238 :268, water_splash.c:193 :226, sprite.c:470 :474. Before: `gPmNdSplash(gMainGfxPos++, 1);` After: `nd_mark(G_PM_ND_SPLASH, 1);`.
   The 4 effect and background files get `#include "nd_at.h"`. water_splash.c also gets `#include "nd_tx.h"` (nd_tex_load_mtile).
3. mdl_draw_hidden_panel_surface (src/model.c:4003) loses its `Gfx** arg0`. Every caller passed `&gMainGfxPos`, so the packet swap was a no-op.
   Before (src/entity/HiddenPanel.c:37): `mdl_draw_hidden_panel_surface(&gMainGfxPos, data->modelID);` After: `mdl_draw_hidden_panel_surface(data->modelID);`.
   Declarations: include/functions.h:812, include/effect_shims.h:23 (the shim is a linker defsym, 3ds/CMakeLists.txt:364-368).
4. Host test: 3ds/tools/nd_at_test.c gets the shapes ndsp and ndgut (gPmNd macro against NDCTL_ND). ns3unit prints `ns3unit ok 27/27`.
## Loop
1. `git apply docs/handoff/ws_GPOSREST.patch`. Commit `GPOSREST 1: native forms for the last mechanical gMainGfxPos sites`.
2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/gposrest_host.txt 2>&1`. Pass: `ND HOST TESTS PASS`, `ns3unit ok 27/27`, `lkey census ok`.
3. One build (3ds/build.sh with PM_LANE=GPOSREST). Pass: rc 0, memcheck margin above 0.
4. `python 3ds/tools/listcen.py > build3ds/out/gposrest_lc.txt`. Pass: `listcen total build 394`, `listcen total gfxpos 97`.
   If main moved, the pass values are main's values -11 and -29.
5. Gate. Benches spot_mac01_drec and fxr_drec. For each bench: `python 3ds/tools/bench.py base <b>`,
   `python 3ds/tools/bench.py run <b> --row GPOSREST --why "GPOSREST gate"`, `python 3ds/tools/bench.py cmp <b> --row GPOSREST --commit $(git merge-base HEAD main)`.
   Pass: 1/1 byte-equal for each bench. Budget: 4 Azahar starts (base runs not charged).
6. Commit `GPOSREST 2: gate` with the values. That commit deletes this page and ws_GPOSREST.patch.
## Fix scope
- A compile error in a patched file: fix it in that file (an include or a cast). Do not change the unit words of a form.
- ns3unit FAIL ndsp or ndgut: compare NDCTL_ND with ND_W0_PMND in 3ds/include/nd_gbi.h:194. Fix the form in nd_at.h only.
- A cmp diff, or any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle.
## Stop and hand-back
Stop after step 6. Hand back: commit hash, listcen build and gfxpos totals, cmp values of the 2 benches, memcheck margin.
