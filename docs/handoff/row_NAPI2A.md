# Row NAPI2A: HUD element rectangles as one native draw record (plan_nativeapi S2a)

(Model: Haiku. Starts after NAPI1 is merged: it calls nd_rec_napi and nd_walk_state of 3ds/include/nd_api.h.)

## Design (oracle code read, main 6e171b39a; line numbers of src/hud_element.c there)
hud_element_draw_rect (:267-622) is 37-45 % of the recorded ops (napicen f600: 368-440 per frame). Core 1 keeps all its arithmetic and
records one NDC_NAPI per call (NdHudRec: the inputs of the emit lines, and its pieces). Core 0 (hud_rect_run) runs the same emit lines
into a local list (nd_gx_w writes the typed unit into a packet that is not gMainGfxPos, nd_rec.c:377) and runs it with nd_walk_state:
same units, same order (syncs are ND_NOP_A, skipped; a shadow drop is a skip in nd_op_same; op_ctl NDO_RECT reaches nd_texrect as
NDC_TEXRECT did). Byte-equal by construction, no GPU state change. fps60 anchors (pm_anchor_rect) still emit on core 1.

## Start
`sh 3ds/tools/wt_new.sh NAPI2A` (after the NAPI1 merge). PM_ROW=NAPI2A PM_LANE=NAPI2A on every build and bench call.

## Touch only
src/hud_element.c: hud_element_draw_rect, new code just above it, pm_anchor_rect's end. No other file.

## Exact edits (src/hud_element.c)
1. After `#include "pm_interp_fi.h"`: `#include "nd_api.h"`. Above hud_element_draw_rect: `void pm_fatal_stop(const char* msg);`,
   `int pm_anchor_is_open(void);` and
   `#define ND_HUD_PC 32` (texture sizes are at most 255 x 255: 4 x 8 pieces of 64 x 32 texels)
   `typedef struct NdHudPiece { s32 uls, ult, lrs, lrt, ulx, uly, lrx, lry, texStartX, texStartY, tileMode, pad; } NdHudPiece;`
   `typedef struct NdHudRec { u32 flags; IMG_PTR imageAddr; PAL_PTR paletteAddr; s32 widthScale, heightScale, fmt, masks, maskt, flipX,
   flipY, clamp, dropShadow, npc; s16 texSizeX, texSizeY; u8 opacity; Color_RGB8 tint; s32 pad; NdHudPiece pc[ND_HUD_PC]; } NdHudRec;`
   `_Static_assert(sizeof(NdHudPiece) == 48 && offsetof(NdHudRec, pc) == 64, "NdHudRec: whole 8-byte units");`
2. The emitter, above hud_element_draw_rect: `#define gMainGfxPos (*gp)`, then
   `static void hud_rect_emit(const NdHudRec* hudElement, Gfx** gp, s32 anchor) {` with locals copied from hudElement: `s32 fmt, masks,
   maskt, flipX, flipY, clamp, dropShadow, widthScale, heightScale; s16 texSizeX, texSizeY; IMG_PTR imageAddr; PAL_PTR paletteAddr; s32 k;`
   Body: lines 368-400 verbatim (the render mode, TLUT and filter switch); then `for (k = 0; k < hudElement->npc; k++) {` with
   `const NdHudPiece* pc = &hudElement->pc[k];` and s32 locals uls, ult, lrs, lrt, ulx, uly, lrx, lry, texStartX, texStartY, tileMode
   from pc; then line 476 (`gDPPipeSync(gMainGfxPos++);`), then lines 491-605 verbatim except the two pm_anchor_rect tests (598, 602):
   `if (!pm_anchor_rect(...))` becomes `if (!(anchor && pm_anchor_rect(...)))`; close the loop; then `gDPPipeSync(gMainGfxPos++);`
   (line 621) and `}`; then `#undef gMainGfxPos`.
3. The core 0 runner, after the emitter: `static void hud_rect_run(const uint32_t* u) { static Gfx buf[512]; Gfx* p = buf;
   hud_rect_emit((const NdHudRec*)u, &p, 0); nd_walk_state((const uint32_t*)buf, (unsigned)(p - buf)); }` with the comment: at most
   9 prologue units + 32 pieces of 14 + 1 = 458 units.
4. hud_element_draw_rect: add local `NdHudRec rec;`. After the masks block (line 364) fill the header: flags = hudElement->flags,
   imageAddr, paletteAddr, widthScale, heightScale, fmt, masks, maskt, flipX, flipY, clamp, dropShadow, npc = 0, texSizeX, texSizeY,
   opacity = hudElement->opacity, tint = hudElement->tint, pad = 0. Delete lines 366-402 (the moved switch and its empty markers).
   In the inner loop delete lines 474-476 (markers and the sync); keep 478-489 (tileMode); replace 491-605 by:
   `if (rec.npc >= ND_HUD_PC) pm_fatal_stop("hud: element over 32 pieces");` and a block that sets all 12 fields of
   `rec.pc[rec.npc]` from the locals of the same names (pad 0), then `rec.npc++;`.
   Replace line 621 by: `if (pm_anchor_is_open()) hud_rect_emit(&rec, &gMainGfxPos, 1); else nd_rec_napi(hud_rect_run, &rec,
   8u + 6u * (unsigned)rec.npc);`. 5. After pm_anchor_rect (:2375): `int pm_anchor_is_open(void) { return sAnchorOpen; }`.

## Gate (CLAUDE.md 2)
- `PM_ROW=NAPI2A PM_LANE=NAPI2A sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. nd_host PASS (bench nd_host).
- Size fact: `grep -A1 -E "\.text\.(hud_rect_emit|hud_rect_run|hud_element_draw_rect)" build3ds/lane-NAPI2A/pm_3ds.map` (hand back).
- `PM_ROW=NAPI2A PM_LANE=NAPI2A python 3ds/tools/bench.py base spot_mac01_det` and `... base msgbox_talk_nowide`; then one list:
  `python 3ds/tools/bench.py run spot_mac01_det,msgbox_talk_nowide,napicen_mac01 --row NAPI2A --why "NAPI2A: HUD rects as one
  NDC_NAPI per call, core 0 replays the same ops"`; then `bench.py cmp spot_mac01_det --row NAPI2A --commit $(git merge-base HEAD main)`
  and the same for msgbox_talk_nowide: every shot byte-equal. napicen: `python 3ds/tools/napicen.py build3ds/azt/logs/napicen_mac01.txt
  --map build3ds/lane-NAPI2A/pm_3ds.map --label napi2a_mac01`: in build3ds/out/napicen_napi2a_mac01.txt window f600 has no
  hud_element_draw_rect line. Budget: at most 5 Azahar starts.

## Stop
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, then hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash, the gate results, the memcheck line, the sizes.
