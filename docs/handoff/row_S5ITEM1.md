# Row S5ITEM1: world item entity state lines as S6T compiled ranges (appendGfx_item_entity)

(Model: Haiku. One function and a def block in src/item_entity.c. Blocker: S5EMDL1 merged (3ds/include/nd_rmdef.h, ND_RM_REC).
The texture loads and the draw stay ops here (S7ENT); render_item_entities and draw_coin_sparkles are rows S5ITEM2, S5ITEM3.)

## Code read (main 5c3865487, src/item_entity.c)
- appendGfx_item_entity :912-1061: matrix (stays); :958-962 Gfx_RM2_ALPHATEST (shading profile on) or Gfx_RM1_ALPHATEST;
  :963 ClearGeometryMode(G_CULL_BOTH | G_LIGHTING); :964 gSPDisplayList(D_8014C620) (:90-102: PipeSync, Texture(0xFFFF, 0xFFFF,
  0, RENDERTILE, ON), CombineMode DECALRGBA, TexturePersp PERSP, TextureDetail CLAMP, TextureLOD TILE, TextureLUT NONE,
  TextureFilter BILERP, TextureConvert FILT, TextureLUT RGBA16: all static); :966-979 transparent or hiding: shading on:
  RenderMode (the GBL_c2 pair, :968-970); shading off: RenderMode (:972-975), CombineMode(PM_CC_2F, PM_CC_32),
  PrimColor(255, 255, 255, alpha) (class 4 after the statics: allowed). Then the TLUT and tile loads (:981-), stay.
- One def holds :963 and the 9 units of D_8014C620 (copied in order); the render-mode list goes through ND_RM_REC.

## Start
`sh 3ds/tools/wt_new.sh S5ITEM1`. PM_ROW=S5ITEM1 PM_LANE=S5ITEM1 on every call.

## Touch only
src/item_entity.c: a def block right after D_8014C620 (:102), and appendGfx_item_entity :958-979. Nothing else.

## Exact edits (under `#ifdef TARGET_3DS`; `#else` keeps the old lines)
1. After `};` of D_8014C620:
   ```c
   #ifdef TARGET_3DS
   #include "nd_rmdef.h"
   static const Gfx it_setup_u[] = { gsSPClearGeometryMode(G_CULL_BOTH | G_LIGHTING),
       gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON), gsDPSetCombineMode(G_CC_DECALRGBA, G_CC_DECALRGBA),
       gsDPSetTexturePersp(G_TP_PERSP), gsDPSetTextureDetail(G_TD_CLAMP), gsDPSetTextureLOD(G_TL_TILE), gsDPSetTextureLUT(G_TT_NONE),
       gsDPSetTextureFilter(G_TF_BILERP), gsDPSetTextureConvert(G_TC_FILT), gsDPSetTextureLUT(G_TT_RGBA16) };
   static const Gfx it_xlu_sh_u[] = { gsDPSetRenderMode(<the :968-970 pair, copied>) };
   static const Gfx it_xlu_u[] = { gsDPSetRenderMode(<the :972-975 pair, copied>), gsDPSetCombineMode(PM_CC_2F, PM_CC_32),
       gsDPSetPrimColor(0, 0, 0, 0, 0, 0) };
   static NdSDef it_setup = { it_setup_u, 10 }, it_xlu_sh = { it_xlu_sh_u, 1 }, it_xlu = { it_xlu_u, 3 };
   #endif
   ```
2. :958-964 become
   `if (gSpriteShadingProfile->flags != 0) ND_RM_REC(Gfx_RM2_ALPHATEST); else ND_RM_REC(Gfx_RM1_ALPHATEST);`
   `nd_scomp_rec(&it_setup, NULL, 0);`
3. :966-979 become
   ```c
       if (item->flags & (ITEM_ENTITY_FLAG_HIDING | ITEM_ENTITY_FLAG_TRANSPARENT)) {
           if (gSpriteShadingProfile->flags != 0) nd_scomp_rec(&it_xlu_sh, NULL, 0);
           else { Gfx v[1]; gDPSetPrimColor(&v[0], 0, 0, 255, 255, 255, alpha); nd_scomp_rec(&it_xlu, v, 1); }
       }
   ```

## Gate (expected values)
- Speed rule (oracle 5): a bench list runs at one speed; spot_mac00_det is a turbo bench (bench_defs speed turbo), every
  other bench here is normal. Run it as its own list: `bench.py run spot_mac00_det --row S5ITEM1 --why "..."` (1 start).
- Build, memcheck OK, no warning in item_entity.c; `bench.py run nd_host --row S5ITEM1 --why "S5ITEM1 nd_host"`: PASS.
- `bench.py base spot_mac00_det`, `bench.py base spot_mac01_det`; one list `bench.py run spot_mac01_det,s6chk_mac01
  --row S5ITEM1 --why "S5ITEM1: item entity ranges"` (2 starts) plus spot_mac00_det alone (1 start). cmp 6/6 and 6/6 byte-equal; s6chk `bad 0`.
- Reach report (not a stop): s6chk_mac01 `ranges` next to main's last s6chk_mac01 run (runs.jsonl). Equal: no item entity ran by
  f200; hand back the values (the proof is then the def, the build and the two byte-equal scenes). No `scomp: def` line. Budget 8.

## Stop
A byte difference, `bad` above 0 or a scomp line: stop, commit, hand back (2 tries per named cause). Delete this row file in the
completing commit.
