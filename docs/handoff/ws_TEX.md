# ws_TEX: texture, rectangle, viewport and colour calls to native setters (C1, C2)
Model: Haiku. PM_ROW, PM_LANE = TEX. Budget: 6 Azahar runs. FACT (oracle census d1bc5fa39, built src, no comments, no MAP/STATE_MAP/GEO name): 356 calls
of 34 names on gMainGfxPos++: root 232, effects 39, world 37, entity 15, filemenu 13, battle 11, pause 8, evt 1. 53 local-packet calls stay (keep first-arg).
## T0, T1: done (merge 4732694bf): nd_tx.h setters, texunit 35/35, codemod --tex (TEX_MAP codemod.py:90-127), run.sh 45/45.
## T2 effects (start now: GEO1 merged 870153f37), T3 world entity battle evt pause filemenu (after `Merge GEO2`, `Merge SCOMP` and `Merge CBALL`), T4 root src/*.c (after `Merge GEO3`)
Steps 1-4 of ws_GEO G1-G3 with --tex and `#include "nd_tx.h"`. Shapes: state_logos.c:274-275 Before `gDPSetFillColor(gMainGfxPos++, PACK_FILL_COLOR(224, 224,
224, 1));` `gDPFillRectangle(gMainGfxPos++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);` After `nd_val_fill(PACK_FILL_COLOR(224, 224, 224, 1));` `nd_rect_fill(0,
0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);`. :283-285 After (one line) `nd_tex_load_tile(gLogosImage1 + i * 0x1000, G_IM_FMT_RGBA, G_IM_SIZ_16b, 128, 112, 0, 0,
127, 15, 0, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);`. draw_box.c:774 After `nd_xf_viewport(&gBoxViewport);`. msg_draw.c:1997
`gDPTileSync(gMainGfxPos++);`: the line goes. Gate, drec bases: ws_GEO (T2 = G1, T3 = G2, T4 = G3). Commit `TEX T<n>: ...`. Fix rights: T0 new files, T1 codemod.py,
run.sh, cases, T2-T4 include lines. Any cmp, listcen or build difference: no commit, build3ds/out/tex_<t>.txt to the oracle, go on. Hand back as ws_GEO.
