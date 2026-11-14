# ws_CBALL: the 7 compiled state defs of the crystal ball (mac_01) become setter calls (C1, C5, C6, C7, C10)
Model: Haiku. PM_ROW, PM_LANE = CBALL. Budget: 2 Azahar runs. Start after `Merge GEO2` shows in `git log --oneline main` (GEO G2 owns src/world).
Branch CBALL (`sh 3ds/tools/wt_new.sh CBALL`). Files: src/world/area_mac/mac_01/crystal_ball_gfx.c, 3ds/include/nd_tx.h (2 new lines), 3ds/tools/nd_tx_test.c,
3ds/tools/nd_geo_test.c, 3ds/tests/host.d/texunit.sh and geounit.sh (the count only). ws_TEX T3 waits for `Merge CBALL` (same file).
## Design (oracle, code read main e0e468ff6)
- gSPLookAtX(p, &la.l[0]) + gSPLookAtY(p, &la.l[1]) = nd_lit_lookat(&la.l[0]): Light is 16 bytes (include/PR/gbi.h:1362-1365), LookAt is Light l[2] (:1417-1419), and
  nd_lit_lookatw writes X at la and Y at la + 16 (3ds/src/nd_geo.c:46-49). No setter for X or Y alone: crystal_ball_gfx.c:76-77 is the only site (grep src include).
- gDPSetHilite1Tile/2Tile (gbi.h:2674-2686) are gDPSetTileSize with the hilite fields: new header macros next to nd_tex_tsz (nd_tx.h:27), the same expressions.
- Each nd_scomp_rec site becomes one setter per def unit in array order (the ws_SCOMP rule). Placeholder j takes the arguments of v line j without its packet.
## Group 1 (host test first, then the file, one build, one gate, one commit `CBALL: ...`)
1. nd_tx.h after the nd_tex_tsz line add:
   `#define nd_tex_hilite1(t, hl, wd, ht) nd_tex_tsz(t, (hl)->h.x1 & 0xfff, (hl)->h.y1 & 0xfff, ((((wd)-1)*4)+(hl)->h.x1) & 0xfff, ((((ht)-1)*4)+(hl)->h.y1) & 0xfff) /* gDPSetHilite1Tile */`
   and the same `nd_tex_hilite2` with x2, y2 (`/* gDPSetHilite2Tile */`). Never name a macro parameter `h`: the field `(hl)->h` would take its value.
2. nd_tx_test.c: `static Hilite ghl = { { 12, 34, 56, 78 } };` near gTimg (:58). Shape "hilite": m = `gDPSetHilite1Tile(gMainGfxPos++, G_TX_RENDERTILE, &ghl, 64, 64);
   gDPSetHilite2Tile(gMainGfxPos++, G_TX_MIRROR, &ghl, 64, 64);`, n = `nd_tex_hilite1(G_TX_RENDERTILE, &ghl, 64, 64); nd_tex_hilite2(G_TX_MIRROR, &ghl, 64, 64);`. Add it at the
   end of shapes[]. nd_geo_test.c: shape "lookatxy": m = `gSPLookAtX(gMainGfxPos++, &glk[0]); gSPLookAtY(gMainGfxPos++, &glk[1]);`, n = `nd_lit_lookat(glk);` (glk :61),
   after { "lookat", ... } (:127). Fix the counts in the comments and scripts: texunit ok 36/36, geounit ok 13/13. Run `sh 3ds/tools/nd_host_test.sh`: both seeds still caught.
