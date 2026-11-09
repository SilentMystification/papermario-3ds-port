# Row NEGPAN: textures with a negative tile offset are frozen at offset 0 (user: mim_05 mushrooms do not light up, 2026-11-09)

## User facts
"The mushrooms that light up in mim_05 to find the correct path do not actually light up, the texture stays the same."

## Cause (shared oracle, code read)
mim_05 guide mushrooms (mim_05/hint.c:98-110) use mim_dark_kinoko2tif CI8 32x64 wrap/wrap ALPHATEST, UVs t 32..54 (dark half).
Inspecting calls SetTexPanOffset(panner, MAIN, 0, -0x8000) (hint.c:19-21, :72-81): model.c:2160 panMainV = -128 (10.2) = -32
texels, SetTileSize ult = -128 & 0xfff = 3968; the RDP reads (t - ult) through the T mask 64, so the lit upper half shows.
Port: set_tilesize (nd_tex_set.c:179) -> tex_win_s25 (:556-566) clamps a negative start to 0, so the -32 offset is lost.
General: every texture with a negative panner value or negative scroll (mac_05 water aux -70/-50, other scrolls) is frozen at 0.
Possibly also the super block shimmer (radial_shimmer scrolls its tiles; row SBLK2 found its TEV and blender equal to the N64).

## Start state
`sh 3ds/tools/wt_new.sh NEGPAN`, branch NEGPAN. PM_ROW=NEGPAN, PM_LANE=NEGPAN. One coder agent.

## Steps
1. Check: one cold log line in tex_win_s25 (debug word negpan) printing raw uls/ult and the clamped values. In mim_05 (new scenario:
   warp mim_05, walk to a mushroom group, A to inspect; or force SetTexPanOffset by a debug word) expect st = -32 -> 0.
2. Fix (nd_tex_set.c tex_win_s25): for a wrap or mirror tile with a mask (tile_wrap[0][2]/[3] nonzero, clamp bit clear), map the
   signed start into the mask period (& (period-1) for wrap, the mirrored period for mirror); keep the window size; x0/y0
   (set_tilesize :192-197) and tile_x0/y0 carry it into uvso (nd_gv_params). Clamp tiles keep the clamp to 0. Host test: a wrap tile
   with start -32, mask 64 -> start 32.
3. Gates: nd_host PASS; mim_05 inspect picture (mushroom lit); mac_05 water scroll (mac05_water_census bench, picture changes
   between two frames); byte-equal spot_mac01_det; memcheck OK. Also one sbk56_superblock shot at the shimmer for the user to compare.
4. Commit; delete this row file in the completing commit. Budget: 6 Azahar starts.

## Touch only
3ds/src/nd_tex_set.c, host test, azt_scenarios.json, bench_defs.json.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
