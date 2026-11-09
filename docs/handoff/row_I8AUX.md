# Row I8AUX: second-texture I8 alpha read as 1 on the console (user: Hurricane is a mixed blue rectangle, 2026-11-09)

## User facts
O3DS XL only: "Lakilester's Hurricane attack does not render properly: the texture is just a mixed blue rectangle."

## Cause (shared oracle, code read)
ProcessHurricane (src/battle/partner/lakilester.c:1370) -> fx_huff_puff_breath (:1405); huff_puff_breath.c:131-154, list D_09000400:
2-cycle, G_RM_PASS + G_RM_CLD_SURF2; colour (PRIM-ENV)*TEXEL0+ENV, alpha TEXEL1, cycle 2 alpha COMBINED*PRIM; two I8 32x16 tiles,
tile 1 at tmem 0x100 (LoadMultiTile). I8 converts to GPU_L8 (pm_tex.c:382), no alpha channel: the PICA samples alpha 1. pm_tex_i8(aux=1)
(pm_tex.c:142-145) is true only for an off-screen aux slot, so a CPU-converted I8 aux slot gives i8_1 = 0 (pm_gpu.c:1312, :3040) and the
TEV takes unit 1 alpha = 1: a solid blue quad. Unit 0 has the CPU I8 case (:145), unit 1 not. Console only (Azahar's L8 decode differs).
General: every two-texture intensity effect reading TEXEL1 alpha; the super block radial_shimmer is the same class (FX1 cause 2).

## Start state
`sh 3ds/tools/wt_new.sh I8AUX`, branch I8AUX. PM_ROW=I8AUX, PM_LANE=I8AUX. One coder agent.

## Steps
1. Log check (debug word i8aux, cold): at each draw with a unit-1 texture, print pm_tex_i8(1), the unit-1 slot fmt/siz and GPU format,
   once per 60 frames. Run fxtest for huff_puff_breath (pm_fxtest.c index) on mac_01: expect fmt I siz 8 with i8_1 0.
2. Fix (pm_tex.c pm_tex_i8): for aux, also return 1 when the bound aux CPU slot is live with FMT_I and SIZ_8 (the aux = 0 rule; aux slot
   index from pm_tex_cur_slots). I4 (LA4) unchanged.
3. Gates: nd_host PASS; the i8aux line shows i8_1 1; fx_direct_det vs stored base (expected to differ only on two-texture intensity
   effects: report the changed shots); spot_mac01_det byte-equal; memcheck OK. Console (user): Hurricane and the super block shimmer.
4. Commit; delete this row file in the completing commit. Budget: 4 Azahar starts.

## Touch only
3ds/src/pm_tex.c (pm_tex_i8), the debug line in pm_gpu.c, azt_scenarios.json, bench_defs.json.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
