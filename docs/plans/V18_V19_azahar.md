# V18 and V19: what Azahar can show (read-only analysis, 2026-10-19)

## Depth numbers
- Game near 8, far 16384 (src/cam_main.c:274,275). Some maps change them. Not checked per map.
- pm_gpu.c:97-102 maps clip z to z' = 0.5 z/w + 0.5, range 0 near to 1 far. z' = 0.5 + 0.5 ((f+n)/(f-n) - 2fn/((f-n)d)).
- Slope: dz'/dd = fn/((f-n) d^2) = 8.004 / d^2. World step for a depth step dz: dd = dz d^2 / 8.004.
- Buffer: 24 bit (pm_gpu.c:537). Its step 2^-24 is 6e-8. This is not the limit.
- Limit: the PICA vertex shader outputs float24 (16 bit mantissa). z' near 1 has a step of 2^-17 = 7.6e-6. At d 500: 0.24 units. At d 1000: 0.95 units.
- N64 for comparison: 15 bit prim depth (pm_gbi.c:82), step 3e-5 (0.95 units at d 500, 3.8 at d 1000). The N64 is coarser than the PICA.
- Decal bias: pm_gpu.c:387 adds 0.001. That is 11 units at d 300, 31 at d 500, 125 at d 1000. It is 130 times the float24 step.

## Finding
- The earlier lead ("bias below the resolution") is the wrong way round. The bias is far above the resolution.
- The shadow or the decal wins GEQUAL against every surface within 31 units behind it (at d 500). Where two surfaces lie within that gap, the winner depends on the sub-pixel vertex rounding, so the edge toggles with a small camera move. This is the flicker.
- V19: Sushie dives under the log and the water. The log and water surfaces lie within 31 units of the shadow decal.
- V18: the sanctuary walkway lies a few units from the ice lattice below it at d 800 to 1500 (bias 80 to 280 units). Only if a decal draws there. Not proven: list the decal draws in the window (oml with Z_DEC).

## Warps
- V19: `PM_ROW=V19 3ds/run_azahar.ps1 -Turbo -Debug "fast fixedseed demoscene=5 preset=toad shots shotwin=225:30"`. The dive is f198 to f250, the camera is under water from f233.
- V18: `PM_ROW=V18 ... -Debug "fast fixedseed shots shotwin=3490:40"`. The pan ends near f3530 (V17d series f3364 to f3530).

## Code fix
- pm_gpu.c:387: replace 0.001f by a static float read once from `pm_debug_value("decalbias")`, default 0.00003f (4 float24 steps, 0.9 units at d 500).
- No other file changes. Keep GPU_GEQUAL (pm_gpu.c:380).

## Azahar test (one start per bias, 3 starts)
Azahar is deterministic. The bias is a depth input, so it shows on screen as a show-through area, not as time flicker.
1. Run the V19 line with `decalbias=0.001`, `decalbias=0.00003` and `decalbias=0`. Same frames, `fixedseed`.
2. Script 3ds/tools/decal_diff.py (new, 15 lines, PIL and numpy): for each frame n, count pixels with abs diff over 24 between bias 0.001 and bias 0.00003. Count the same for bias 0 against 0.00003.
3. Pass: bias 0.00003 against 0 differs under 30 pixels per frame (the shadow still draws). Bias 0.001 against 0.00003 differs by many pixels in the dive frames f233 to f250. That proves show-through.
4. Camera test: the V18 line with a camera pitch of +0.05 degree (a one-line debug add to the lookat) gives frame B. Diff A against B at bias 0.001 and at 0.00003. A z-fight shows as isolated pixels that change far above the plain motion diff.
5. If step 3 shows no difference, the cause is not the bias. Next: `depthview=3` on the device.

## Limits
- Azahar uses a host GL depth buffer, not float24 vertex output. It may hide small z-fights. Bias differences above 1e-4 do show.
- The user must confirm on the O3DS and N3DS.
