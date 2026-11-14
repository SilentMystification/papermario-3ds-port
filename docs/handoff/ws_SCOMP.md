# ws_SCOMP: the compiled state defs of entity blocks, hos model fx and the tattle camera become setter calls (C1, C5, C6, C7, C10)
Model: Haiku. PM_ROW, PM_LANE = SCOMP. Budget: 6 Azahar runs. Branch SCOMP (`sh 3ds/tools/wt_new.sh SCOMP`), start now. Files (no other workstream
edits them before `Merge SCOMP`): src/entity/Block.c, ItemBlock.c, SaveBlock.c, ShatteringBlock_common.c, WoodenCrate.c, src/world/area_hos/hos_01/model_fx.c,
src/world/area_hos/hos_05/model_fx.c, src/battle/16C8E0.c (S3 after GEO2). FACT (grep, main b8fcf0e17): 18 nd_scomp_rec sites (entity 10, hos 5, battle 3). S1+S2 files:
12 def arrays with 36 gs lines, 13 value lines, 11 `Gfx v[n];` lines: listcen recdl -15, build -13, data -36, data_arrays -23, gfxtype -23.
Not in this page: mac_01/crystal_ball_gfx.c (7 sites): gsSPLookAtX/Y alone and gDPSetHilite1Tile/2Tile have no map entry (oracle design next).
## The rule (one site)
A site `nd_scomp_rec(&D, v, n)` runs the units of D's array in order. A placeholder unit takes the words of a v line: the j-th v line fills the j-th def unit
of the same macro name (gSPViewport -> gsSPViewport(0), gDPSetPrimColor -> gsDPSetPrimColor(0, 0, 0, 0, 0, 0)). Write one setter call per unit, in array
order. The setter of `gsXX(args)` is the codemod.py entry of `gXX` (MAP, STATE_MAP, GEO_MAP, TEX_MAP, :38-127), args as its index tuple picks them (1 = the first after the packet), e.g. gsDPSetTextureLUT(x)
-> nd_pso_lut(x), gsSPTexture(s, t, l, tile, on) -> nd_tex_on(s, t, l, tile, on), gsDPSetRenderMode(a, b) -> nd_pso_rm(a, b), gsDPSetCombineMode(a, b) ->
nd_pso_cc(a, b), gsDPSetPrimColor -> nd_val_prim, gsSPMatrix(m, f) -> nd_xf_push(m, f). An entry with None (a sync): no call. A placeholder setter takes the
arguments of its v line without the first one (`&v[j]` or `p++`). Then delete the `Gfx v[n];` and `Gfx* p = v;` lines, the v lines, the rec call, and (when no
other line names them) the `static const Gfx X[] = {...};` and `static NdSDef D = {...};` lines and their S6E2, S6M6 or S5BTL1 comment line. A gs macro with
no map entry: leave that site, list it in the hand-back. Replace `#include "nd_scomp.h"` with the headers of the setters you wrote (nd_st.h nd_pso_* and
nd_tex_on, nd_val.h nd_val_*, nd_xf.h nd_xf_push, nd_geo.h nd_tgt_cimg and nd_xf_clip, nd_tx.h the TEX_MAP names), one per line, each only once per file.
- Block.c:53-60 Before: `if (entity->alpha >= 255) {` `nd_scomp_rec(&Block_def_o, NULL, 0);` `} else {` `Gfx v[1];` `gDPSetPrimColor(&v[0], 0, 0, 0, 0, 0,
  entity->alpha);` `nd_scomp_rec(&Block_def_t, v, 1);` `}` After: `if (entity->alpha >= 255) {` `nd_pso_lut(G_TT_NONE);` `nd_tex_on(-1, -1, 0, G_TX_RENDERTILE,
  G_ON);` `nd_pso_rm(G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);` `nd_pso_cc(G_CC_MODULATEIA, G_CC_MODULATEIA);` `} else {` `nd_pso_lut(G_TT_NONE);` `nd_tex_on(-1,
  -1, 0, G_TX_RENDERTILE, G_ON);` `nd_pso_cc(PM_CC_01, PM_CC_02);` `nd_val_prim(0, 0, 0, 0, 0, entity->alpha);` `}` (arrays Block_st_o :35-40, Block_st_t :42-47).
- SaveBlock.c:56-62 Before: `Gfx v[2];` `gSPMatrix(&v[0], &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);`
  `gDPSetPrimColor(&v[1], 0, 0, 0, 0, 0, alpha);` `nd_scomp_rec(&SaveBlock_def_a, v, 2);` After: `nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++],
  G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);` `nd_pso_rm(G_RM_ZB_CLD_SURF, G_RM_ZB_CLD_SURF2);` `nd_pso_cc(PM_CC_01, PM_CC_02);` `nd_val_prim(0, 0, 0, 0, 0, alpha);`.
  Keep the enclosing `{` `}` of the block when it holds other lines, else delete it too.
- hos_01/model_fx.c:44-47 Before: `Gfx v[1];` `gDPSetPrimColor(&v[0], 0, 0, 0, 0, 0, N(GlowingStarAlpha));` `nd_scomp_rec(&N(cc05), v, 1);` After:
  `nd_pso_cc(PM_CC_05, PM_CC_05);` `nd_val_prim(0, 0, 0, 0, 0, N(GlowingStarAlpha));` (array N(cc05_u) :9, def :10).
## Groups (each: edit, nd_host, one build, one gate, one commit)
S1 src/entity (5 files, 10 sites). S2 src/world/area_hos (2 files, 5 sites). S3 src/battle/16C8E0.c (3 sites, :816, :871-872): only after `Merge GEO2` shows in `git log --oneline main` (GEO G2 edits :875, :883), else skip S3. Per group: `python 3ds/tools/listcen.py` before and after: the five counters move
by the group's share of the numbers above (S1: recdl -10, S2: recdl -5, S3: recdl -3; count the other four from your diff with the listcen regexes, listcen.py:42-47).
Gate S1: `bench.py base ent_kmr03_det --row SCOMP`, then `bench.py run ent_kmr03_det --row SCOMP --why "SCOMP S1"`, `bench.py cmp ent_kmr03_det --row SCOMP`:
6/6 byte-equal. Gate S2: the same with sprshade_hos01_pic. Gate S3: battle_wheel_nowide_det (the S5BTL1 gate of these defs). memcheck margin 310 KB or more. nd_host PASS (stunit proves the setter words equal the macro words).
No scene draws SaveBlock, WoodenCrate or hos_05 (FACT: no scenario warps to their maps). Their edits use the same six setters as the covered files.
Commit `SCOMP S<n>: ...` with the listcen lines and the cmp result. Fix rights: the 8 files. A cmp, listcen or build difference: no commit,
write build3ds/out/scomp_<g>.txt, send the path to the oracle, go on. Hand back: 8 lines (hashes, counters, cmp results; details build3ds/out/handback_SCOMP.txt).
