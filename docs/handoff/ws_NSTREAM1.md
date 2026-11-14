# ws_NSTREAM1: the setters write native records from native fields (no gbi word, no ND_X0); NSTREAM step 1 (design_lookahead section 8)
Model: Haiku. PM_ROW, PM_LANE = NSTREAM1. Budget: 2 Azahar runs. Start now (no running workstream owns these files; check `git log --oneline -10 main`).
Branch NSTREAM1 (`sh 3ds/tools/wt_new.sh NSTREAM1`). Files: new 3ds/include/nd_nr.h, 3ds/src/nd_st.c, nd_geo.c, nd_val.c, nd_xf.c, 3ds/include/nd_tx.h.
No counter moves (the kinds keep the NDO_ names). Every record keeps every bit, so the stream is the same: stunit, geounit, texunit, hunit compare it to the macro form.
- Facts (main a37368582): st_rec nd_st.c:13-15, geo_rec nd_geo.c:13-15 and nd_tx_rec nd_tx.h:14 build a gbi word and translate it (ND_X0, ND_X1: nd_op.h:79-122). nd_val.c:13-15, nd_xf.c:11 :16 write NDO_W0.
- Seeds that must still match: `G_MDSFT_CYCLETYPE, 2)` nd_st.c (stunit.sh:7), `G_MDSFT_ZSRCSEL, 1)` nd_geo.c (geounit.sh:7), `G_TX_LOADTILE, 15)` nd_tx.h (texunit.sh), `NDO_ENV, 0u` nd_val.c (hunit.sh).
## Step 1: new 3ds/include/nd_nr.h (guard ND_NR_H; includes <stdint.h>, "nd_op.h", "nd_cmd.h"). Copy these lines exactly:
`#define nd_nr(k, v) nd_rec_state((k), (v)) /* one native record into the open run (NSTREAM2 renames the recorder) */`
`#define ND_NR_OM(h, sft, len) NDO_W0((h) ? NDO_OMH : NDO_OML, ((uint32_t)(sft) & 0xffu) | ((uint32_t)(len) & 0xffu) << 8)`
`#define ND_NR_GEOM(clr) NDO_W0(NDO_GEOM, ~(uint32_t)(clr) & 0xffffffu)` and `#define ND_NR_TEXON(on) NDO_W0(NDO_TEXTURE, ((uint32_t)(on) & 0x7fu) != 0u)`
`#define ND_NR_HL(h, l) (((uint32_t)(h) & 0xffffu) << 16 | ((uint32_t)(l) & 0xffffu))` and `#define ND_NR_XY12(x, y) (((uint32_t)(x) & 0xfffu) << 12 | ((uint32_t)(y) & 0xfffu))`
`#define ND_NR_XY10(x, y) (((uint32_t)(x) & 0x3ffu) << 14 | ((uint32_t)(y) & 0x3ffu) << 2)` and `#define ND_NR_TXY(tile, x, y) (((uint32_t)(tile) & 7u) << 24 | ND_NR_XY12(x, y))`
`#define ND_NR_CIMG(siz, width) NDO_W0(NDO_MISC, (uint32_t)NDO_MISC_CIMG << 16 | ((uint32_t)(siz) & 3u) << 12 | (((uint32_t)(width) - 1u) & 0xfffu))`
`#define ND_NR_VTX(n, v0) NDO_W0(NDO_VTX, ((uint32_t)(n) & 0xffu) << 12 | (((uint32_t)(v0) + (uint32_t)(n)) & 0x7fu) << 1)` and `#define ND_NR_MW(kind, arg) NDO_W0(NDO_MW, (uint32_t)(kind) << 16 | (uint32_t)(arg))`
`#define ND_NR_MOVEMEM(len, idx, ofs) NDO_W0(NDO_MOVEMEM, ((((uint32_t)(len) - 1u) >> 3) & 0x1fu) << 19 | (((uint32_t)(ofs) >> 3) & 0xffu) << 8 | ((uint32_t)(idx) & 0xffu))`
`#define ND_NR_IMG(fmt, siz, width) (((uint32_t)(fmt) & 7u) << 21 | ((uint32_t)(siz) & 3u) << 19 | (((uint32_t)(width) - 1u) & 0xfffu))`
`#define ND_NR_TILE_A(fmt, siz, line, tmem) (((uint32_t)(fmt) & 7u) << 21 | ((uint32_t)(siz) & 3u) << 19 | ((uint32_t)(line) & 0x1ffu) << 9 | ((uint32_t)(tmem) & 0x1ffu))`
`#define ND_NR_TILE_B(tile, pal, cmt, maskt, shiftt, cms, masks, shifts)` = the body of ND_W1_TILE (nd_gbi.h:107-109) with each `_SHIFTL(x, s, w)` written `((uint32_t)(x) & <2^w - 1>u) << s`.
`#define ND_NR_TLUT(tile, count) (((uint32_t)(tile) & 7u) << 24 | ((uint32_t)(count) & 0x3ffu) << 14)`, `#define ND_NR_RGBA(r, g, b, a)` (4 bytes, r high, as VAL_COL nd_val.c:11), `#define ND_NR_KEYR(cR, sR, wR) (((uint32_t)(wR) & 0xfffu) << 16 | ((uint32_t)(cR) & 0xffu) << 8 | ((uint32_t)(sR) & 0xffu))`, `#define ND_NR_TR_W0(xh, yh) ((uint32_t)G_TEXRECT << 24 | ND_NR_XY12(xh, yh))`.
## Step 2: the setter bodies (include "nd_nr.h" in each file; delete st_rec, geo_rec, nd_tx_rec, nd_tx_rect). Before and after:
- nd_st.c:24 `st_rec(ND_W0_SOM(G_SETOTHERMODE_H, G_MDSFT_CYCLETYPE, 2), (unsigned)(t));` -> `nd_nr(ND_NR_OM(1, G_MDSFT_CYCLETYPE, 2), (unsigned)(t));` (all 13 SOM setters: `_H,` -> `1,`, `_L,` -> `0,`;
  nd_pso_som: `ND_NR_OM((cmd) == G_SETOTHERMODE_H, sft, len)`). :60 `st_rec(ND_W0_GEOM(0), (u32)(w));` -> `nd_nr(ND_NR_GEOM(0), (u32)(w));` (clr: ND_NR_GEOM(w)). :18 comb: `nd_nr(NDO_W0(NDO_COMB, w0), w1)`.
  :66 nd_tex_on: `nd_nr(ND_NR_TEXON(on), ND_NR_HL(s, t))`. :69 scis: `nd_nr(NDO_W0(NDO_SCISSOR, ND_NR_XY12((int)((float)(ulx) * 4.0F), (int)((float)(uly) * 4.0F))), ND_NR_XY12(<lrx, lry the same>)); (void)mode;`
