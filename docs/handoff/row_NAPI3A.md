# Row NAPI3A: message glyphs as one native draw record each (plan_nativeapi S3a)

(Model: Haiku. Starts after NAPI1 is merged; runs in parallel with NAPI2A: different file.)

## Design (oracle code read, main 6e171b39a; line numbers of src/msg_draw.c there)
msg_draw_char (:1772-1911) records 540-600 typed ops per dialogue frame (napicen_talk f600): per glyph an optional palette load
(gDPLoadTLUT_pal16, 6 units), gDPLoadTextureBlock_4b or gDPLoadTextureTile_4b (7 units) and gSPTextureRectangle (3 units). Core 1
keeps all the arithmetic (clip, offsets, scale) and the printModeFlags update (it writes game state) and records one NDC_NAPI per glyph
with an NdGlyphRec. Core 0 (msg_glyph_run) runs the same three macros on the recorded values into a local list (nd_gx_w writes the typed
unit into a packet that is not gMainGfxPos, nd_rec.c:377) and runs it with nd_walk_state. Same units, same order: byte-equal by
construction, no GPU state change (NAPI2A design note). VERSION_IQUE lines are not built (US).

## Start
`sh 3ds/tools/wt_new.sh NAPI3A` (after the NAPI1 merge). PM_ROW=NAPI3A PM_LANE=NAPI3A on every build and bench call.

## Touch only
src/msg_draw.c: the include list, new code just above msg_draw_char, the emit lines of msg_draw_char. No other file.

## Exact edits (src/msg_draw.c)
1. After `#include "sprite.h"` (line 5): `#include "nd_api.h"`.
2. Above msg_draw_char (line 1772):
   `typedef struct NdGlyphRec { u32 raster, pal; s32 ulx, uly, lrx, lry, texOffsetX, texOffsetY, dsdx, dtdy; s16 texW, texH; s32 tlut; }
   NdGlyphRec;` and `_Static_assert(sizeof(NdGlyphRec) == 48, "NdGlyphRec: 6 stream units");`
   `static void msg_glyph_run(const uint32_t* u)` (core 0): `const NdGlyphRec* r = (const NdGlyphRec*)u; Gfx buf[24]; Gfx* p = buf;`
   then `if (r->tlut) { gDPLoadTLUT_pal16(p++, 0, r->pal); }`;
   `if (r->texW >= 16 && r->texW % 16 == 0) {` the gDPLoadTextureBlock_4b call of line 1872 with `r->raster` for the raster expression
   and `r->texW`, `r->texH` for messageCharset->texSize.x/.y `} else {` the gDPLoadTextureTile_4b call of line 1892 with the same
   replacements (also in `r->texW - 1`, `r->texH - 1`) `}`;
   `gSPTextureRectangle(p++, r->ulx * 4, r->uly * 4, r->lrx * 4, r->lry * 4, G_TX_RENDERTILE, r->texOffsetX, r->texOffsetY, r->dsdx,
   r->dtdy);` then `nd_walk_state((const uint32_t*)buf, (unsigned)(p - buf));` (at most 6 + 7 + 3 = 16 units).
3. msg_draw_char: put `#ifdef TARGET_3DS`, the block below, `#else` before line 1855 and `#endif` after line 1901 (the original
   lines stay as the #else branch). The block: `{ NdGlyphRec rec;` `rec.tlut = 0; rec.pal = 0;`
   `if (drawState->printModeFlags & (MSG_PRINT_FLAG_10 | MSG_PRINT_FLAG_1)) { drawState->printModeFlags &= ~(MSG_PRINT_FLAG_10 |
   MSG_PRINT_FLAG_1); rec.tlut = 1; rec.pal = (u32)(uintptr_t)D_802F4560[palette]; }`
   `rec.raster = (u32)(uintptr_t)(messageCharset->rasters[fontVariant].raster + messageCharset->charRasterSize * charIndex);`
   `rec.texW = messageCharset->texSize.x; rec.texH = messageCharset->texSize.y;` rec.ulx, uly, lrx, lry, texOffsetX, texOffsetY,
   dsdx, dtdy = the locals of the same names; `nd_rec_napi(msg_glyph_run, &rec, sizeof rec / 8u); }`

## Gate (CLAUDE.md 2)
- `PM_ROW=NAPI3A PM_LANE=NAPI3A sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. nd_host PASS (bench nd_host).
- `PM_ROW=NAPI3A PM_LANE=NAPI3A python 3ds/tools/bench.py base msgbox_talk_nowide`; then one list:
  `python 3ds/tools/bench.py run msgbox_talk_nowide,napicen_talk --row NAPI3A --why "NAPI3A: glyphs as one NDC_NAPI each, core 0
  replays the same ops"`; then `bench.py cmp msgbox_talk_nowide --row NAPI3A --commit $(git merge-base HEAD main)`: byte-equal.
  napicen: `python 3ds/tools/napicen.py build3ds/azt/logs/napicen_talk.txt --map build3ds/lane-NAPI3A/pm_3ds.map --label
  napi3a_talk`: in build3ds/out/napicen_napi3a_talk.txt window f600 has no msg_draw_char line. Budget: at most 4 Azahar starts.

## Stop
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, then hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash, the gate results, the memcheck line.
