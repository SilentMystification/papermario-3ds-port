# Row S5PQUAD2: the rest of btl_appendGfx_prim_quad as compiled state ranges (no typed op left in the function)

(Model: Haiku. One function and two defs in one file: the S6S1 pattern, src/background_gfx.c:36-51. Blocker: S5PQUAD merged.)

## Code read (S5PQUAD branch 93d224cb7, src/battle/190B20.c)
- btl_appendGfx_prim_quad (:3020-3046): PipeSync, gSPDisplayList(D_80293970), the NdFillRec record (S5PQUAD), PipeSync,
  RenderMode TEX_EDGE, CombineMode DECALRGBA. napicen_btl after S5PQUAD: 190B20.o 4 ops per quad (16 at f600, 4 quads).
- D_80293970 (:44-55): PipeSync, gsSPTexture, 7 othermode H setters, End. nd_scomp.c:16-27 classes: sync 3 (dropped),
  NDO_TEXTURE, NDO_OMH, NDO_OML, NDO_COMB 1 (static). So D_80293970 (9 units without End) and a new list of PipeSync,
  RenderMode, CombineMode (3 units) compile with no value unit: nd_scomp_rec(&def, NULL, 0).
- After this row the function writes nothing to gMainGfxPos under TARGET_3DS (both syncs move into the #else): 0 typed ops.

## Start
`sh 3ds/tools/wt_new.sh S5PQUAD2`. PM_ROW=S5PQUAD2 PM_LANE=S5PQUAD2 on every call.

## Touch only
src/battle/190B20.c: the block right after D_80293970 (:55) and btl_appendGfx_prim_quad. No other file.

## Exact edits
1. After the closing `};` of D_80293970 add:
   ```c
   #ifdef TARGET_3DS
   #include "nd_scomp.h"
   /* S5PQUAD2: the setup list and the restore after the fill record as compiled state ranges (nd_scomp.h; counts leave out End). */
   static Gfx pq_restore_gfx[] = {
       gsDPPipeSync(),
       gsDPSetRenderMode(G_RM_TEX_EDGE, G_RM_TEX_EDGE2),
       gsDPSetCombineMode(G_CC_DECALRGBA, G_CC_DECALRGBA),
       gsSPEndDisplayList(),
   };
   static NdSDef pq_def_setup = { D_80293970, 9 };
   static NdSDef pq_def_restore = { pq_restore_gfx, 3 };
   #endif
   ```
2. btl_appendGfx_prim_quad: the body becomes
   ```c
   #ifdef TARGET_3DS
       nd_scomp_rec(&pq_def_setup, NULL, 0);
       {   <the NdFillRec block of S5PQUAD, unchanged> }
       nd_scomp_rec(&pq_def_restore, NULL, 0);
   #else
       <the original body: PipeSync, gSPDisplayList(D_80293970), the if/else render mode and combine, PrimColor, FillRectangle,
        PipeSync, RenderMode TEX_EDGE, CombineMode DECALRGBA, unchanged>
   #endif
   ```
   The comment line of S5PQUAD above the NdFillRec block stays.

## Gate (expected values)
- `sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. `bench.py run nd_host --row S5PQUAD2 --why "S5PQUAD2 nd_host"`:
  PASS (includes SCOMP TEST).
- `bench.py base battle_wheel_nowide_det` and `bench.py base xfer_map_shots`; one list
  `bench.py run battle_wheel_nowide_det,xfer_map_shots,napicen_btl --row S5PQUAD2 --why "S5PQUAD2: prim quad state as defs"`
  (3 starts). cmp: battle_wheel_nowide_det 1/1, xfer_map_shots 16/16 byte-equal.
- napicen_btl: no `battle/190B20.o` entry in the napicen lines of f600-f799, or 0 ops (was 4 per quad: 16 at f600).
- No `scomp: def` line in any of the 3 logs (a def the compiler refused).
- Budget 8 Azahar starts.

## Stop
A byte difference, a `scomp: def` line or 190B20.o above 0: stop, commit, hand back the lines (2 tries per named cause).
Delete this row file in the completing commit.
