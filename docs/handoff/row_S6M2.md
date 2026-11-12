# Row S6M2: flo_10 water effect custom gfx as S6T compiled ranges (map custom gfx, file 2)

(Model: Haiku. One file, src/world/area_flo/flo_10/waterfx.c, function N(UnkModelFunc001) only. The form of row_S6M1.md: read it
first; its ranges R1 and R2 are the same unit kinds as here.)

## Blocker and base
SCOMP0 merged. Independent of S6M1 (other file). `sh 3ds/tools/wt_new.sh S6M2`; PM_ROW=S6M2 PM_LANE=S6M2.

## Code read (oracle 4, main 2fdcf9a8b; re-grep, lines may move)
N(UnkModelFunc001) (:98-259), inside `if (temp_f24 != temp_f20 || ...)` (:218):
- R1 :229-235: CycleType(1CYCLE), RenderMode(the Z_CMP | CVG_DST_CLAMP ... pair, :230-231), ColorImage(zbuffer) (value),
  CombineMode(PM_CC_PRIM_NO_ALPHA x2), PrimColor(248, 240, 240, 0) (value after the last static), PipeSync (no-op). nv 2.
- :237 gSPDisplayList(model list) stays an op.
- R2 :238-251: PipeSync, ColorImage(cfb) (value), DepthSource(G_ZS_PRIM), PrimDepth(32734, 0) (value), RenderMode(G_RM_ZB_XLU_DECAL
  pair), TextureFilter(POINT), TexturePersp(NONE), Texture(0xFFFF, 0xFFFF, 0, RENDERTILE, ON), TextureLUT(NONE),
  TextureDetail(CLAMP), TextureLOD(TILE), CombineMode(PM_CC_10 x2), PipeSync. nv 2.
- :253 N(UnkModelFunc000)(...) stays (its strips: prim, frame-buffer load, rectangle: draws, not this row).
- R3 :255-257: PipeSync, CycleType(2CYCLE), DepthSource(G_ZS_PIXEL). nv 0.
- src/world/dead/area_flo/flo_10/waterfx.c is the dead copy: do not edit it.

## Touch only
src/world/area_flo/flo_10/waterfx.c: N(UnkModelFunc001) and a def block above it; `#include "nd_scomp.h"` under `#ifdef TARGET_3DS`
after the includes. 3ds/tools/azt_scenarios.json and bench_defs.json: one entry each, inserted right after sbk56_orbdbg_det (not at
the file end; S1TSTEN and S6E2 insert after other entries).

## Exact edits (under `#ifdef TARGET_3DS`; `#else` keeps the old lines)
1. Def block: wfx_r1_u (5 units), wfx_r2_u (11), wfx_r3_u (2): the gs forms in order without the PipeSync lines, value slots as the
   S6M1 placeholders (`gsDPSetColorImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, 0)`, `gsDPSetPrimColor(0, 0, 0, 0, 0, 0)`,
   `gsDPSetPrimDepth(0, 0)`); one `static NdSDef` each.
2. Each range in the function: `{ Gfx v[2]; Gfx* p = v; <the value lines with p++ and their exact arguments>; nd_scomp_rec(&def,
   v, (unsigned)(p - v)); }`. Expected nv: 2, 2, 0.
3. Scenario and bench `flo10_det` (one commit): debug `warp=flo_10:0 shots shotwin=200:6 loglevel=debug det`, until
   `shot \d+ frame 205\b`, cap 25, deadline 20, expected_events: `warp: flo_10 entry 0` and the until line; bench kind azahar_pic,
   tags azahar, pic. Pre-flight: grep flo_10's scripts for `SetCustomGfxBuilders` with UnkModelFunc001 and name the model in the
   hand-back.

## Gate (expected values)
- Build, memcheck OK; `bench.py run nd_host --row S6M2 --why "S6M2"`: PASS.
- `PM_ROW=S6M2 PM_LANE=S6M2 python 3ds/tools/bench.py base flo10_det` (2 runs: a new bench; expected stored, 6 shots). One list:
  `bench.py run flo10_det,s6chk_mac01 --row S6M2 --why "S6M2 water effect ranges"` (2 starts). `bench.py cmp flo10_det --row
  S6M2`: every shot byte-equal; s6chk `bad 0`; no `scomp: def` line. Budget 6.

## Stop and end
A byte difference, `bad` above 0, a scomp error line, a base not stored, an edit that does not fit, or a test-tool fault: commit,
hand back the exact output. Delete this row file in the completing commit.
