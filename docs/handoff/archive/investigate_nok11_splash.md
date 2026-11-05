# investigate_nok11_splash: New 3DS hard hang on the Jr Troopa water splash (nok_11)

Read-only. No source edit. Branch native, HEAD 8fca53deb. Azahar starts used: 0 (see section 6).
Tags: CONFIRMED (source line read) or INFERRED (reasoning, not measured on hardware).

## 1. Cause (INFERRED)

The splash draws 40 particles. Each particle reads a 32 by 32 window of the live colour buffer as a texture. Each such read is a new grab (copy of the colour buffer, GPU command list split, extra quad). No cap limits the grabs per frame. Six splash instances can live at once, so the worst case is about 240 grabs, 240 command list splits and about 72 MB of VRAM copy per frame. The hang happens in this load. The exact limit that fails (command buffer space, split count or GPU wait) is not measured.

## 2. Call chain (CONFIRMED by source)

| Step | Location | Fact |
|---|---|---|
| Script | src/world/area_nok/nok_11/npc.c:91-107 | Thread runs 6 `PlayEffect(EFFECT_WATER_SPLASH, 2, x, y, z, Float(0.7), 30)`. Arg0 = 2, lifetime 30. |
| Part count | src/effects/water_splash.c:33 | `case 2: numParts = 41;` (particle 0 is the centre, 40 particles draw). |
| Window | src/effects/water_splash.c:191-192 | `temp_t2 = particle.x + spawn.x`, `temp_t3 = particle.y + spawn.y`, then masked to 7 bits (x) and 6 bits (y). Each particle gets its own window. |
| Load | src/effects/water_splash.c:206 | `gDPLoadMultiTile(... OS_K0_TO_PHYSICAL(nuGfxCfb_ptr), 0x100, 1, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, 0, ...)`. RGBA16 (fmt 0, siz 2) from the frame buffer being drawn. |
| Bind | src/pm_gbi.c:2566 (grab) and 2582 (`ens_src = td[n].src`) | `if (td[n].fmt == 0 && td[n].siz == 2) grab_cur(...)`. tile_bound (pm_gbi.c:2485) matches the window source pointer, so a new window is a miss. |
| Grab | src/pm_gbi.c:266-273 (`grab_cur`) | `pm_is_cur_cfb(src)` is true, so it calls `pm_gpu_grab(nuGfxCfb_ptr, y, y+h)`. |
| Grab body | src/pm_gpu.c:1826-1880 (`grab_impl`) | Reuse only if no draw dirtied the rows (line 1836). Else: `flush()`, `C3D_FrameSplit` (line 1851), `GX_TextureCopy` of 40 strips of 7680 B = 314880 B (lines 1803-1807), two quad draws (lines 1865-1866). |
| No cap | src/pm_gpu.c:297 and 1838 | `GRAB_MAX = 8` only prints a warning (`B2: %d grabs in one frame`). Grabs continue. |

Splash particles draw blended into the same screen area, so each part dirties rows. The reuse check at line 1836 then fails for most parts. This is INFERRED for the hardware run. It is not measured.

## 3. Why the hang is plausible (INFERRED)

- Each split is one GPU command list chunk. citro3d `GPUCMD_Add` has a bound check (`off+1+n > gpuCmdBufSize` calls `svcBreak`), see docs/handoff/inv_citro3d_decode.md:37. The buffer size is not read here (no citro3d headers on this PC).
- `gspWaitForP3D` has no timeout (docs/handoff/agent_jumptest.md:37, pm_gpu.c jt_result). A GPU that stalls blocks the game thread with no line.
- pm_wdog (src/pm_wdog.c) ends the process only from its own thread. A kernel or GSP stall can stop the console without a Luma dump. This fits the report.
- N3DS-specific code does not touch this path. `pm_new3ds` is used only in input and log (pm_input.c, pm_main.c:581-635). The N3DS difference is speed: more grabs per second of wall time.

## 4. Azahar comparison (INFERRED)

Azahar emulates the GPU and the command list in software. It has no real command buffer size and no GSP wait. The same grab count runs as slow frames, not as a hang. Expected result: Azahar does not hang. Not verified here.

## 5. Checked and not found

- Texture size and index overflow: the window is 32 by 32 (`lrs-uls = 31`, `lrt-ult = 31`). Copy offsets stay inside grab_tex (BLK_FULL, pm_gpu.c:352) and the colour buffer (strips 5 to 45 of 50).
- VRAM alloc failure: `copy_get` (pm_gpu.c:369-378) and `offs` allocation fail softly. The load then reads frame buffer memory (pm_gpu.c:1843). No hang from this path.
- Loops with bad bounds: the particle loops (water_splash.c:68, 134, 190) use `numParts`, fixed at 41.
- Record replay (UC3e, pm_gbi.c:85 `rec_gdl`): replay is off by default (commit 702862635). Not checked on the console build the user runs.

## 6. Azahar starts

None. The splash path needs a 3dsx build from the Docker image, and this worktree has an uncommitted src/map_restore_table.c. No nok_11 scenario exists in 3ds/tools/azt_scenarios.json. The grab count can be measured with the `grablog` switch (pm_gpu.c:1820-1824) on a run that reaches the splash. That is the next step.

## 7. Fix proposal

- Smallest change, one place: in `grab_impl` (pm_gpu.c:1834), when `grab_n >= GRAB_MAX` and not `prev_frame`, return 0. The caller then reads frame buffer memory, the same fallback as pm_gpu.c:1843. Cost: the splash shows last-frame pixels for the extra parts.
- Alternative: one grab of rows 0 to 240 per frame for the splash (the `pm_gpu_grab(nuGfxCfb_ptr, 0, 240)` call pattern at pm_gpu.c:1998). Larger change in water_splash.c.
- Test before choosing: count grabs per frame. Other effects may use more than 8 grabs per frame (B2 notes say 8 was a log level, not a limit), so check them too.

## 8. Test scenario (to create, not created here)

- New scenario `nok11_splash` in azt_scenarios.json and the catalog table, same commit. Warp to nok_11 at the Jr Troopa encounter (warp args not checked here). Switch `grablog`.
- Quantity: grab lines per frame during the splash (expect up to 40 per instance). Until: a `grab` log line after the `PlayEffect` event.
- Picture gate: the splash shows (a person reads the shot). Log gate: count of `B2: ... grabs in one frame` lines.
- Console gate: push_3ds.sh with the splash, and a hardware log line with the grab count per frame.
