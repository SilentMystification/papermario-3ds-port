# Row S6M4: the stone chomp ambush setup range of isk_05, isk_13 and isk_14 as one S6T compiled range each

(Model: Haiku. One pattern, three files with the same lines. The cam_main def form (src/cam_main.c:115-143, :257-258). No
blocker: SCOMP0 merged.)

## Code read (main 5c3865487)
In each file (src/world/area_isk/isk_05/npc.c N(func_80241610_97F0E0), isk_13/npc.c N(func_80241610_990DF0), isk_14/npc.c
N(func_80241610_993D40); all start at :22) the lines :29-50 are one state range:
Viewport (value), PerspNormalize only when not ortho (MW none: a static no-op, so the ortho camera runs the same range, as the
cam_main def notes), guMtxF2L of the projection (C, stays before the record), Matrix NOPUSH|LOAD|PROJECTION (value), PipeSync
(dropped), CycleType 1CYCLE, ClearGeometryMode(...), SetGeometryMode(...), Texture(-1, -1, 0, RENDERTILE, ON), TextureLOD,
TexturePersp, TextureFilter(BILERP), ColorDither(DISABLE), TextureDetail, TextureConvert, CombineKey(NONE), AlphaCompare(NONE)
(all static). nv 2. The modelview gSPMatrix (:62), imgfx_appendGfx_component and gSPPopMatrix (:76) stay as they are.
No scene reaches the ambush (a scripted event in Dry Dry Ruins): no byte gate; the proof is the def (the S6T compiler and s6chk
are proven on mac_01) and the build.

## Start
`sh 3ds/tools/wt_new.sh S6M4`. PM_ROW=S6M4 PM_LANE=S6M4 on every call.

## Touch only
The 3 npc.c files: a def block right before the function, and the lines :29-50 of the function. Nothing else.

## Exact edits (each file; under `#ifdef TARGET_3DS`, `#else` keeps the old lines)
1. Before the function:
   ```c
   #ifdef TARGET_3DS
   #include "nd_scomp.h"
   static const Gfx N(ambush_setup_u)[] = {
       gsSPViewport(0),
       gsSPMatrix(0, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION),
       gsDPSetCycleType(G_CYC_1CYCLE),
       gsSPClearGeometryMode(G_SHADE | G_CULL_BOTH | G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_LOD | G_SHADING_SMOOTH),
       gsSPSetGeometryMode(G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH),
       gsSPTexture(-1, -1, 0, G_TX_RENDERTILE, G_ON),
       gsDPSetTextureLOD(G_TL_TILE),
       gsDPSetTexturePersp(G_TP_PERSP),
       gsDPSetTextureFilter(G_TF_BILERP),
       gsDPSetColorDither(G_CD_DISABLE),
       gsDPSetTextureDetail(G_TD_CLAMP),
       gsDPSetTextureConvert(G_TC_FILT),
       gsDPSetCombineKey(G_CK_NONE),
       gsDPSetAlphaCompare(G_AC_NONE),
   };
   static NdSDef N(ambush_setup) = { N(ambush_setup_u), sizeof N(ambush_setup_u) / sizeof N(ambush_setup_u)[0] };
   #endif
   ```
   (Copy the ClearGeometryMode and SetGeometryMode masks from the file's own lines :39-41; they must be equal.)
   ORACLE6 (value order): the def has no gsSPPerspNormalize unit. It is a walker no-op (NDO_MW_NONE: set_mw default arm,
   nd_set.c:107-117), and in the def it counted as a static unit, which put the Matrix value between two static units (the
   S6VORD hazard). Without it Viewport and Matrix are both before the first static unit: nv 2, pre 2.
2. Lines :29-50 become, under TARGET_3DS:
   ```c
       guMtxF2L(cam->mtxPerspective, &gDisplayContext->camPerspMatrix[gCurrentCameraID]);
       {
           Gfx v[2];
           gSPViewport(&v[0], &cam->vp);
           gSPMatrix(&v[1], &gDisplayContext->camPerspMatrix[gCurrentCameraID], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
           nd_scomp_rec(&N(ambush_setup), v, 2);
       }
   ```

## Gate (expected values)
- `sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK, no warning in the 3 files.
- `bench.py run nd_host --row S6M4 --why "S6M4 nd_host"`: PASS.
- `bench.py base spot_mac01_det`; `bench.py run spot_mac01_det --row S6M4 --why "S6M4: build check"` (1 start): 6/6 byte-equal
  (no shared code changed: a guard against a build fault). Budget 3 Azahar starts.

## Stop
A build error, a warning in the 3 files or a byte difference: stop, commit, hand back. Delete this row file in the completing
commit.
