# Row S6S1: gfx_init_state's two init lists as S6T compiled ranges

(Model: Haiku. It copies the S6T first use (src/cam_main.c:115-143, :258; src/status_icons.c:592-628). One function changes.)

## Pattern (S6T, merged b16f91c46; API 3ds/include/nd_scomp.h)
A def is `static NdSDef name = { units, count };` over typed gs units in stream order, with no End unit.
`nd_scomp_rec(&def, values, nv)` records it. `values` holds the value units of the def in order (viewport, scissor, matrix, colour
image, segment, fog and the like: S6T design note table, kind "value"). nv must equal the def's value count, else the record falls
back to plain ops with a `scomp: def ... has N value units` error line (nd_scomp.c:120, :135).

## Code read
gfx_init_state (src/background_gfx.c:35-39): `gSPSegment(0x00, 0)`, `gSPDisplayList(D_80074230)`, `gSPDisplayList(D_80074210)`.
- D_80074230 (:26-32): Viewport (value), ClearGeometryMode, SetGeometryMode, Texture (static), End.
- D_80074210 (:19-24): RenderMode, CombineMode, ColorDither (static), End.

## Start
`sh 3ds/tools/wt_new.sh S6S1`; PM_ROW=S6S1 PM_LANE=S6S1. Only src/background_gfx.c is edited (S5BG later edits other functions).

## Exact edits (src/background_gfx.c)
1. Below the two lists, under `#ifdef TARGET_3DS`: `#include "nd_scomp.h"`,
   `static NdSDef init_def_vp = { D_80074230, 4 };` and `static NdSDef init_def_rm = { D_80074210, 3 };` (counts without End).
   Check the NdSDef field order in nd_scomp.h. If `u` is `const Gfx*` and the lists are non-const Gfx, the initializer is fine;
   else add a cast.
2. gfx_init_state under `#ifdef TARGET_3DS`:
   - keep `gSPSegment(gMainGfxPos++, 0x00, 0x0);` (a value op);
   - replace the first gSPDisplayList with `nd_scomp_rec(&init_def_vp, &D_80074230[0], 1);` (its one value unit is the viewport
     unit 0 of the list);
   - replace the second with `nd_scomp_rec(&init_def_rm, NULL, 0);`.
   `#else` keeps the 3 old lines.

## Gate
- Build, memcheck OK; nd_host PASS.
- Bases at the merge-base, then one list:
  `bench.py run spot_mac01_det,mdl5_iwa10_det,spot_mac00_det --row S6S1 --why "S6S: init lists compiled"` (3 starts).
  `bench.py cmp` each: every shot byte-equal.
- `bench.py run s6chk_mac01 --row S6S1 --why "s6chk on the init ranges"` (1 start, the bench S6T added). Pass: every
  `s6chk f<n> ranges <r> bad <b>` line has b = 0.
- No `scomp: def` error line in the logs. Budget 5.

## Stop
A byte difference, an s6chk mismatch or a scomp error line: stop, commit, hand back. Delete this row file in the completing commit.
