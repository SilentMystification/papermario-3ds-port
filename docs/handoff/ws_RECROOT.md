# ws_RECROOT: the 15 root nd_scomp_rec sites become setter calls in unit order (C10 recdl 15 to 0 in root src/*.c; C5, C6, C7 fall)
Model: Haiku. PM_ROW, PM_LANE = RECROOT. Budget: 4 Azahar runs. Start after `Merge TEX4` shows in `git log --oneline main` (TEX T4 owns root src/*.c). Branch RECROOT.
Files (this page owns them until merged; NS3 waits for `Merge RECROOT` on sprite.c): src/animator.c, background_gfx.c, cam_main.c, entity_model.c, item_entity.c,
msg.c, sprite.c, status_icons.c. FACT (grep, main 1149b5513): sites animator.c:889, background_gfx.c:46-47, cam_main.c:230-231, entity_model.c:73, :74, :384, :482,
item_entity.c:977, :981, :985, msg.c:617, sprite.c:509, status_icons.c:625. The ws_SCOMP rule (merged S3, commit 52ac44959) proved this form byte-equal.
## The rule (one site at a time)
1. The units of def D = the first n entries of its array (n from `static NdSDef D = { arr, n }`, or the array size form). Expand macros (animator.c:57 AN_NODE_HEAD).
2. At the nd_scomp_rec line write one setter per unit, in unit order. The setter of gsX(args) is the codemod.py entry of gX (codemod.py:38-127: name and argument
   positions; `nd_pso_cc(a, b)` for gsDPSetCombineMode). Drop gsDPPipeSync and gsDPNoOp. Keep gsSPClipRatio (nd_xf_clip) and gsSPPerspNormalize (nd_xf_pnorm).
3. Value units: the k-th value unit of D (viewport, scissor, colour image, matrix, prim, fog colour) takes v[k] of the call. Its setter gets the arguments of the g
   macro that filled v[k]; delete that g macro line and the local `Gfx v[]`/`scv[]` when unused. A value argument not in scope at the site: keep the site, write it to
   build3ds/out/recroot_keep.txt. background_gfx.c:46 passes `&D_80074230[0]`: the value is the first unit of D_80074230 itself.
4. A ternary or index over defs becomes if/else or a switch with one setter sequence per def (sprite.c:509 `spr_st_def[si]`: case 0-3; value unit 1 only in case 1).
5. Delete each converted `_u` array and NdSDef, and a Gfx array whose last user is gone (`grep -rn "\bNAME\b" src include`). Remove `#include "nd_scomp.h"` when unused.
Example (status_icons.c:594-609, :623-625) Before: `Gfx scv[1]; gDPSetScissor(&scv[0], G_SC_NON_INTERLACE, 12, 20, SCREEN_WIDTH - 12, SCREEN_HEIGHT - 20);`
`nd_scomp_rec(&icons_range, scv, 1);` After: `nd_pso_scis(G_SC_NON_INTERLACE, 12, 20, SCREEN_WIDTH - 12, SCREEN_HEIGHT - 20); nd_pso_cyc(G_CYC_1CYCLE); nd_pso_persp(G_TP_NONE);`
`nd_pso_lod(G_TL_TILE); nd_pso_detail(G_TD_CLAMP); nd_pso_conv(G_TC_FILT); nd_pso_combkey(G_CK_NONE); nd_pso_alcmp(G_AC_NONE); nd_pso_dither(G_CD_DISABLE);`
`nd_pso_aldith(G_AD_DISABLE); nd_tex_on(-1, -1, 0, G_TX_RENDERTILE, G_ON);` (PipeSync and NoOp dropped; icons_range_u and icons_range deleted).
Example (animator.c:59-63, :888-889) After: `nd_tex_on(0, 0, 0, G_TX_RENDERTILE, G_OFF); nd_pso_lod(G_TL_TILE); nd_pso_lut(G_TT_NONE); nd_pso_geom_clr(G_LIGHTING);`
`nd_pso_geom_set(cull ? G_CULL_BACK : 0);` `if (!gAnimModelFogEnabled) nd_pso_cc(G_CC_DECALRGBA, G_CC_DECALRGBA); else nd_pso_cc(PM_CC_42, PM_CC2_MULTIPLY_SHADE);`
Add the setter headers a file lacks (nd_st.h, nd_val.h, nd_xf.h, nd_geo.h, nd_tx.h: the header that declares the setter).
## Groups (per group: nd_host PASS, 1 build, 2 runs, commit `RECROOT <g>: ...`)
G1: animator.c, background_gfx.c, cam_main.c. G2: entity_model.c, item_entity.c, msg.c, sprite.c, status_icons.c.
Gate: build rc 0, memcheck margin 310 KB or more. `bench.py run` then `bench.py cmp <bench> --row RECROOT --commit 930de29e1` byte-equal: spot_mac01_drec, gold_12_drec.
listcen before and after: recdl falls by the converted site count exactly; build, data, data_arrays, gfxtype fall; gfxpos unchanged. The G2 commit deletes this page.
Fix rights: the 8 files. A cmp difference or a host FAIL: 2 tries per named cause, then no commit, build3ds/out/recroot.txt, send the path to the oracle.
Hand back 8 lines: hashes, sites converted and kept, listcen recdl before and after, cmp results, margin.
