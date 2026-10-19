# Plan for V16, V17 and V19 (read-only analysis, 2026-10-19)

Nothing ran. This file uses the two diag files, the rows in 3ds/SLICES.md and a read of the decomp and port code.

## V16 box around flickering sprites
- Most likely root cause: a translucent sprite draw (`RENDER_MODE_SURFACE_XLU_LAYER3`, actor_rendering.c:779) has no alpha cut in the port (`sync_render`, blend on, atest -1). A texel with RDP alpha 0 but port alpha above 0 then blends in and shows the quad. The combiner is already exact (PLAN-OK), so the texture alpha is the lead (lead 2 of V16_diag.md). The intro star box of f2070 is the tape graphic and is not this fault.
- Files and functions: 3ds/src/pm_tex.c (conversion of CI4, IA8, I4 for the box draw, force alpha 0 where the RDP alpha is 0). Second choice: 3ds/src/pm_gbi.c `sync_render`, the translucent branch (atest 0 with GPU_GREATER). Use the second choice only if the texture alpha is the proven cause.
- Step 0 (no Azahar): add one temporary `pm_log` in src/battle/actor_rendering.c at line 779 with the frame number. Do not commit it.
- Narrow test: `PM_ROW=V16d powershell -File 3ds/run_azahar.ps1 -Turbo -Timeout 400 -Until "NEVERMATCH" -Debug "fast preset=toad battle autoa shots shotwin=<f>:30 drawlog=<f>:2 rmlog"`, with `<f>` from the log line. 1 start, 30 frames. Find the `dd` entry with `oml` x4b4x, read its `fmt` and `comb`. After the fix: the same command, one shot, no box.
- Do not run two shot-writing tests at once (shared shots folder).

## V17 intro stars glow and partial fade
- Star glow: f2070 boxes are the tape strips (FadeAwayTapeGraphic, hos_05 intro.c). Sky stars at f3540 and f3610 show no box. Most likely not a fault. Action: compare one shot at f2070 with the decal of the real tape (no code change). If the user still sees a glow, the open lead is TEXEL1 (tile 1) in `pm_gbi.c` `G_SETTILESIZE` (tile 1 branch, about line 2035) with `ensure_aux` and `t1_fold`. Same cause as V21 (fire_breath). Then use the V21 test below.
- Partial white fade (the real open item): the viewport overlay reaches about 85 percent at f3620 and the title starts at f3626.
  Root cause options: the game ramp is short in the decomp (not a fault), or the port ends the map early.
  Files: src/state_intro.c (`INTRO_INIT` branch lines 126-141 and the hand-off at line 194), src/world/area_hos/hos_05/intro.c lines 1961-1977 (`func_80244934_A2EB74`, ramp (10 - n) * 25, ends at n = 10). That ramp is a 10 frame ramp from 255 down to 0 (fade out of white). The fade in to white is another script: grep `OVERLAY_VIEWPORT_COLOR` in hos_05/intro.c for the end script.
  Change: none yet. First add a temporary `pm_log` of the overlay alpha and `pm_frame_count` in `set_screen_overlay_params_back` and read the last value before `game mode 2`.
- Narrow test: `-Debug "fast" -Until "game mode 2 "` with the temporary log, 1 start, about 3650 frames, `-Turbo`. Pass if the last alpha is 255 or the decomp value.

## V19 Sushie shadows (jan_04)
- Most likely root cause: the shadow is an XLU decal (`oml 00504f50`, `G_RM_ZB_XLU_DECAL`). The port draws a decal with `GPU_GEQUAL` and a depth bias of 0.001 (`C3D_DepthMap(true, -1.0f, r_decal ? 0.001f : 0.0f)`, 3ds/src/pm_gpu.c line 317). A bias of 0.001 is below the float24 depth step at the camera distance of jan_04. The shadow z-fights with the floor or the log collider and flickers when the Sushie position changes by sub-pixel steps. HISTORY.md already lists "decal bias 0.001 below float24 resolution" as an unverified hardware-only cause. Dive under the log changes the raycast hit (`entity_raycast_down`, src/npc.c lines 713-735) between the log and the lake floor, which makes the shadow height jump.
- Files and functions: 3ds/src/pm_gpu.c `pm_gpu_flush` area (line 310-317, `C3D_DepthTest` and `C3D_DepthMap`). Fix candidate: a larger decal offset (try 0.005 and 0.01), checked on hardware too. Logic of the shadow itself (src/entity.c `render_shadows`, src/npc.c `disable_npc_shadow`, `enable_npc_shadow`) is the real decomp and is not the change place. Sushie calls `disable_npc_shadow` at ride start (sushie.c lines 418, 494) and `enable_npc_shadow` at exit (line 721).
- Not proven. If the shadow flicker shows as the shadow turning on and off (not shimmering), log `shadow->flags` ENTITY_FLAG_HIDDEN each frame for `npc->shadowIndex` of Sushie.
- Narrow test: `PM_ROW=V19 powershell -File 3ds/run_azahar.ps1 -Turbo -Timeout 300 -Debug "fast demoscene=5 shots shotwin=<f>:60"`, 1 start, 60 frames at the dive. Find `<f>` first from the warp log lines of jan_04. Pass if the shot series has no frame where the shadow disappears or jumps.
- Checked demoscene indices (`DemoScenes[]` in src/state_demo.c, counted in order):
  0 nok_12 world, 1 battle 0, 2 isk_04, 3 tik_03, 4 trd_09, 5 jan_04 (Sushie, entry 4), 6 battle 1, 7 mac_00, 8 kzn_02, 9 battle 2, 10 mac_03, 11 nok_02, 12 battle 3, 13 kkj_11 (Peach, confirmed by the run), 14 arn_08, 15 battle 4, 16 nok_12 (entry 3), 17 kkj_26 (Peach), 18 DONE (end).
  Index 5 is jan_04. Indices 8 to 12 and 14 to 17 follow the list order and are not run. Index 13 gave the Peach scene, which matches the list.
