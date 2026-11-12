# Row S5BTL1: the tattle camera setup ranges as S6T compiled ranges (src/battle/16C8E0.c tattle_cam_pre_render)

(Model: Haiku. One function, two ranges, the cam_main def form (src/cam_main.c:115-143, :257-258). No blocker: SCOMP0 merged.
The background strips (:780-823) and the fills (:849, :856) stay ops: later rows (fill kind for FILL cycle, NdTRect).)

## Code read (main 5c3865487, src/battle/16C8E0.c)
- R1 :770-778 (inside the background branch): PipeSync, CycleType(COPY), TexturePersp(NONE), TextureLUT(RGBA16),
  CombineMode(G_CC_DECALRGB x2), RenderMode(G_RM_NOOP, G_RM_NOOP2), TextureFilter(POINT), Scissor(value: the cam viewport
  rectangle), PipeSync. 6 static units, nv 1.
- R2 :827-848: Viewport (value), ClearGeometryMode(the 8 flags), Texture(0, 0, 0, RENDERTILE, OFF), CycleType(1CYCLE),
  PipelineMode(NPRIMITIVE), ScissorFrac (value), TextureLOD(TILE), TextureLUT(NONE), TextureDetail(CLAMP), TexturePersp(PERSP),
  TextureFilter(BILERP), TextureConvert(FILT), CombineMode(G_CC_SHADE x2), CombineKey(NONE), AlphaCompare(NONE),
  RenderMode(G_RM_OPA_SURF x2), ColorDither(DISABLE), ClipRatio(FRUSTRATIO_2) (MW none: static no-op), PipeSync,
  CycleType(FILL), ColorImage (value: the z buffer), FillColor (value). nv 4. The FillRectangle (:849) and every line after it
  stay as they are.

## Start
`sh 3ds/tools/wt_new.sh S5BTL1`. PM_ROW=S5BTL1 PM_LANE=S5BTL1 on every call.

## Touch only
src/battle/16C8E0.c: a def block right before tattle_cam_pre_render (:732), the lines :771-777 and :827-848.

## Exact edits (under `#ifdef TARGET_3DS`; `#else` keeps the old lines)
1. Def block: `#include "nd_scomp.h"`; arrays (gs forms in the order above, PipeSync lines left out, value placeholders
   `gsDPSetScissor(G_SC_NON_INTERLACE, 0, 0, 0, 0)`, `gsSPViewport(0)`, `gsDPSetScissorFrac(G_SC_NON_INTERLACE, 0, 0, 0, 0)`,
   `gsDPSetColorImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, 0)`, `gsDPSetFillColor(0)`):
   `static const Gfx tc_copy_u[]` (7 entries) and `static const Gfx tc_cam_u[]` (all of R2 without the PipeSync; gsSPClipRatio
   may write several entries: count with sizeof). `static NdSDef tc_copy = { tc_copy_u, sizeof tc_copy_u / sizeof tc_copy_u[0] },
   tc_cam = { tc_cam_u, sizeof tc_cam_u / sizeof tc_cam_u[0] };`
2. :771-777 -> `{ Gfx v[1]; gDPSetScissor(&v[0], <the :777 arguments>); nd_scomp_rec(&tc_copy, v, 1); }`
3. :827-848 -> `{ Gfx v[4]; Gfx* p = v; gSPViewport(p++, PM_VP(&cam->vp)); gDPSetScissorFrac(p++, <the :832 arguments>);
   gDPSetColorImage(p++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, osVirtualToPhysical(nuGfxZBuffer));
   gDPSetFillColor(p++, PACK_FILL_DEPTH(G_MAXFBZ, 0)); nd_scomp_rec(&tc_cam, v, (unsigned)(p - v)); }`
   Expected nv: 1 and 4 (a mismatch logs `scomp: def ... has N value units`: stop).

## Gate (expected values)
- Build, memcheck OK, no warning in 16C8E0.c; `bench.py run nd_host --row S5BTL1 --why "S5BTL1 nd_host"`: PASS.
- No det scene opens the tattle camera (it needs a Goombario tattle in battle). Guard: `bench.py base battle_wheel_nowide_det`;
  `bench.py run battle_wheel_nowide_det --row S5BTL1 --why "S5BTL1: build guard"` (1 start): 1/1 byte-equal, no `scomp: def`
  line. The census of this row: none (the function does not run in the gate scene). Budget 3.

## Stop
A build error, a warning, a byte difference or a scomp line: stop, commit, hand back. Delete this row file in the completing
commit.
