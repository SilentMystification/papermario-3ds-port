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

## Gate (expected values)
- Build, memcheck OK; `bench.py run nd_host --row S5CHUDB --why "S5CHUDB nd_host"`: PASS.
- `bench.py base battle_wheel_nowide_det`, `bench.py base spot_mac01_det`; one list `bench.py run
  battle_wheel_nowide_det,spot_mac01_det,napicen_btl --row S5CHUDB --why "S5CHUDB: complex HUD quads direct"` (3 starts).
  cmp: battle_wheel_nowide_det 1/1, spot_mac01_det 6/6 byte-equal.
- napicen_btl (no base: compare with the S5CHUDA-merge main run in runs.jsonl, or hand back both): hud_element.o ops per frame
  drop; a mode 2 call records 0 typed ops. No `napihud fallback ... why 40` line in the battle_wheel_nowide_det log.
- Budget 8 Azahar starts.

## Stop
A byte difference or a why 40 line: stop, commit, hand back (2 tries per named cause). Delete this row file in the completing
commit.
