# Diagnosis: PRIMZ (toad pond water shows the background)

Cause NOT confirmed. Ranked candidates below. No game source edit. No Azahar run. Not read: toad camera, citro3d mapping, Azahar depth. Band computed, not measured.

## Code facts
- [C] water_gfx.c:8-9: G_ZS_PRIM, prim depth 0x7FF0 (32752).
- [C] water_gfx.c:10-11: pass 1 has Z_CMP, no Z_UPD. Blocker tests depth, writes colour only (pm_gpu.c:740-743, GREATER).
- [C] water_gfx.c:20-21: pass 2 tests the stored depth, not the blocker depth.
- [C] pm_gbi.c:3640: prim_z = (w1 >> 16) & 0x7fff.
- [C] pm_gbi.c:1231 (zmix) and :1886 (CPU): zn = 2*prim_z/32767 - 1, z = zn * w.
- [C] shaders/pm4.v.pica: outpos.z = zmix.y * w, outpos.w = w. Float24.
- [C] pm_gpu.c:1220: DEPTH24_STENCIL8. Clear 0 (282, 555). Depth map at 749.
- [C] Near 8, far 16384 (cam_main.c:300-301). Toad camera not read [E].

## Window depth mapping [E]
- Assumed: d24 = round((1 - z_ndc) / 2 * 16777215). Near = max, far = 0. Fits GREATER and clear 0. citro3d not in tree.
- Near 8, far 16384: z_ndc = 1.000977 - 16.00977 / eye_distance [C].

## Numbers
| prim z | zn | eye distance | d24 (assumed) |
|---|---|---|---|
| 0x7FFF | 1.000000 | 16384 | 0 |
| 0x7FF0 | 0.999084 | 8458 | 7680 |
| 0 | -1.000000 | 8 | 16777215 |

- [C] One prim unit = 512 d24 units.
- [E] float24 rounding of zn: about +-64 d24 units.
- [C] Blocker vs far background (d24 0): margin 7680. Passes exact and with 1-ulp error.
- [C] Blocker fails any stored depth at d24 >= 7680, i.e. geometry nearer than eye 8458.
- [E] Rounding flips the test only for stored d24 7616 to 7744 (eye 8424 to 8492).
- [E] Overlay at eye 200: d24 about 663000. It does not meet the blocker.

## Console vs Azahar [E]
- Console: 24-bit depth, float24 vertex z, strict GREATER, ties fail.
- Azahar: host GPU, float depth, exact compare (assumed, not read).

## Ranked candidates
1. [E] Stored depth in the band (eye 8.42k to 8.49k). Console rounding flips the blocker. Fits "Azahar correct, console wrong".
2. [E] Stored depth exactly 7680 from an earlier 0x7FF0 pass. Strict tie fails on console.
3. [E] Stored depth nearer than eye 8458. Blocker fails by design. Azahar would match. Low fit.
4. Rejected: blocker behind the far background.

## Smallest fix (not applied)
- Fix A: one helper for pm_gbi.c:1231 and :1886 using prim_z - 1 (512 d24 nearer), clamped at 0. Prim users at :3674, :3742 need the same.
- Fix B (candidate 2 only): GEQUAL for the blocker, pm_gpu.c:741-743.

## Azahar check (later row, one quantity per shot)
- Debug switch: round zmix.y to 2^-16 steps in pm4.v.pica. Scenario toad_pond (azt_scenarios.json:1040).
- Candidate 1 or 2: pond shows the background colour, as on console. Candidate 3: background at any prim value near 0x7FF0.
