# Row S5EMDL3: animator render-mode lists and the per-node state range as S6T compiled ranges (animator.c)

(Model: Haiku. One file, two functions. Blocker: S5EMDL1 merged (3ds/include/nd_rmdef.h, the Gfx_RM*_def objects in model.c).)

## Code read (main 5c3865487, src/animator.c)
- appendGfx_animator :783-853: 14 lines `gSPDisplayList(gMainGfxPos++, Gfx_RMk_NAME);` (:798-841) in the render-mode switch, then
  in the fog arm FogColor and FogPosition (:845-846). These two values stay ops: a def of value units only is refused
  (nd_scomp.c:63, `if (!seen) ok = 0`).
- appendGfx_animator_node :873-888, per node: matrix (stays), PipeSync, Texture(0, 0, 0, RENDERTILE, OFF), TextureLOD(TILE),
  TextureLUT(NONE), ClearGeometryMode(G_LIGHTING), SetGeometryMode(G_CULL_BACK) only with MODEL_ANIMATOR_FLAG_CULL_BACK, then
  CombineMode DECALRGBA (no fog) or PM_CC_42 / MULTIPLY_SHADE (fog), PipeSync. All static: 4 defs (cull x fog), nv 0.
- The vertex-buffer node arm (:904-1011) builds a heap list per frame: not touched (S7ENT).

## Start
`sh 3ds/tools/wt_new.sh S5EMDL3`. PM_ROW=S5EMDL3 PM_LANE=S5EMDL3 on every call.

## Touch only
src/animator.c: a block after its Gfx_RM extern lines (:39-), the 14 RM lines, :874-888. azt_scenarios.json and bench_defs.json:
one entry `s6chk_yard` each, right after `msgbox_talk_det` (a unique anchor: other rows insert after s6chk_mac01 and spot_sbk56_det).

## Exact edits
1. After the Gfx_RM externs:
   ```c
   #ifdef TARGET_3DS
   #include "nd_rmdef.h"
   #define AN_RM(l) ND_RM_REC(l)
   #define AN_NODE_HEAD gsSPTexture(0, 0, 0, G_TX_RENDERTILE, G_OFF), gsDPSetTextureLOD(G_TL_TILE), gsDPSetTextureLUT(G_TT_NONE),        gsSPClearGeometryMode(G_LIGHTING)
   static const Gfx an_n_dec_u[] = { AN_NODE_HEAD, gsSPSetGeometryMode(0), gsDPSetCombineMode(G_CC_DECALRGBA, G_CC_DECALRGBA) };
   static const Gfx an_n_dec_cull_u[] = { AN_NODE_HEAD, gsSPSetGeometryMode(G_CULL_BACK), gsDPSetCombineMode(G_CC_DECALRGBA, G_CC_DECALRGBA) };
   static const Gfx an_n_fog_u[] = { AN_NODE_HEAD, gsSPSetGeometryMode(0), gsDPSetCombineMode(PM_CC_42, PM_CC2_MULTIPLY_SHADE) };
   static const Gfx an_n_fog_cull_u[] = { AN_NODE_HEAD, gsSPSetGeometryMode(G_CULL_BACK), gsDPSetCombineMode(PM_CC_42, PM_CC2_MULTIPLY_SHADE) };
   static NdSDef an_n_dec = { an_n_dec_u, 6 }, an_n_dec_cull = { an_n_dec_cull_u, 6 }, an_n_fog = { an_n_fog_u, 6 },
                 an_n_fog_cull = { an_n_fog_cull_u, 6 };
   #else
   #define AN_RM(l) gSPDisplayList(gMainGfxPos++, l)
   #endif
   ```
   (No macro takes a combine mode as an argument: its commas would split it.) `gsSPSetGeometryMode(0)` sets no bit (geom = (geom & ~0) | 0): it equals the old path without the SetGeometryMode line.
2. `sed -i 's/gSPDisplayList(gMainGfxPos++, Gfx_RM/AN_RM(Gfx_RM/' src/animator.c`; `grep -c "AN_RM(Gfx_RM" src/animator.c`: 14.
3. appendGfx_animator_node :876-886 (Texture to the combine if/else; the two PipeSync lines may stay) under TARGET_3DS become
   ```c
       {
           const int cull = (animator->flags & MODEL_ANIMATOR_FLAG_CULL_BACK) != 0;
           nd_scomp_rec(!gAnimModelFogEnabled ? (cull ? &an_n_dec_cull : &an_n_dec) : (cull ? &an_n_fog_cull : &an_n_fog), NULL, 0);
       }
   ```
4. Scenario `s6chk_yard`: debug `preset=yard nowide uishow=talk shots shotwin=200:1 loglevel=debug det s6chk`, until
   `s6chk f2\d\d ranges \d+ bad \d+`, cap 30, deadline 25, expected_events only `warp: kmr_20 entry 4`; bench `s6chk_yard` as the
   s6chk_mac01 bench. Same commit as steps 1-3.

## Gate (expected values)
- Build, memcheck OK; `bench.py run nd_host --row S5EMDL3 --why "S5EMDL3 nd_host"`: PASS.
- `bench.py base msgbox_talk_det` (kmr_20: napicen_talk had animator.o 103 ops a frame) and `bench.py base s6chk_yard` (2 runs,
  new bench). One list `bench.py run msgbox_talk_det,s6chk_yard --row S5EMDL3 --why "S5EMDL3: animator ranges"` (2 starts).
  cmp msgbox_talk_det 1/1 byte-equal; s6chk_yard `bad 0` and `ranges` above the base log's value. No `scomp: def` line.
- Budget 8 Azahar starts.

## Stop
A byte difference, `bad` above 0, ranges not above the base, or a scomp line: stop, commit, hand back (2 tries per named cause).
Delete this row file in the completing commit.
