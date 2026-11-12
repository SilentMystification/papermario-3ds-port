# Row S5CHUDB: complex HUD elements of mode 2 (IA8) as one direct record (call site of nd_direct_quad)

(Model: Haiku. One function, every line below. Blocker: S5CHUDA merged (nd_direct_quad, nd_tex_blk_desc, and NdCHudRec plus
cplx_quad_run in src/hud_element.c). Before the edit, grep those 4 names: a missing one or another NdCHudRec field list is a stop.)

## Code read (src/hud_element.c, main 5c3865487)
render_complex_hud_element (:1623-1841). Mode 2 iff `flags & HUD_ELEMENT_FLAG_FMT_IA8` (the mode logic :1704-1722: CI4 alone
gives 1, IA8 with or without CI4 gives 2). Its ops: filter (:1647-1651), gSPMatrix (:1701), PipeSync, combine, prim, render
mode, LoadTextureBlock, TLUT, vertex, 2 triangles (:1806-1835), pop and PipeSync (:1839-1840). Syncs are not recorded.
After this row a mode 2 call records only the NDC_NAPI record; modes 0 and 1 keep their ops.

## Start
`sh 3ds/tools/wt_new.sh S5CHUDB`. PM_ROW=S5CHUDB PM_LANE=S5CHUDB on every call.

## Exact edits (src/hud_element.c only)
1. The record NdCHudRec and its runner cplx_quad_run are in hud_element.c after hud_rect_run (written by S5CHUDA).
2. render_complex_hud_element, all under `#ifdef TARGET_3DS` with the old line in `#else` (or the guard shown):
   a. First statement of the body: `#ifdef TARGET_3DS` `const int cplx_direct = (hudElement->flags & HUD_ELEMENT_FLAG_FMT_IA8) != 0;`
      `#endif`. Wrap the filter if/else (:1647-1651) in `if (!cplx_direct) { ... }` under TARGET_3DS.
   b. Replace the gSPMatrix statement (:1701-1702) under TARGET_3DS by
      `Mtx* cplx_mtx = &gDisplayContext->matrixStack[gMatrixListPos++];` and
      `if (!cplx_direct) gSPMatrix(gMainGfxPos++, VIRTUAL_TO_PHYSICAL(cplx_mtx), G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);`
      (gMatrixListPos still advances once; declare cplx_mtx at the top of the function under TARGET_3DS if C90 rules need it).
   c. In `case 2:` keep the 4 template copies and the vertex lines (:1776-1804). Replace :1806-1835 (PipeSync to the second
      gSP1Triangle; keep the ctr_store debug block) under TARGET_3DS by
      ```c
      {
          NdCHudRec rec;
          rec.img = hudElement->imageAddr; rec.mtx = VIRTUAL_TO_PHYSICAL(cplx_mtx); rec.vtx = &transform->quadBuffers[FrameQuadIndex];
          rec.w = width; rec.h = height; rec.flags = hudElement->flags; rec.pad = 0;
          rec.prim = ((u32)hudElement->tint.r << 24) | ((u32)hudElement->tint.g << 16) | ((u32)hudElement->tint.b << 8) | (u32)hudElement->opacity;
          nd_rec_napi(cplx_quad_run, &rec, sizeof rec / 8u);
      }
      ```
   d. The gSPPopMatrix and gDPPipeSync after the switch (:1839-1840): under TARGET_3DS `if (!cplx_direct) { both }`.

## Try 2 (oracle 5): every mode 2 call fell back (napicen_btl: 41 of 41 `why 40`)
- Named cause (code read): nd_tex_blk_desc (3ds/src/nd_direct.c:363-366) refuses `w * h > 2048`. That cap is the 16-bit limit.
  gDPLoadTextureBlock of an 8-bit image loads it as 16-bit words: texels ((w * h + 1) >> 1) - 1 <= 2047, so an IA8 block holds up
  to 4096 texels (4 KB TMEM). An IA8 complex HUD image of 48 x 48 (2304) or 64 x 64 (4096) is refused. The other two checks
  (nd_direct_ready, nd_direct_bind) are the same as hud_rect_direct's, which passes in the same frames (no why 31-34 lines).
- Fix 1 (nd_direct.c:364): replace `w * h > 2048` by `(((unsigned)(w * h) << siz) >> 1) > 4096u` (bytes of the image: 4b half a
  byte per texel, 8b one, 16b two, 32b four; siz is G_IM_SIZ_4b..32b = 0..3).
- Fix 2 (cplx_quad_run in hud_element.c): name the failing check in the census: `why 41` nd_tex_blk_desc, `why 42`
  nd_direct_ready, `why 43` nd_direct_bind. Write the three tests as separate ifs, each calling nd_direct_fallback with its value
  and then the old ops block (one shared label or a helper).
- Both fixes in one commit on branch S5CHUDB (it holds 0d2556b76). Budget reset once.

## Gate (expected values)
- Build, memcheck OK; `bench.py run nd_host --row S5CHUDB --why "S5CHUDB try 2"`: PASS (nd_direct_test's nd_tex_blk_desc case
  still passes; add a case 48 x 48 IA8 -> 1).
- `bench.py base battle_wheel_nowide_det` and `bench.py base spot_mac01_det` (merge-base); one list `bench.py run
  battle_wheel_nowide_det,spot_mac01_det,napicen_btl --row S5CHUDB --why "S5CHUDB try 2: blk cap"` (3 starts).
  cmp: battle_wheel_nowide_det 1/1, spot_mac01_det 6/6 byte-equal.
- napicen_btl log: 0 lines with why 41. A why 42 or 43 line: stop and hand back the count per why (the cause is then the
  walker state, and the oracle reads that path). hud_element.o ops per frame: hand back the value (expected lower than main's).
- Budget 8 Azahar starts.

## Stop
A byte difference or a why 41-43 line: stop, commit, hand back (2 tries per named cause). Delete this row file in the
completing commit.
