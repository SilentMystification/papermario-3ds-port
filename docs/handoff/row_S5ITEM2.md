# Row S5ITEM2: render_item_entities and draw_coin_sparkles state lines as S6T compiled ranges (item_entity.c)

(Model: Haiku. Two functions in src/item_entity.c, the S5ITEM1 form. Blocker: S5ITEM1 merged (its def block with it_xlu_sh,
it_xlu and the nd_rmdef.h include). Re-grep the lines after the merge: the text of each line is given.)

## Code read (main 5c3865487, src/item_entity.c)
- render_item_entities :1132-: per item, :1174-1178 Gfx_RM2_ALPHATEST or Gfx_RM1_ALPHATEST, :1179 ClearGeometryMode(G_LIGHTING)
  (not CULL_BOTH as appendGfx_item_entity), :1180 gSPDisplayList(D_8014C620); :1192-1200 the same render mode pairs, combine
  and prim as appendGfx_item_entity :968-977 (equal text: S5ITEM1's it_xlu_sh and it_xlu fit).
- draw_coin_sparkles :173-: :195-207 CycleType(1CYCLE), ClearGeometryMode(the 9 flags of :196), SetGeometryMode(G_ZBUFFER |
  G_SHADE | G_LIGHTING | G_SHADING_SMOOTH), SetLights1(ItemEntityLights) (NUMLIGHT and 2 MOVEMEM: value units, class 2; ORACLE6
  value-order rule, row_S6VORD.md: not allowed between statics, so the range is two defs split right after the lights), Texture(-1, -1, 0, RENDERTILE, ON), TextureLOD, TexturePersp(PERSP), TextureFilter(BILERP), ColorDither
  (DISABLE), TextureDetail(CLAMP), TextureConvert(FILT), CombineKey(NONE), AlphaCompare(NONE). nv 3. The imgfx draw after stays.

## Start
`sh 3ds/tools/wt_new.sh S5ITEM2`. PM_ROW=S5ITEM2 PM_LANE=S5ITEM2 on every call.

## Touch only
src/item_entity.c: S5ITEM1's def block (append 2 defs at its end, before its `#endif`), render_item_entities :1174-1180 and
:1191-1201, draw_coin_sparkles :195-207. Nothing else.

## Exact edits (under `#ifdef TARGET_3DS`; `#else` keeps the old lines)
1. At the end of S5ITEM1's def block:
   ```c
   static const Gfx it_setup2_u[] = { gsSPClearGeometryMode(G_LIGHTING),
       gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON), gsDPSetCombineMode(G_CC_DECALRGBA, G_CC_DECALRGBA),
       gsDPSetTexturePersp(G_TP_PERSP), gsDPSetTextureDetail(G_TD_CLAMP), gsDPSetTextureLOD(G_TL_TILE), gsDPSetTextureLUT(G_TT_NONE),
       gsDPSetTextureFilter(G_TF_BILERP), gsDPSetTextureConvert(G_TC_FILT), gsDPSetTextureLUT(G_TT_RGBA16) };
   static const Gfx it_spark_a_u[] = { gsDPSetCycleType(G_CYC_1CYCLE),
       gsSPClearGeometryMode(<the :196 flags>), gsSPSetGeometryMode(G_ZBUFFER | G_SHADE | G_LIGHTING | G_SHADING_SMOOTH),
       gsSPSetLights1(ItemEntityLights) };
   static const Gfx it_spark_b_u[] = { gsSPTexture(-1, -1, 0, G_TX_RENDERTILE, G_ON), gsDPSetTextureLOD(G_TL_TILE),
       gsDPSetTexturePersp(G_TP_PERSP), gsDPSetTextureFilter(G_TF_BILERP), gsDPSetColorDither(G_CD_DISABLE),
       gsDPSetTextureDetail(G_TD_CLAMP), gsDPSetTextureConvert(G_TC_FILT), gsDPSetCombineKey(G_CK_NONE), gsDPSetAlphaCompare(G_AC_NONE) };
   static NdSDef it_setup2 = { it_setup2_u, 10 }, it_spark_a = { it_spark_a_u, sizeof it_spark_a_u / sizeof it_spark_a_u[0] },
       it_spark_b = { it_spark_b_u, sizeof it_spark_b_u / sizeof it_spark_b_u[0] };
   ```
   If ItemEntityLights is defined after S5ITEM1's block, move only the it_spark defs to right after its definition.
2. render_item_entities :1174-1180 ->
   `if (gSpriteShadingProfile->flags != 0) ND_RM_REC(Gfx_RM2_ALPHATEST); else ND_RM_REC(Gfx_RM1_ALPHATEST);`
   `nd_scomp_rec(&it_setup2, NULL, 0);`
3. :1191-1201 (the inner `if (item->flags & (TRANSPARENT | HIDING))` block) ->
   `if (gSpriteShadingProfile->flags != 0) nd_scomp_rec(&it_xlu_sh, NULL, 0);`
   `else { Gfx v[1]; gDPSetPrimColor(&v[0], 0, 0, 255, 255, 255, alpha); nd_scomp_rec(&it_xlu, v, 1); }` inside the same if.
4. draw_coin_sparkles :195-207 -> `{ Gfx v[3]; Gfx* p = v; gSPSetLights1(p++, ItemEntityLights); nd_scomp_rec(&it_spark_a, v,
   (unsigned)(p - v)); nd_scomp_rec(&it_spark_b, NULL, 0); }`. Expected nv 3 for it_spark_a (a `scomp: def ... has N value units` line means the macro wrote another count: stop).

## Gate (expected values)
- Speed rule (oracle 5): a bench list runs at one speed; spot_mac00_det is a turbo bench (bench_defs speed turbo), every
  other bench here is normal. Run it as its own list: `bench.py run spot_mac00_det --row S5ITEM2 --why "..."` (1 start).
- Build, memcheck OK, no warning in item_entity.c; `bench.py run nd_host --row S5ITEM2 --why "S5ITEM2 nd_host"`: PASS.
- `bench.py base spot_mac00_det`, `bench.py base spot_mac01_det`; one list `bench.py run spot_mac01_det,s6chk_mac01
  --row S5ITEM2 --why "S5ITEM2: item ranges 2"` (2 starts) plus spot_mac00_det alone (1 start). cmp 6/6 and 6/6 byte-equal; s6chk `bad 0`; hand back `ranges` next to
  the S5ITEM1 merge run (a reach report, not a stop). No `scomp: def` line. Budget 8.

## Stop
A byte difference, `bad` above 0 or a scomp line: stop, commit, hand back (2 tries per named cause). Delete this row file in the
completing commit.
