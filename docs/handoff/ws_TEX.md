# ws_TEX: texture, rectangle, viewport and colour calls to native setters (C1, C2)
Model: Haiku. PM_ROW, PM_LANE = TEX. Budget: 6 Azahar runs. FACT (oracle census d1bc5fa39, built src, no comments, no MAP/STATE_MAP/GEO name): 356 calls
of 34 names on gMainGfxPos++: root 232, effects 39, world 37, entity 15, filemenu 13, battle 11, pause 8, evt 1. 53 local-packet calls stay (keep first-arg).
## T0: nd_tx.h and texunit (branch TEX, `sh 3ds/tools/wt_new.sh TEX`; start now; new files only; one commit `TEX T0: ...`)
1. New 3ds/include/nd_tx.h (no .c). Includes <stdint.h>, nd_cmd.h, nd_gbi.h. `static inline void nd_tx_rec(uint32_t w0, uint32_t w1) { nd_rec_state(ND_X0(w0, w1),
   ND_X1(w0, w1)); }` (path of nd_rec.c:459). `static inline void nd_tx_rect(uint32_t w0, uint32_t w1, uint32_t h1, uint32_t h2) { const uint32_t a0 = ND_X0(w0,
   w1), a1 = ND_X1(w0, w1); nd_rec_texrect(((uint32_t)G_TEXRECT << 24) | (a0 & 0xffffffu), a1 & ~NDO_RECT_TEX, h1, h2); }` (path of nd_rec.c:450-455).
2. Every setter is a macro. Word setters call nd_tx_rec with the words of nd_gbi.h: nd_tex_img(f, s, w, i) :97 with G_SETTIMG, nd_tex_tile :111-112,
   nd_tex_ldtile(t, uls, ult, lrs, lrt) :103 with G_LOADTILE, nd_tex_tsz (same) :103 with G_SETTILESIZE, nd_tex_ldblk :119, nd_tex_ldtlut :123, nd_val_fill(d)
   :50 with G_SETFILLCOLOR, nd_val_fog / nd_val_blend(r, g, b, a) :50 with G_SETFOGCOLOR / G_SETBLENDCOLOR and the DPRGBColor word (gbi.h:3128-3131),
   nd_xf_viewport(v) :162 (ND_W0_D2(G_MOVEMEM, sizeof(Vp), G_MV_VIEWPORT, 0), gbi.h:1808), nd_pso_fogpos(min, max) :74 (G_MW_FOG, G_MWO_FOG, word of
   gbi.h:2710-2712), nd_pso_pipe(m) :22 (gbi.h:2914), nd_pso_geom_load(w) :27 with c = -1 (gbi.h:2849), nd_pso_keyr :85, nd_pso_keygb :91, nd_pso_scisf :68,
   nd_rect_fill :143, nd_rect_sfill :147-149. nd_rect_tex and nd_rect_scis call nd_tx_rect with the four words of :206-209 and :223-230 (w0, w1, then the
   gImmp1 words). Loads: copy the `#ifndef _HW_VERSION_1` body. nd_tex_load_tile gbi.h:3884-3909, _tile_4b :4018-4043, _mtile :3934-3959, _mtile_4b :4049-4074,
   _block :3375-3393, _block_4b :3682-3699, _tlut16 :4134-4143, _tlut256 :4188-4197, nd_tex_scroll_mtile2_4b gbi_custom.h:6-36, _tile_4b :39-65, _block_4b
   :67-86. In the copy: remove the parameter pkt and each `pkt, `. Rename gDPSetTextureImage nd_tex_img, gDPSetTile nd_tex_tile, gDPLoadTile nd_tex_ldtile,
   gDPLoadBlock nd_tex_ldblk, gDPLoadTLUTCmd nd_tex_ldtlut, gDPSetTileSize nd_tex_tsz. Delete the gDPLoadSync, gDPPipeSync, gDPTileSync lines. Put `;` after
   each call. Wrap in `do { ... } while (0)`. Example After: `#define nd_tex_load_tlut16(pal, dram) do { nd_tex_img(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, dram);
   nd_tex_tile(0, 0, 0, (256+(((pal)&0xf)*16)), G_TX_LOADTILE, 0 , 0, 0, 0, 0, 0, 0); nd_tex_ldtlut(G_TX_LOADTILE, 15); } while (0)`.
