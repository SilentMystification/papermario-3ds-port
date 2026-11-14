# ws_GEO: target, depth, vertex, triangle and light calls to native setters (C1, C2)
Model: Haiku. Design: design_lookahead.md section 1. PM_ROW, PM_LANE = GEO. Budget: 6 Azahar runs. FACT (`codemod.py --dry --geo`, main b8fcf0e17):
src/world src/battle value 26 keep 6 (5 calls in src/world/dead), root src/*.c value 78 keep 8. ws_SCOMP S3 edits src/battle/16C8E0.c after `Merge GEO2`.
## G0 (done, merge 7efc7f404): nd_geo.h/.c setters, geounit 12/12, codemod --geo (GEO_MAP codemod.py:75-88, run.sh `geo_*` cases).
## G1 effects: done (merge 870153f37, 55 calls). G2 src/world src/battle and G3 root src/*.c: start now (WSD 72162b87f and WSCR4 7f6d3d638 are merged).
Each: new branch GEO<n> from main. A group whose merge line `git log --oneline main` does not show: skip it. 1. listcen before.
`python 3ds/tools/codemod.py --dry --geo <dirs>` (G3: src/*.c): v = total value. 2. Same without --dry. Never --state, never --ntab.
3. A file with a call of nd_geo.h and no `#include "nd_geo.h"` gets it after the last #include of its top block (no #include line, part2.inc.c: line 1).
- Shapes. msg_draw.c:2004-2005 Before: `gSPVertex(gMainGfxPos++, gMsgSpeechBoxLQuad, 4, 0);` `gSP2Triangles(gMainGfxPos++, 0, 2, 1, 0, 1, 2, 3, 0);` After:
  `nd_mesh_vtx(gMsgSpeechBoxLQuad, 4, 0);` `nd_mesh_tri2(0, 2, 1, 0, 1, 2, 3, 0);`. sprite_shading.c:392 Before: `gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA,
  G_IM_SIZ_16b, SCREEN_WIDTH, SpriteShadingPalette);` After: `nd_tgt_cimg(G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, SpriteShadingPalette);`.
  item_entity.c:213 Before: `gSPSetLights1(gMainGfxPos++, ItemEntityLights);` After: `nd_lit_set1(ItemEntityLights);`. main.c:153: the line goes.
4. nd_host PASS, 1 build, listcen build and gfxpos = before - v exactly. drec (`bench.py run`, then `cmp --commit <base>`, byte-equal): G1 gold_12_drec
   (930de29e1), G2 spot_mac01_drec (930de29e1) and spot_mac00w_det (a0e0414d8), G3 spot_mac01_drec and sbk14_drec (930de29e1). Commit `GEO G<n>: ...`.
Fix rights: G0 new files, codemod.py, run.sh, cases. G1-G3 include lines only. A cmp, listcen or build difference: no commit, write build3ds/out/geo_<g>.txt,
send the path to the oracle, go on. Hand back after the last group you can run (8 lines; details build3ds/out/handback_GEO.txt).
