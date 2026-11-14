# ws_D: state calls of src/world, src/entity, src/evt, src/battle, src/pause, src/filemenu to the nd_st setters (C1, C2)
Model: Haiku. Branch WSD (`sh 3ds/tools/wt_new.sh WSD`), PM_ROW and PM_LANE = WSD. Budget: 5 Azahar runs. Same loop as ws_C R1-R3 (merged).
FACT (`python 3ds/tools/listcen.py`, 6fbd4d6c6): C1 build 2008, C2 gfxpos 1438 (world 256/252, entity 83/76, battle 36/31, pause 35/35,
filemenu 40/44). FACT (`codemod.py --dry --state <dir>`, 6fbd4d6c6, value = rewrites): world 131, entity 39, evt 5, battle 9, pause 19,
filemenu 19 (49 files). Edit only files below these 6 directories. Never src/effects, never a root src/*.c, never --ntab.
## Groups (one commit each: `WSD D<n>: ...`)
- D1: src/world (v 131). drec: spot_mac01_drec (--commit 930de29e1) and spot_mac00w_det (--commit a0e0414d8).
- D2: src/entity src/evt (v 44). drec: sbk14_drec and spot_mac01_drec (both --commit 930de29e1).
- D3: src/battle src/pause src/filemenu (v 47). drec: spot_mac01_drec (--commit 930de29e1).
## Loop per group
1. listcen > build3ds/out/lc_d<n>_before.txt. `python 3ds/tools/codemod.py --dry --state <dirs>`: v = the sum of the `codemod total value`
   lines (each dir its own run). A v other than the group value above: write build3ds/out/wsd_<n>.txt, send it to the oracle, stop the group.
2. `python 3ds/tools/codemod.py --state <dirs>` (one run per dir).
3. Includes (codemod adds none). A file with an nd_pso_ or nd_tex_on call and no `#include "nd_st.h"` gets it after the last #include of
   its top include block. The same for nd_val.h (nd_val_ calls) and nd_xf.h (nd_xf_ calls). A file with no #include line
   (src/world/area_mac/mac_01/npc/flower_gate.inc.c, included at file scope by npc.c:1288): the include lines go on line 1.
4. `sh 3ds/tools/nd_host_test.sh` PASS (hunit, stunit), 1 build (memcheck margin 310 KB or more).
5. listcen after: build = before - v and gfxpos = before - v, exactly. recdl unchanged.
6. `python 3ds/tools/bench.py run <bench> --row WSD --why "D<n>"`, then `python 3ds/tools/bench.py cmp <bench> --row WSD --commit <hash>`
   for each drec bench of the group: byte-equal. Commit the group (changed files and runs.jsonl only).
## Edit shapes (real lines, src/world/area_flo/flo_10/waterfx.c)
- :241 Before: `gDPSetRenderMode(gMainGfxPos++, G_RM_ZB_XLU_DECAL, G_RM_ZB_XLU_DECAL2);` After: `nd_pso_rm(G_RM_ZB_XLU_DECAL, G_RM_ZB_XLU_DECAL2);`
- :244 Before: `gSPTexture(gMainGfxPos++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);` After: `nd_tex_on(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);`
- :248 Before: `gDPSetCombineMode(gMainGfxPos++, PM_CC_10, PM_CC_10);` After: `nd_pso_cc(PM_CC_10, PM_CC_10);`
- :240 `gDPSetPrimDepth(gMainGfxPos++, 32734, 0);` stays (not in STATE_MAP, codemod.py:52-72). Any `codemod keep` line stays as it is.
- Include: waterfx.c :1-3 are `#include "flo_10.h"`, `"nu/nusys.h"`, `"model.h"`. After: line 4 is `#include "nd_st.h"`.
## Fix rights
Only include lines. A build error in a rewritten line, a listcen difference or a cmp difference: no commit of that group, write
build3ds/out/wsd_<n>.txt (the command output, the file:line), send the path to the oracle, go on with the next group (CLAUDE.md 5).
## Stop and hand-back
Stop after D3. Hand-back (8 lines): 3 hashes, listcen C1 C2 per group, the cmp lines, memcheck line. Details: build3ds/out/handback_WSD.txt.
