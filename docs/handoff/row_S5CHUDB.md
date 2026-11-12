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

## Try 2 (oracle 5, done in 5edc90be1)
nd_tex_blk_desc's byte cap fixed (nd_direct.c:364), and the census split into why 41 (tex block), 42 (ready) and 43 (bind).

## Try 3 (ORACLE6): why 41 0, why 43 0, why 42 22 (napicen_btl, 5edc90be1)
- nd_direct_ready (3ds/src/nd_direct.c:83-89) has four refusals: -1 not the main colour image or cimg_zero, -2 a non-frame-buffer
  colour image, -3 `m1.valid`, -4 an off-screen tile 1 (pm_tex_aux_off_idx).
- Path -3, named by code read: in battle, the actors are shaded sprites. tex_tile1 (nd_tex_set.c:566-568) sets m1.valid = 1 at
  the 4-bit tile 1 TILESIZE. Only a later tile 0 TILESIZE clears it (nd_tex_set.c:205-206), and tex_shaded0 (:611-615) clears it
  only when it takes the shaded draw. So after the last shaded sprite of a frame, m1.valid stays 1 until the next walker tile 0
  load. The complex HUD record has no such load before it, so it reads -3. The old ops it falls back to then clear m1.valid at
  their IA8 TILESIZE: tex_shaded0 needs tile 0 CI 4-bit (`tile_fmt[0] == 2 && tile_siz[0] == 0`, :611), and IA8 is never that,
  so :206 runs `m1.valid = 0`. The direct IA8 bind can leave the same state: clear m1.valid and go on.
- Paths -1, -2 and -4 are off-screen targets: their fallback is correct and stays.
- Edits (src/hud_element.c cplx_quad_run only):
  1. Declare `int rd;` at the top of the function. The test chain becomes:
     `if (!nd_tex_blk_desc(...)) nd_direct_fallback(41u);`
     `else if ((rd = nd_direct_ready()) != 1 && rd != -3) nd_direct_fallback((unsigned)(43 - rd));`
     `else if (!nd_direct_bind(...)) nd_direct_fallback(43u);`
     `else { if (rd == -3) nd_direct_m1_clear(); nd_direct_tex(&d); nd_direct_quad(...); nd_direct_end(); return; }`.
     The why values are then 44 (-1), 45 (-2) and 47 (-4).
  2. 3ds/src/nd_direct.c (end) and nd_api.h (end): `void nd_direct_m1_clear(void) { m1.valid = 0; }` with the comment "the
     walker's tile 0 TILESIZE of a non-CI4 tile clears the shaded sprite raster (nd_tex_set.c:205-206)". The source line goes in
     nd_direct.c because m1 is in nd_int.h, which hud_element.c does not include.
  Apply the merge rule above (file ends) before the gate.

## Gate (expected values)
- Build, memcheck OK; `bench.py run nd_host --row S5CHUDB --why "S5CHUDB try 3"`: PASS.
- Bases at the new merge-base: battle_wheel_nowide_det, spot_mac01_det. One list `bench.py run battle_wheel_nowide_det,
  spot_mac01_det,napicen_btl --row S5CHUDB --why "S5CHUDB try 3: m1 clear"` (3 starts). cmp 1/1 and 6/6 byte-equal.
- napicen_btl: expected 0 lines with why 41, 43, 44, 45 and 47, so all 41 mode 2 quads are direct. If a why 44, 45 or 47 line
  shows: stop and hand back its count. That would be an off-screen path, which the oracle then reads. Hand back hud_element.o
  ops per frame.
- Budget 8 Azahar starts (reset once more for this row fault).

## Stop
A byte difference, or any why 41 to 47 line: stop, commit, hand back (2 tries per named cause). Delete this row file in the
completing commit.