- nd_geo.c: cimgw `nd_nr(ND_NR_CIMG(siz, width), img); (void)fmt;`; zsrc `ND_NR_OM(0, G_MDSFT_ZSRCSEL, 1)`; primz `nd_nr(NDO_W0(NDO_PRIMZ, 0u), ND_NR_HL(z, dz))`; vtxw `nd_nr(ND_NR_VTX(n, v0), v)`;
  tri1 `nd_nr(NDO_W0(NDO_TRI1, __gsSP1Triangle_w1f(a, b, c, f)), 0u)`; tri2 `nd_nr(NDO_W0(NDO_TRI2, __gsSP1Triangle_w1f(a, b, c, f)), __gsSP1Triangle_w1f(d, e, g, h))`;
  :36 `geo_rec(ND_W0_MW(G_MW_NUMLIGHT, G_MWO_NUMLIGHT), NUML(NUMLIGHTS_1));` -> `nd_nr(ND_NR_MW(NDO_MW_NUMLIGHT, 0u), NUML(NUMLIGHTS_1));`; each `ND_W0_D2(G_MOVEMEM, sizeof(Light), G_MV_LIGHT, X)`
  -> `ND_NR_MOVEMEM(sizeof(Light), G_MV_LIGHT, X)`; pnorm and the 4 clip lines: `ND_NR_MW(NDO_MW_NONE, 0u)` (G_MW_PERSPNORM and G_MW_CLIP are kind NONE, nd_op.h:71).
