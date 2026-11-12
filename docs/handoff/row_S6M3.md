# Row S6M3: mac_00 water custom gfx as S6T compiled ranges (map custom gfx, file 3)

(Model: Haiku. One file, src/world/area_mac/mac_00/water_gfx.c, function N(gfx_build_water) only. The form of row_S6M2.md and
src/background_gfx.c:36-51 (S6S1). No blocker: SCOMP0 is merged.)

## Code read (main 5c3865487, water_gfx.c:6-26)
- R1 :9-15: DepthSource(G_ZS_PRIM) (OML, static), PrimDepth(0x7FF0, 0) (value, class 2), RenderMode(the Z_CMP | CVG_DST_CLAMP |
  ZMODE_OPA | FORCE_BL pair, :11-12) (static), CombineMode(PM_CC_PRIM_NO_ALPHA x2) (static), PrimColor(0, 0, 0, 128, 0)
  (ordered value, class 4: after the last static unit, allowed), PipeSync (dropped). nv 2.
- :17-19 the model lookup and gSPDisplayList(model list) stay as they are.
- R2 :20-23: PipeSync (dropped), CycleType(G_CYC_2CYCLE), DepthSource(G_ZS_PIXEL), RenderMode(G_RM_PASS, G_RM_AA_ZB_XLU_SURF2):
  all static. nv 0.
- :25 mdl_draw_hidden_panel_surface stays.
- Classes from 3ds/src/nd_scomp.c:16-27. Values are constants here, but value units still come from the record (nv must equal
  the def's value count, nd_scomp.c:129).

## Start
`sh 3ds/tools/wt_new.sh S6M3`. PM_ROW=S6M3 PM_LANE=S6M3 on every call.

## Touch only
src/world/area_mac/mac_00/water_gfx.c (a def block before N(gfx_build_water), the function). 3ds/tools/azt_scenarios.json and
bench_defs.json: one entry `s6chk_mac00` each, inserted right after `s6chk_mac01` (not at the file end).

## Exact edits (under `#ifdef TARGET_3DS`; `#else` keeps the old lines)
1. After `MAP_RODATA_PAD(1,unk);`:
   ```c
   #ifdef TARGET_3DS
   #include "nd_scomp.h"
   static const Gfx N(water_r1_u)[] = {
       gsDPSetDepthSource(G_ZS_PRIM),
       gsDPSetPrimDepth(0, 0),
       gsDPSetRenderMode(Z_CMP | CVG_DST_CLAMP | ZMODE_OPA | FORCE_BL | G_RM_PASS,
                         Z_CMP | CVG_DST_CLAMP | ZMODE_OPA | FORCE_BL | GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)),
       gsDPSetCombineMode(PM_CC_PRIM_NO_ALPHA, PM_CC_PRIM_NO_ALPHA),
       gsDPSetPrimColor(0, 0, 0, 0, 0, 0),
   };
   static const Gfx N(water_r2_u)[] = {
       gsDPSetCycleType(G_CYC_2CYCLE),
       gsDPSetDepthSource(G_ZS_PIXEL),
       gsDPSetRenderMode(G_RM_PASS, G_RM_AA_ZB_XLU_SURF2),
   };
   static NdSDef N(water_r1) = { N(water_r1_u), 5 };
   static NdSDef N(water_r2) = { N(water_r2_u), 3 };
   #endif
   ```
2. N(gfx_build_water): lines :9-15 become, under TARGET_3DS,
   `{ Gfx v[2]; Gfx* p = v; gDPSetPrimDepth(p++, 0x7FF0, 0); gDPSetPrimColor(p++, 0, 0, 0, 0, 128, 0); nd_scomp_rec(&N(water_r1), v, (unsigned)(p - v)); }`
   and lines :20-23 become `nd_scomp_rec(&N(water_r2), NULL, 0);`. The model lines, the gSPDisplayList and
   mdl_draw_hidden_panel_surface stay unchanged between and after them.
3. Scenario `s6chk_mac00`: a copy of s6chk_mac01 with debug `warp=mac_00:0 pos=-135:0:85 shots shotwin=200:1 loglevel=debug det
   s6chk` (the spot_mac00_det position), first_marker and expected_events with `warp: mac_00 entry 0` only (both builds print it),
   until unchanged (`s6chk f2\d\d ranges \d+ bad \d+`). Bench `s6chk_mac00`: a copy of the s6chk_mac01 bench with that scenario.
   Same commit as steps 1-2.

## Gate (expected values)
- Build, memcheck OK; `bench.py run nd_host --row S6M3 --why "S6M3 nd_host"`: PASS.
- `bench.py base spot_mac00_det` and `bench.py base s6chk_mac00` (merge-base; the base of the new bench takes 2 runs). One list
  `bench.py run spot_mac00_det,s6chk_mac00 --row S6M3 --why "S6M3: mac_00 water ranges"` (2 starts).
  cmp spot_mac00_det 6/6 byte-equal. s6chk_mac00 head: `bad 0`, and `ranges` above the base log's value of the same line (the 2
  new ranges per drawn water frame). No `scomp: def` line in either log.
- Budget 8 Azahar starts.

## Stop
A byte difference, `bad` above 0, ranges not above the base, or a scomp line: stop, commit, hand back the lines (2 tries per
named cause). Delete this row file in the completing commit.
