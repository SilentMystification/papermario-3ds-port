# Row S6M5: the sprite-imgfx setup range of KnockDownPlayer, trd_05 trap and trd_06 falling as one S6T compiled range each

(Model: Haiku. The pattern of row_S6M4.md without the viewport and projection units. No blocker: SCOMP0 merged.)

## Code read (main 5c3865487)
Each function starts with the same 13 lines: PipeSync (dropped), CycleType(1CYCLE), ClearGeometryMode(G_SHADE | G_CULL_BOTH |
G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_LOD | G_SHADING_SMOOTH), SetGeometryMode(G_ZBUFFER | G_SHADE |
G_SHADING_SMOOTH), Texture(0xFFFF or -1, same value), TextureLOD(TILE), TexturePersp(PERSP), TextureFilter(BILERP),
ColorDither(DISABLE), TextureDetail(CLAMP), TextureConvert(FILT), CombineKey(NONE), AlphaCompare(NONE): 12 static units, nv 0.
- src/world/common/complete/KnockDownPlayer.inc.c N(gfx_build_knocked_down_player) :14-28 (included by kkj_25 peach_free.c,
  mac_01 npc.c, obk_08 cabinets.c: one edit, three map TUs, each gets its own N() def).
- src/world/area_trd/trd_05/trap.c (function at :18-, lines :25-38).
- src/world/area_trd/trd_06/falling.c N(appendGfx_FallingSprite) :28-40.
The matrix, imgfx_appendGfx_component and gSPPopMatrix after them stay. The events are scripted (knock-down, trap, fall): no
det scene reaches them; the proof is the def and the build.

## Start
`sh 3ds/tools/wt_new.sh S6M5`. PM_ROW=S6M5 PM_LANE=S6M5 on every call.

## Touch only
The 3 files: a def block right before the function, and the 13 lines named above. Nothing else.

## Exact edits (each file; under `#ifdef TARGET_3DS`, `#else` keeps the old lines)
1. Before the function (copy the Texture first two arguments and both geometry masks from the file's own lines):
   ```c
   #ifdef TARGET_3DS
   #include "nd_scomp.h"
   static const Gfx N(spr_setup_u)[] = {
       gsDPSetCycleType(G_CYC_1CYCLE),
       gsSPClearGeometryMode(G_SHADE | G_CULL_BOTH | G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_LOD | G_SHADING_SMOOTH),
       gsSPSetGeometryMode(G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH),
       gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
       gsDPSetTextureLOD(G_TL_TILE), gsDPSetTexturePersp(G_TP_PERSP), gsDPSetTextureFilter(G_TF_BILERP),
       gsDPSetColorDither(G_CD_DISABLE), gsDPSetTextureDetail(G_TD_CLAMP), gsDPSetTextureConvert(G_TC_FILT),
       gsDPSetCombineKey(G_CK_NONE), gsDPSetAlphaCompare(G_AC_NONE),
   };
   static NdSDef N(spr_setup) = { N(spr_setup_u), 12 };
   #endif
   ```
   In trd_05 and trd_06 the def name is the same (N() makes it per map). If a name clash appears in a map that includes two of
   these files, stop and hand back.
2. The 13 lines become `nd_scomp_rec(&N(spr_setup), NULL, 0);` under TARGET_3DS.

## Gate (expected values)
- `sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK, no warning in the 3 files (mac_01 npc.c builds the
  KnockDownPlayer copy).
- `bench.py run nd_host --row S6M5 --why "S6M5 nd_host"`: PASS.
- `bench.py base spot_mac01_det`; `bench.py run spot_mac01_det --row S6M5 --why "S6M5: build guard"` (1 start): 6/6 byte-equal
  (mac_01 links the edited KnockDownPlayer copy; the knock-down event does not run). Budget 3.

## Stop
A build error, a warning in the 3 files or a byte difference: stop, commit, hand back. Delete this row file in the completing
commit.