- nd_val.c:14 and nd_xf.c:11 :16: `nd_rec_state(` -> `nd_nr(`.
- nd_tx.h: img `nd_nr(NDO_W0(NDO_TIMG, ND_NR_IMG(f, s, w)), (unsigned int)(i))`; tile `nd_nr(NDO_W0(NDO_TILE, ND_NR_TILE_A(...)), ND_NR_TILE_B(...))`; ldtile and tsz `nd_nr(NDO_W0(NDO_LOADTILE or NDO_TILESIZE,
  ND_NR_XY12(uls, ult)), ND_NR_TXY(t, lrs, lrt))`; ldblk `ND_NR_TXY(tile, MIN(lrs, G_TX_LDBLK_MAX_TXL), dxt)` with NDO_LOADBLOCK; ldtlut `nd_nr(NDO_W0(NDO_LOADTLUT, 0u), ND_NR_TLUT(tile, count))`;
  fill, fog, blend `NDO_W0(NDO_FILL|NDO_FOG|NDO_BLEND, 0u)` with ND_NR_RGBA for fog and blend; viewport `ND_NR_MOVEMEM(sizeof(Vp), G_MV_VIEWPORT, 0)`; fogpos `ND_NR_MW(NDO_MW_FOG, 0u)` with ND_NR_HL(<the two values>);
  pipe `ND_NR_OM(1, G_MDSFT_PIPELINE, 1)`; geom_load `ND_NR_GEOM(0xffffffffu)`; keyr `NDO_W0(NDO_RARE, NDO_RARE_KEYR)`, ND_NR_KEYR; keygb `NDO_W0(NDO_RARE, NDO_RARE_KEYGB)`, ND_NR_RGBA(cG, sG, cB, sB);
  scisf as scis with `(int)(x)`; rect_fill `nd_nr(NDO_W0(NDO_RECT, ND_NR_XY10(lrx, lry)), ND_NR_XY10(ulx, uly))` (sfill: each value in MAX((v), 0));
  rect_tex `nd_rec_texrect(ND_NR_TR_W0(xh, yh), ND_NR_TXY(tile, xl, yl), ND_TR_H1(s, t), ND_TR_H2(dsdx, dtdy))`; rect_scis the same with its corner and h1, h2 expressions unchanged.
- Check: `grep -nE "ND_X[01]|ND_W[01]_|_SHIFTL\(G_|st_rec|geo_rec|nd_tx_rec" 3ds/src/nd_st.c 3ds/src/nd_geo.c 3ds/src/nd_val.c 3ds/src/nd_xf.c 3ds/include/nd_tx.h` prints nothing.
## Gate (1 build, 2 Azahar runs)
`sh 3ds/tools/nd_host_test.sh` PASS: stunit ok 18/18, geounit ok 13/13, texunit ok 36/36, hunit ok 7/7, each seed line seen. Build rc 0, memcheck margin 310 KB or more.
`bench.py run` then `cmp --commit <merge-base>` byte-equal: gold_12_drec, spot_mac01_drec. Commit `NSTREAM1: ...` with the unit lines. The commit deletes this page.
Fix rights: the six files. A unit FAIL names the setter: fix its pack from the lines above (2 tries per setter). A drec difference: no commit, write build3ds/out/nstream1.txt,
send the path to the oracle. Hand back: 8 lines (hash, unit lines, drec results, margin; details build3ds/out/handback_NSTREAM1.txt).
