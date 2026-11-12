# Row S6M6: hos_01 and hos_05 model custom gfx setups (combine PM_CC_05 + prim alpha) as one S6T compiled range each

(Model: Haiku. Two files, five two-line functions, one def per file. No blocker: SCOMP0 merged.)

## Code read (main 5c3865487)
- src/world/area_hos/hos_01/model_fx.c: N(setup_gfx_glowing_star) :37-40 and N(setup_gfx_giving_power) :49-52, each
  `gDPSetCombineMode(gMainGfxPos++, PM_CC_05, PM_CC_05); gDPSetPrimColor(gMainGfxPos++, 0, 0, 0, 0, 0, <alpha>);` with alpha
  N(GlowingStarAlpha), N(GivingPowerAlpha).
- src/world/area_hos/hos_05/model_fx.c: N(setup_gfx_light_beam) :55-58 (alpha `StarShrineLightBeamAlpha & 0xFF`),
  N(setup_gfx_starship_shimmer) :69-72 (N(StarshipShimmerAmt)), N(setup_gfx_unused) :83-86 (N(UnusedAlphaParam)).
- Classes (nd_scomp.c:16-27): CombineMode static, PrimColor ordered value after the last static (allowed). nv 1.
- These are custom gfx builders (the model path records them into a range that nd_range_run replays; scomp records work there,
  as S6M1 showed).

## Start
`sh 3ds/tools/wt_new.sh S6M6`. PM_ROW=S6M6 PM_LANE=S6M6 on every call.

## Touch only
The two model_fx.c files: a def block after the file's includes, and the five functions' bodies.

## Exact edits (under `#ifdef TARGET_3DS`; `#else` keeps the old lines)
1. After the includes of each file:
   ```c
   #ifdef TARGET_3DS
   #include "nd_scomp.h"
   static const Gfx N(cc05_u)[] = { gsDPSetCombineMode(PM_CC_05, PM_CC_05), gsDPSetPrimColor(0, 0, 0, 0, 0, 0) };
   static NdSDef N(cc05) = { N(cc05_u), 2 };
   #endif
   ```
2. Each function body becomes
   `{ Gfx v[1]; gDPSetPrimColor(&v[0], 0, 0, 0, 0, 0, <the same alpha expression>); nd_scomp_rec(&N(cc05), v, 1); }`.

## Gate (expected values)
- Build, memcheck OK, no warning in the 2 files; `bench.py run nd_host --row S6M6 --why "S6M6 nd_host"`: PASS.
- `bench.py base sprshade_hos01_pic` (hos_01, det, shots 200-205); `bench.py run sprshade_hos01_pic --row S6M6 --why "S6M6:
  hos model fx ranges"` (1 start): 6/6 byte-equal, no `scomp: def` line. hos_05 has no det scene: its proof is the def form and
  the build. Census: none (no napicen scene of hos_01). Budget 3.

## Stop
A build error, a warning, a byte difference or a scomp line: stop, commit, hand back. Delete this row file in the completing
commit.