3. crystal_ball_gfx.c. Before (:76-80) `gSPLookAtX(p++, &gDisplayContext->lookAt.l[0]);` `gSPLookAtY(p++, &gDisplayContext->lookAt.l[1]);` `gDPSetHilite1Tile(p++,
   G_TX_RENDERTILE, &gDisplayContext->hilite, 64, 64);` `gDPSetHilite2Tile(p++, G_TX_MIRROR, &gDisplayContext->hilite, 64, 64);` `nd_scomp_rec(&cb_pre, v, (unsigned)(p - v));`
   After `nd_lit_lookat(&gDisplayContext->lookAt.l[0]);` `nd_tex_hilite1(G_TX_RENDERTILE, &gDisplayContext->hilite, 64, 64);` `nd_tex_hilite2(G_TX_MIRROR,
   &gDisplayContext->hilite, 64, 64);` `nd_tex_on(0x1000, 0x1000, 0, G_TX_RENDERTILE, G_ON);` `nd_pso_geom_set(G_TEXTURE_GEN);`. Delete `Gfx v[4];` and `Gfx* p = v;` (:65-66).
4. :84 `nd_scomp_rec(&cb_post, NULL, 0);` -> `nd_pso_geom_clr(G_TEXTURE_GEN);`. The gSPEndDisplayList line stays (ws_TEX T3).
5. :145-149 Before `p = v1;` `gDPSetColorImage(p++, ..., osVirtualToPhysical(nuGfxZBuffer));` `gDPSetPrimColor(p++, 0, 0, 248, 240, 240, 0);` the two rec lines. After
   `nd_pso_cyc(G_CYC_1CYCLE);` `nd_pso_rm(<the two cb_r1a_u render mode arguments, :24>);` `nd_tgt_cimg(G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH,
   osVirtualToPhysical(nuGfxZBuffer));` `nd_pso_cc(PM_CC_PRIM_NO_ALPHA, PM_CC_PRIM_NO_ALPHA);` `nd_val_prim(0, 0, 248, 240, 240, 0);`.
6. :151-155 (`p = v2;` to the cb_r2b rec line) After `nd_tgt_cimg(G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, osVirtualToPhysical(nuGfxCfb_ptr));` `nd_pso_zsrc(G_ZS_PRIM);` `nd_val_primz(32734, 0);`
   `nd_pso_rm(G_RM_ZB_OPA_DECAL, G_RM_ZB_OPA_DECAL2);` `nd_pso_filt(G_TF_POINT);` `nd_pso_persp(G_TP_NONE);` `nd_tex_on(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);`
   `nd_pso_lut(G_TT_NONE);` `nd_pso_detail(G_TD_CLAMP);` `nd_pso_lod(G_TL_TILE);`. Delete `Gfx v1[2];` `Gfx v2[2];` `Gfx* p;` (:95-97).
7. :168 `nd_scomp_rec(&cb_r3, NULL, 0);` -> `nd_pso_zsrc(G_ZS_PIXEL);` `nd_pso_rm(G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2);` `nd_pso_cc(G_CC_SHADE, G_CC_SHADE);`
   `nd_pso_persp(G_TP_PERSP);` `nd_tex_on(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);`.
8. Delete :6-56 (the S6M1 comment, the 7 `static const Gfx cb_*_u[]` arrays, the 7 NdSDef lines, the S6VORD comment). `#include "nd_scomp.h"` -> `#include "nd_geo.h"`,
   `#include "nd_val.h"`, `#include "nd_tx.h"` (nd_st.h stays).
## Gate (1 build, 1 Azahar run)
nd_host PASS (texunit 36/36, geounit 13/13, the seeds caught). listcen before and after: recdl -7, build -8, data -27, data_arrays -10, gfxtype -12 (GUESS, oracle count:
27 gs lines, 7 arrays and 3 `Gfx v[n];` locals, 8 g*P calls; give the measured values). memcheck margin 310 KB or more. `bench.py base spot_mac01_drec --row CBALL`,
`bench.py run spot_mac01_drec --row CBALL --why "CBALL"`, `bench.py cmp spot_mac01_drec --row CBALL`: byte-equal. No scenario is known to draw the ball (GUESS): the
host units are the proof of the two new shapes. Commit `CBALL: ...` with the listcen lines; the commit deletes this page.
Fix rights: the six files. A host FAIL, a cmp or build difference: no commit, build3ds/out/cball.txt, send the path to the oracle (CLAUDE.md rule 5).
Hand back: 8 lines (hash, counters, cmp; details build3ds/out/handback_CBALL.txt).