3. New 3ds/tools/nd_tx_test.c (copy nd_st_test.c; include nd_tx.h, not nd_st.h; add `int pm_wide;`; replace the pairs :56-91 and table :126-132; print
   `texunit ok N/N` or `texunit FAIL <name>`). 34 shapes: one per setter (30), and gDPLoadSync, gDPTileSync, gDPNoOp, gSPEndDisplayList with an empty n_
   function. Arguments: the first hit of `grep -n "<name>(gMainGfxPos++" src`, a local variable as 37, a pointer as a static array of the test. New
   3ds/tests/host.d/texunit.sh (copy stunit.sh without nd_st.c). Seed first: `mkdir -p /tmp/txseed; sed 's/G_TX_LOADTILE, 15)/G_TX_LOADTILE, 14)/'
   3ds/include/nd_tx.h > /tmp/txseed/nd_tx.h`, `-I /tmp/txseed` before $FL: must print `texunit FAIL tlut16` (the shape of nd_tex_load_tlut16). Real run:
   `texunit ok 34/34`. 4. Gate T0: nd_host_test.sh PASS (texunit, stunit). No build, no Azahar run: no game file includes nd_tx.h yet.
## T1: codemod --tex (branch TEX1 from main after `Merge GEO` with G0 shows in `git log --oneline main`; one commit `TEX T1: ...`)
TEX_MAP after GEO_MAP (form of codemod.py:38-48): `(n, True, '<setter>', tuple(range(1, n)))`, n = gbi argument count with pkt (gDP left out): SetTextureImage 5,
SetTile 13, LoadTile 6, LoadBlock 6, LoadTLUTCmd 3, SetFillColor 2, SetFogColor 5, SetBlendColor 5, gSPViewport 2, gSPFogPosition 3, PipelineMode 2, gSPLoad-
GeometryMode 2, SetKeyR 4, SetKeyGB 7, SetScissorFrac 6, FillRectangle 5, ScisFillRectangle 5, gSPTextureRectangle 10, gSPScisTextureRectangle 10, LoadTexture-
Tile 17, _4b 16, LoadMultiTile 19, _4b 18, LoadTextureBlock 13, _4b 12, LoadTLUT_pal16 3, _pal256 2, ScrollMultiTile2_4b 18, ScrollTextureTile_4b 18, Scroll-
TextureBlock_4b 14. `(1, True, None, None)`: gDPLoadSync, gDPTileSync, gDPNoOp, gSPEndDisplayList. After the --geo line: `elif a == '--tex': MAP.clear();
MAP.update(TEX_MAP)`. run.sh: `tex_*) fl=--tex ;;`. 6 cases: tex_fill (state_logos.c:274-275), tex_load (:283-285), tex_rect (:286), tex_sync (msg_draw.c:1997,
item_entity.c:994), tex_view (draw_box.c:774), tex_dl (form of geo_dl). Gate: run.sh 45/45 PASS, nd_host PASS, no build.
## T2 effects (after `Merge GEO1`), T3 world entity battle evt pause filemenu (after `Merge GEO2`), T4 root src/*.c (after `Merge GEO3`)
Steps 1-4 of ws_GEO G1-G3 with --tex and `#include "nd_tx.h"`. Shapes: state_logos.c:274-275 Before `gDPSetFillColor(gMainGfxPos++, PACK_FILL_COLOR(224, 224,
224, 1));` `gDPFillRectangle(gMainGfxPos++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);` After `nd_val_fill(PACK_FILL_COLOR(224, 224, 224, 1));` `nd_rect_fill(0,
0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);`. :283-285 After (one line) `nd_tex_load_tile(gLogosImage1 + i * 0x1000, G_IM_FMT_RGBA, G_IM_SIZ_16b, 128, 112, 0, 0,
127, 15, 0, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);`. draw_box.c:774 After `nd_xf_viewport(&gBoxViewport);`. msg_draw.c:1997
`gDPTileSync(gMainGfxPos++);`: the line goes. Gate, drec bases: ws_GEO (T2 = G1, T3 = G2, T4 = G3). Commit `TEX T<n>: ...`. Fix rights: T0 new files, T1 codemod.py,
run.sh, cases, T2-T4 include lines. Any cmp, listcen or build difference: no commit, build3ds/out/tex_<t>.txt to the oracle, go on. Hand back as ws_GEO.
