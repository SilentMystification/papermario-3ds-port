# Row S5EMDL2: the entity model state lines around the render-mode list as S6T compiled ranges (entity_model.c)

(Model: Haiku. One file, two functions, defs and exact edits below. Blocker: S5EMDL1 merged (nd_rmdef.h included by
entity_model.c, EM_RM). Re-grep the line numbers after the merge; the text of each line is given.)

## Code read (main 5c3865487, src/entity_model.c)
- appendGfx_entity_model: pre range :366-371 (PipeSync, RenderMode TEX_EDGE, PrimColor 255x4, CombineMode MODULATEIA_PRIM,
  AlphaCompare NONE, SetOtherMode H 18 bits) when renderMode != SHADOW; after the list switch, in the fog arm :455-456 FogColor
  and FogPosition; then :459-466 ClearGeometryMode(G_LIGHTING) and the combine (MODULATEIA when no fog and not SHADOW, PM_CC_42 /
  MULTIPLY_SHADE with fog, none for SHADOW without fog); :479 the SHADOW combine after the setup callback.
- draw_entity_model_E: fog lines :797-798, then :801-806 ClearGeometryMode and the combine (MODULATEIA or PM_CC_42; no SHADOW case).
- Classes (nd_scomp.c:16-27): PrimColor is an ordered value (class 4): it must not sit between static units, so the def puts it
  first (it runs before the block; prim and the render mode are independent). FogColor (FOG) and FogPosition (MW FOG) are values.
- gSPSegment (:469, :808), the matrix, the list draw and the pops stay as ops (S7ENT moves them).

## Start
`sh 3ds/tools/wt_new.sh S5EMDL2`. PM_ROW=S5EMDL2 PM_LANE=S5EMDL2 on every call.

## Touch only
src/entity_model.c: a def block right after the S5EMDL1 macro block; appendGfx_entity_model :366-371, :455-456, :459-466, :479;
draw_entity_model_E :797-798, :801-806.

## Exact edits (under `#ifdef TARGET_3DS`; `#else` keeps the old lines)
1. Def block (TARGET_3DS):
   ```c
   static const Gfx em_pre_u[] = { gsDPSetPrimColor(0, 0, 0, 0, 0, 0), gsDPSetRenderMode(G_RM_TEX_EDGE, G_RM_TEX_EDGE2),
       gsDPSetCombineMode(G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM), gsDPSetAlphaCompare(G_AC_NONE),
       gsSPSetOtherMode(G_SETOTHERMODE_H, G_MDSFT_ALPHADITHER, 18, G_AD_DISABLE | G_CD_DISABLE | G_CK_NONE | G_TC_FILT | G_TF_POINT | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE) };
   static const Gfx em_plain_u[] = { gsSPClearGeometryMode(G_LIGHTING), gsDPSetCombineMode(G_CC_MODULATEIA, G_CC_MODULATEIA) };
   static const Gfx em_noncomb_u[] = { gsSPClearGeometryMode(G_LIGHTING) };
   static const Gfx em_fog_u[] = { gsDPSetFogColor(0, 0, 0, 0), gsSPFogPosition(0, 1000), gsSPClearGeometryMode(G_LIGHTING),
       gsDPSetCombineMode(PM_CC_42, PM_CC2_MULTIPLY_SHADE) };
   static const Gfx em_shadow_u[] = { gsDPSetCombineMode(PM_CC1_SHADOW, PM_CC2_SHADOW) };
   static NdSDef em_pre = { em_pre_u, 5 }, em_plain = { em_plain_u, 2 }, em_noncomb = { em_noncomb_u, 1 }, em_fog = { em_fog_u, 4 },
                 em_shadow = { em_shadow_u, 1 };
   static void em_tail(int fog, int comb) { /* S5EMDL2: ClearGeometryMode(G_LIGHTING) and the combine; fog: with the fog values first */
       if (fog) {
           Gfx v[2];
           gDPSetFogColor(&v[0], entity_fog_red, entity_fog_green, entity_fog_blue, entity_fog_alpha);
           gSPFogPosition(&v[1], entity_fog_dist_min, entity_fog_dist_max);
           nd_scomp_rec(&em_fog, v, 2);
       } else nd_scomp_rec(comb ? &em_plain : &em_noncomb, NULL, 0);
   }
   ```
2. appendGfx_entity_model :366-371 (inside `if (model->renderMode != RENDER_MODE_SHADOW)`): becomes
   `{ Gfx v[1]; gDPSetPrimColor(&v[0], 0, 0, 255, 255, 255, 255); nd_scomp_rec(&em_pre, v, 1); }`.
3. :455-456 (FogColor, FogPosition in `case true:`): no line under TARGET_3DS (the values move into em_tail).
4. :459-466 become `em_tail(entity_fog_enabled && !(model->flags & ENTITY_MODEL_FLAG_FOG_DISABLED), model->renderMode != RENDER_MODE_SHADOW);`
5. :479 (`gDPSetCombineMode(gMainGfxPos++, PM_CC1_SHADOW, PM_CC2_SHADOW);`) becomes `nd_scomp_rec(&em_shadow, NULL, 0);`.
6. draw_entity_model_E :797-798: no line under TARGET_3DS; :801-806 become
   `em_tail(entity_fog_enabled && !(model->flags & ENTITY_MODEL_FLAG_FOG_DISABLED), 1);`

## Gate (expected values)
- Build, memcheck OK, no warning in entity_model.c; `bench.py run nd_host --row S5EMDL2 --why "S5EMDL2 nd_host"`: PASS.
- `bench.py base spot_mac01_det`, `bench.py base mdl5_iwa10_det`; one list `bench.py run spot_mac01_det,mdl5_iwa10_det,s6chk_mac01
  --row S5EMDL2 --why "S5EMDL2: entity state lines as defs"` (3 starts). cmp 6/6 and 6/6 byte-equal; s6chk `bad 0`, `ranges` above
  the S5EMDL1 merge run of s6chk_mac01 (runs.jsonl). No `scomp: def` line. Budget 8.

## Stop
A byte difference, `bad` above 0 or a scomp line: stop, commit, hand back (2 tries per named cause). Delete this row file in the
completing commit.
