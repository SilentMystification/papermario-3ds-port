# ws_GEO: target, depth, vertex, triangle and light calls to native setters (C1, C2)
Model: Haiku. Design: design_lookahead.md section 1. PM_ROW, PM_LANE = GEO. Budget: 6 Azahar runs. FACT (grep `<name>(gMainGfxPos++` in src, 8fbb0c1ae):
161 calls (plan rows NTGT sites, NXF2-4, lights): effects 55, world 22 (5 in src/world/dead), battle 4, root src/*.c 80.
## G0: setters, host test, codemod --geo (branch GEO, `sh 3ds/tools/wt_new.sh GEO`; no src edit; one commit `GEO G0: ...`)
1. New 3ds/include/nd_geo.h and 3ds/src/nd_geo.c (include list of nd_st.c:4-11, nd_geo.h last). Body form of nd_st.c:13-15, one `geo_rec(w0, w1)`
   per unit = `nd_rec_state(ND_X0(w0, w1), ND_X1(w0, w1))`. The words are those of nd_gbi.h for the macro:
   nd_tgt_cimgw(fmt, siz, width, uint32_t img): ND_W0_IMG(G_SETCIMG, fmt, siz, width), img (nd_gbi.h:95). nd_pso_zsrc(src): ND_W0_SOM(G_SETOTHERMODE_L,
   G_MDSFT_ZSRCSEL, 1), src. nd_val_primz(z, dz): _SHIFTL(G_SETPRIMDEPTH, 24, 8), _SHIFTL(z, 16, 16) | _SHIFTL(dz, 0, 16) (nd_gbi.h:50, gbi.h:3154).
   nd_mesh_vtxw(uint32_t v, n, v0): ND_W0_VTX(n, v0), v (nd_gbi.h:165). nd_mesh_tri1(a, b, c, f): _SHIFTL(G_TRI1, 24, 8) | __gsSP1Triangle_w1f(a, b, c, f), 0.
   nd_mesh_tri2(a, b, c, f, d, e, g, h): _SHIFTL(G_TRI2, 24, 8) | __gsSP1Triangle_w1f(a, b, c, f), __gsSP1Triangle_w1f(d, e, g, h) (nd_gbi.h:171-176).
   nd_lit_1(uint32_t l0, uint32_t a), nd_lit_2(l0, l1, a): ND_W0_MW(G_MW_NUMLIGHT, G_MWO_NUMLIGHT), NUML(NUMLIGHTS_1 or _2), then per light k = 1, 2, ..:
   ND_W0_D2(G_MOVEMEM, sizeof(Light), G_MV_LIGHT, k * 24 + 24), pointer (gbi.h:2482, :2520-2525; the ambient is the last k). nd_lit_lookatw(uint32_t la):
   G_MVO_LOOKATX with la, then G_MVO_LOOKATY with la + 16 (gbi.h:2646-2668). nd_xf_pnorm(s): ND_W0_MW(G_MW_PERSPNORM, 0), s. nd_xf_clipw(neg, pos):
   4 units ND_W0_MW(G_MW_CLIP, G_MWO_CLIP_RNX/RNY/RPX/RPY) with neg, neg, pos, pos (gbi.h:2215-2221).
   Macros in nd_geo.h (a pointer is cast as gSetImage does): `#define nd_tgt_cimg(f, s, w, i) nd_tgt_cimgw(f, s, w, (uint32_t)(uintptr_t)(i))`, the same
   for nd_mesh_vtx(v, n, v0) and nd_lit_lookat(la), `#define nd_lit_set1(nm) nd_lit_1((uint32_t)(uintptr_t)&(nm).l[0], (uint32_t)(uintptr_t)&(nm).a)`,
   nd_lit_set2 (l[0], l[1], a), `#define nd_xf_clip(r) nd_xf_clipw(FR_NEG_##r, FR_POS_##r)`. gDPSetDepthImage: no setter. Its unit is a NOP (nd_op.h:114)
   that nd_gx_w drops (nd_rec.c:457), so codemod removes the call.
2. New 3ds/tools/nd_geo_test.c (copy nd_st_test.c; replace the m_/n_ pairs :56-91 and the table :126-132) and 3ds/tests/host.d/geounit.sh (copy
   stunit.sh with nd_geo.c; seed `sed 's/G_MDSFT_ZSRCSEL, 1)/G_MDSFT_ZSRCSEL, 2)/'` must print `geounit FAIL zsrc`). 12 shapes from real sites: cimg
   sprite_shading.c:389, zimg main.c:153 (n_zimg: empty test function), zsrc background_gfx.c:341, primz :340, vtx and tri2 msg_draw.c:2004-2005 (static
   Vtx[4]), tri1 hud_element.c:1905, lit1 item_entity.c:210, lit2 imgfx.c:1009, lookat lil_oink.c:184, pnorm draw_box.c:497, clip windows.c:376. `geounit ok 12/12`.
3. codemod.py: GEO_MAP after STATE_MAP (:52-72), tuple form of :38-48: gDPSetColorImage (5, True, 'nd_tgt_cimg', (1, 2, 3, 4)), gDPSetDepthImage (2, True,
   None, None), gDPSetDepthSource nd_pso_zsrc, gDPSetPrimDepth nd_val_primz, gSPVertex nd_mesh_vtx, gSP1Triangle nd_mesh_tri1, gSP2Triangles nd_mesh_tri2
   (9 args), gSPSetLights1 nd_lit_set1, gSPSetLights2 nd_lit_set2, gSPLookAt nd_lit_lookat, gSPPerspNormalize nd_xf_pnorm, gSPClipRatio nd_xf_clip. After
   :279-280: `elif a == '--geo': MAP.clear(); MAP.update(GEO_MAP)` (no list call is touched). :258 and :314: sum DL_NAMES only `if x in stats` / `if x in tot`.
   run.sh:21 adds `geo_*) fl=--geo ;;`. 12 cases geo_<shape> plus geo_dl (one ND_PSO_DESC, `nd_rec_dl(D_X);`, a gDPSetCycleType call: out = in, log empty).
4. Gate: nd_host_test.sh PASS (geounit, stunit), run.sh 39/39 PASS, 1 build (memcheck margin 310 KB or more), listcen C1 C2 unchanged, no Azahar run.
## G1 effects (after `Merge ENTRYFX` on main), G2 src/world src/battle (after `Merge WSD`), G3 root src/*.c (after `Merge WSCR4` with R6)
Each: new branch GEO<n> from main. A group whose merge line `git log --oneline main` does not show: skip it. 1. listcen before.
`python 3ds/tools/codemod.py --dry --geo <dirs>` (G3: src/*.c): v = total value. 2. Same without --dry. Never --state, never --ntab.
3. A file with a call of nd_geo.h and no `#include "nd_geo.h"` gets it after the last #include of its top block (no #include line, part2.inc.c: line 1).
- Shapes. msg_draw.c:2004-2005 Before: `gSPVertex(gMainGfxPos++, gMsgSpeechBoxLQuad, 4, 0);` `gSP2Triangles(gMainGfxPos++, 0, 2, 1, 0, 1, 2, 3, 0);` After:
  `nd_mesh_vtx(gMsgSpeechBoxLQuad, 4, 0);` `nd_mesh_tri2(0, 2, 1, 0, 1, 2, 3, 0);`. sprite_shading.c:389 Before: `gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA,
  G_IM_SIZ_16b, SCREEN_WIDTH, SpriteShadingPalette);` After: `nd_tgt_cimg(G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, SpriteShadingPalette);`.
  item_entity.c:210 Before: `gSPSetLights1(gMainGfxPos++, ItemEntityLights);` After: `nd_lit_set1(ItemEntityLights);`. main.c:153: the line goes.
4. nd_host PASS, 1 build, listcen build and gfxpos = before - v exactly. drec (`bench.py run`, then `cmp --commit <base>`, byte-equal): G1 gold_12_drec
   (930de29e1), G2 spot_mac01_drec (930de29e1) and spot_mac00w_det (a0e0414d8), G3 spot_mac01_drec and sbk14_drec (930de29e1). Commit `GEO G<n>: ...`.
Fix rights: G0 new files, codemod.py, run.sh, cases. G1-G3 include lines only. A cmp, listcen or build difference: no commit, write build3ds/out/geo_<g>.txt,
send the path to the oracle, go on. Hand back after the last group you can run (8 lines; details build3ds/out/handback_GEO.txt).
