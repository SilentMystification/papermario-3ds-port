# Row S5SPR3: the sprite matrix, primitive colour and render-mode state as one S6T range per component

(Model: Haiku. One file, one function plus the def block of S5SPR2. Exact edits below.)

## Blocker and base
S5SPR2 and SCOMP0 merged (SCOMP0: a PRIM unit before the first static unit is a value unit). `sh 3ds/tools/wt_new.sh S5SPR3`
from that main; PM_ROW=S5SPR3 PM_LANE=S5SPR3 on every build and bench call.

## Code read (oracle 4, main eacd96ed9)
spr_appendGfx_component (src/sprite.c:400-511) records, in this order and with nothing recorded between: gSPMatrix (:437, PUSH |
LOAD | MODELVIEW, matrixStack[gMatrixListPos++]), the PrimColor (:475-477, unshaded and opacity != 255 only), the render-mode range
(:478, S5SPR2: nd_scomp_rec(&spr_rm_def[i], NULL, 0)). The record words (:469) read gMatrixListPos - 1 after :437: keep that order.
MTX is a value unit (nd_scomp.c unit_class); PRIM before the first static unit is a value unit after SCOMP0.

## Touch only
src/sprite.c: the S5SPR2 def block after TranslucentShadedSpriteGfx, and spr_appendGfx_component lines :436-478. No other row.

## Exact edits (src/sprite.c)
1. Replace the S5SPR2 block `static NdSDef spr_rm_def[4] = {...};` with 4 unit arrays and their defs (inside the same `#ifdef
   TARGET_3DS`). Each array is `static const Gfx spr_st_uN[] = { gsSPMatrix(0, G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW), ... };`
   followed by the units of its list copied exactly, without the End unit:
   - spr_st_u0: the matrix, then the 6 units of OpaqueSpriteGfx (:63-68);
   - spr_st_u1: the matrix, `gsDPSetPrimColor(0, 0, 0, 0, 0, 0)`, then the 6 units of TranslucentSpriteGfx (:73-78);
   - spr_st_u2: the matrix, then the 5 units of OpaqueShadedSpriteGfx (:83-87);
   - spr_st_u3: the matrix, then the 5 units of TranslucentShadedSpriteGfx (:92-96).
   Then `static NdSDef spr_st_def[4] = { { spr_st_u0, 7 }, { spr_st_u1, 8 }, { spr_st_u2, 6 }, { spr_st_u3, 6 } };`.
2. In spr_appendGfx_component:
   - At :436-438 keep guMtxF2L. Under `#ifdef TARGET_3DS` declare `Gfx stv[2];` and write the matrix into `stv[0]`:
     `gSPMatrix(&stv[0], VIRTUAL_TO_PHYSICAL(&gDisplayContext->matrixStack[gMatrixListPos++]), G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);`
     `#else` keeps the old gSPMatrix on gMainGfxPos.
   - Replace :475-478 (the PrimColor if-block and the S5SPR2 nd_scomp_rec) under `#ifdef TARGET_3DS` with:
     ```
     {
         const int si = ((gSpriteShadingProfile->flags & SPR_SHADING_FLAG_ENABLED) ? 2 : 0) + ((u8) opacity == 255 ? 0 : 1);
         unsigned nv = 1;
         if (si == 1) { gDPSetPrimColor(&stv[1], 0, 0, 0, 0, 0, (u8) opacity); nv = 2; }
         nd_scomp_rec(&spr_st_def[si], stv, nv);
     }
     ```
     `#else` keeps the old PrimColor if-block and `gSPDisplayList(gMainGfxPos++, rmList);`.
   The record block (:459-474) stays where it is (it reads gMatrixListPos - 1, set above).

## Gate (expected values)
- Speed rule (oracle 5): a bench list runs at one speed; spot_mac00_det is a turbo bench (bench_defs speed turbo), every
  other bench here is normal. Run it as its own list: `bench.py run spot_mac00_det --row S5SPR3 --why "..."` (1 start).
- `PM_ROW=S5SPR3 PM_LANE=S5SPR3 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row S5SPR3 --why "S5SPR3 build"`: PASS.
- `PM_ROW=S5SPR3 PM_LANE=S5SPR3 python 3ds/tools/bench.py base spot_mac01_det`, the same for spot_mac00_det. One list:
  `python 3ds/tools/bench.py run spot_mac01_det,s6chk_mac01,napicen_mac01 --row S5SPR3 --why "S5SPR3: sprite
  state range"` (4 starts). `bench.py cmp` both det benches: every shot byte-equal. s6chk: `bad 0`. napicen_mac01: hand back the
  sprite.o ops per frame (expected: lower than main's value, each component loses its MTX and PRIM ops). No `scomp: def` line.
  Budget 6 starts.

## Stop and end
A byte difference, `bad` above 0, a `scomp: def` line, an edit that does not fit, or a test-tool fault: commit, hand back the
exact output. Delete this row file in the completing commit. Hand back: the ready hash, the gate values, the memcheck line.
