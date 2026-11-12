# Row S5SPR2: the sprite render-mode lists as S6T compiled ranges (no G_DL per sprite component)

(Model: Haiku. The S6S1 pattern: a static list becomes an NdSDef, its gSPDisplayList becomes nd_scomp_rec. One file, two places.)

## Code read (oracle 4, main eacd96ed9)
- spr_appendGfx_component (src/sprite.c:400-511) records `gSPDisplayList(gMainGfxPos++, rmList)` at :478 for every component
  (quad and imgfx arms). rmList is one of 4 static lists (:62-98): OpaqueSpriteGfx and TranslucentSpriteGfx (6 units each:
  ClearGeometryMode, SetCombineMode, Texture, SetGeometryMode, SetRenderMode, SetOtherMode H; then End), OpaqueShadedSpriteGfx and
  TranslucentShadedSpriteGfx (5 units: no combine; then End).
- Every unit is a static kind of the compiler (nd_scomp.c unit_class: GEOM, COMB, TEXTURE, OML, OMH), so nv = 0.
- The sprite record keeps the rmList pointer (rec[0].w1, :466); no core 0 reader uses it (pm_gpu_spr_build sets mode 0). Keep it.
- Def count: cam, icons (S6T), init_vp, init_rm (S6S1), msg (S6S2) and these 4 = 9, under the cap of 16 (nd_scomp.c:58).

## Start
`sh 3ds/tools/wt_new.sh S5SPR2`; PM_ROW=S5SPR2 PM_LANE=S5SPR2 on every build and bench call.

## Touch only
src/sprite.c: the include block (:4-8), a new block right after TranslucentShadedSpriteGfx (:98), and line :478 in
spr_appendGfx_component. No running row edits sprite.c.

## Exact edits (src/sprite.c)
1. In the `#ifdef TARGET_3DS` include block (:4-8) add `#include "nd_scomp.h"`.
2. After the closing `};` of TranslucentShadedSpriteGfx (:98) add:
   ```
   #ifdef TARGET_3DS
   /* S5SPR2: the render-mode lists as compiled state ranges (S6T, nd_scomp.h); index = shaded * 2 + translucent */
   static NdSDef spr_rm_def[4] = {
       { OpaqueSpriteGfx, 6 }, { TranslucentSpriteGfx, 6 }, { OpaqueShadedSpriteGfx, 5 }, { TranslucentShadedSpriteGfx, 5 },
   };
   #endif
   ```
3. Replace line :478 `gSPDisplayList(gMainGfxPos++, rmList); /* DLF5 ... */` with:
   ```
   #ifdef TARGET_3DS
       nd_scomp_rec(&spr_rm_def[((gSpriteShadingProfile->flags & SPR_SHADING_FLAG_ENABLED) ? 2 : 0) + ((u8) opacity == 255 ? 0 : 1)], NULL, 0);
   #else
       gSPDisplayList(gMainGfxPos++, rmList); /* DLF5: a render mode list is a G_DL (ST-4) */
   #endif
   ```
   The index must pick the same list as the rmList choice at :454-458. Keep rmList (the record uses it).

## Gate (expected values)
- `PM_ROW=S5SPR2 PM_LANE=S5SPR2 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row S5SPR2 --why "S5SPR2 build"`: PASS.
- `PM_ROW=S5SPR2 PM_LANE=S5SPR2 python 3ds/tools/bench.py base spot_mac01_det` and the same for spot_mac00_det (stored shots of
  the merge-base; no start).
- `python 3ds/tools/bench.py run spot_mac01_det,spot_mac00_det --row S5SPR2 --why "S5SPR2: sprite render-mode lists compiled"`
  (2 starts), then `bench.py cmp spot_mac01_det --row S5SPR2` and `bench.py cmp spot_mac00_det --row S5SPR2`: every shot byte-equal.
- `python 3ds/tools/bench.py run s6chk_mac01 --row S5SPR2 --why "s6chk on the sprite ranges"` (1 start; until = the first
  `s6chk f2xx` line). Expected: `bad 0`, and `ranges` higher than in the last s6chk_mac01 run of main (bench.py show s6chk_mac01).
- grep the three logs: no `scomp: def` line. Budget 4 starts.

## Stop
A byte difference, `bad` above 0, a `scomp: def` line, or a test-tool fault: commit, hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash, the gate values, the memcheck line.
