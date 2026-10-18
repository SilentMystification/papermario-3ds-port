# V12 and V13 diagnosis (read-only)

Date: 2026-10-18. Method: code reading against the decomp. No build. No Azahar run.
Status words: LIKELY = code evidence, not yet run. OPEN = no single cause found.

## V12 damage star zoomed and clipped

### Cause (LIKELY)
The port ignores the tile shift fields of G_SETTILE (shifts, shiftt). The star display list sets both shifts to 15.

Evidence:
- Effect: `src/effects/damage_stars.c` `damage_stars_appendGfx` runs `D_090004C0_343500` and draws `D_090005E0_343620` (4 vertices).
- `assets/us/effects/gfx/D_090004C0_343500.gfx.inc.c`: `gsDPLoadTextureTile_4b(D_09000000_343040, G_IM_FMT_I, 32, 0, 0, 0, 31, 63, 0, MIRROR|CLAMP, NOMIRROR|CLAMP, 5, 6, 15, 15)`. The last two arguments are shifts = 15.
- RDP rule: shift 11 to 15 shifts the texture coordinate LEFT by (16 - n). Shift 15 doubles S and T. It applies in G_TL_TILE mode too.
- `assets/us/effects/gfx/D_09000400_343440.vtx.inc.c`: UV 0 to 1024 (32 texels). With the doubling, the quad covers 64 x 64 texels. S mask 5 with mirror makes the 32 wide image 64 wide. T mask 6 is 64. So the N64 shows the whole mirrored star.
- Port: `grep -i shift 3ds/src/pm_gbi.c 3ds/src/pm_tex.c` finds nothing. `G_SETTILE` (pm_gbi.c:1669) stores wrap and mask (`tile_wrap`) but drops bits 0-3 (shifts) and bits 10-13 (shiftt). `vert_uv` (pm_gbi.c:445) uses `tc / 32 - tile_x0` with no shift. The quad then shows only the top-left 32 x 32 texels of the 64 x 64 window: a quarter of the star, magnified 2x. This matches "zoomed in and clipped".

### Common cause: other effects
Same pattern (`gsDPLoadTextureTile*` with shifts 11 to 15) in 19 asset files. Effects hit: damage_stars, stars_spread, stars_orbiting, explosion, snowflake, big_snowflakes, snowfall, energy_orb_wave, radiating_energy_orb, bulb_glow, damage_indicator, ending_decals, radial_shimmer, fright_jar (found with grep over `assets/us/effects/gfx`). Model textures (`src/model.c`) use G_TX_NOLOD (shift 0) and do not change.
Not checked: shifts 1 to 10 (right shift) outside effects, `gsDPSetTile` calls with a nonzero shift in `src/` (grep for them first in step 1).

### Fix plan
Files: `3ds/src/pm_gbi.c` only.
1. Add `static unsigned tile_shift[8][2];` next to `tile_wrap` (line 129).
2. In `G_SETTILE` (line 1669) store `tile_shift[tile][0] = w1 & 15; tile_shift[tile][1] = (w1 >> 10) & 15;`.
3. Add `static float shift_mul(unsigned n) { return n == 0 ? 1.f : n <= 10 ? 1.f / (float)(1 << n) : (float)(1 << (16 - n)); }`.
4. In `vert_uv` (line 445) use the render tile (tile 0; `cur_tile` for tile 1): `*u = ((float)v->v.tc[0] / 32.f * shift_mul(tile_shift[0][0]) - (float)tile_x0) / (float)tw;` and the same for T with `tile_shift[0][1]`. The shift applies BEFORE the subtraction of the tile origin.
5. Do the same for `s0` and `t0` in the G_TEXRECT path (line 1635) only if a texrect uses a nonzero shift. Check with grep first. Otherwise leave it.
6. Flush before the change: G_SETTILE already calls `pm_gpu_flush()` for tile 0, so queued triangles keep the old shift.
Risk: any other draw with a nonzero shift changes. That is the correct RDP result. Check the list in step 1 of "Not checked".
Test: `python 3ds/tools/golden.py battle yard` (pixel diff must stay small), then one battle hit shot with `pwsh 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop` on the battle warp preset. Crop the star and look for a whole star. If no preset reproduces a hit, call `playFX`/the damage stars from a battle script is out of scope: use the battle preset and a pad script for one attack.

## V13 lava texture off

### Cause: OPEN (code reading finds no single proof). Ranked candidates
Lava is model texture panning, not a texrect or a special effect.
Path: `src/world/area_kzn/kzn_02/main.c` (also kzn_03, kzn_11, kzn_18, kzn_19, kzn_20, kzn_22) `EnableTexPanning` plus `TEX_PAN_PARAMS_*` `Exec(EVS_UpdateTexturePan)` (`src/world/common/atomic/TexturePan.inc.c`) writes `texPannerMainU/V, AuxU/V` (range 0 to 0x20000, always positive). `src/model.c:1957-1972` emits `gDPSetTileSize` for tile 0 and tile 1 with `pan >> 8` as the start (10.2 fixed point, 0 to 512 = 0 to 128 texels).

1. LIKELY: sub-texel scroll is lost. `pm_gbi.c` G_SETTILESIZE (line 1713, `x0 = uls >> 2` at about line 1816 and `td[1].x0 = uls >> 2`) keeps whole texels only. The pan step is 100 to 500 per frame in units of 1/256 of a quarter texel (0.4 to 2 quarter texels per frame at the script rate). The port moves the lava in 1 texel jumps. The N64 moves it smoothly with bilinear filtering.
2. LIKELY: filter. `pm_tex.c:413` forces `GPU_NEAREST` for every texture. Lava models use G_TF_BILERP (`src/model.c:3371` `filteringMode`). The N64 result is a smooth blend. The port shows blocky texels. This is shared by all textures, not only lava.
3. POSSIBLE: the aux tile (tile 1) offset. `dual_bind` (pm_gbi.c:827) computes the tile 1 offset from `td[0].x0 - td[1].x0`. It uses the same truncation as candidate 1, so the two layers drift against each other in 1 texel steps.
4. RULED OUT: negative pan. The S25 negative start code (pm_gbi.c:1722) is never reached because the pan values are positive.
5. RULED OUT: tile shift. Model textures use G_TX_NOLOD.

### Fix plan
Step 0 (diagnose, one Azahar run): `tour=kzn_02:<entry>` plus `shot` on the lava room. Compare to a decomp reference or to the user's description. Decide between candidates 1 and 2.
Step 1 (candidate 1): `pm_gbi.c` keep the fraction. Store `td[n].x0q = uls`, `y0q = ult` (quarter texels). Use `(float)uls / 4.f` instead of `uls >> 2` for `tile_x0`, `tile_y0` as floats. In `vert_uv` and `dual_bind` use the float origin. Do not use the float origin for the source pointer offset (keep the integer part for `src`).
Step 2 (candidate 2): in `pm_tex.c` set `GPU_LINEAR` when `omode_h` filter bits are G_TF_BILERP or G_TF_AVERAGE. Pass the filter into `pm_tex_set_wrap` or a new `pm_tex_set_filter`. Edge case: bilinear on clamped or masked textures needs the 1 texel border handled (check the star and HUD golden frames).
Test: `python 3ds/tools/golden.py yard toad battle` (no regression), one `tour` run through kzn_02, kzn_03, kzn_11, kzn_20, kzn_22 in one start, then one shot per lava map.
Other animated surfaces to test after the fix (same `EnableTexPanning` path): water in other areas (grep `EnableTexPanning` in `src/world/area_*`), conveyors. Not tested yet.
