# Row PAUSEOPEN: lag when the pause (Start) menu opens on O3DS (user, 2026-11-09)

## User facts
User (O3DS): "entering the start menu is laggy".

## Cause (shared oracle, code read on main 00a4e1901)
1. CPU readback of the colour buffer from VRAM at the pause start, about 30-50 ms in one frame on core 0: BEGIN_PAUSED VISCVG fill
   is ROLE_CPUFILL (nd_rect.c:278-280) -> pm_cfb_capture_prev_cpu_for -> cfb_capture_prev(cpu=1) (pm_main.c:264-300) ->
   pm_gpu_read_frame (pm_gpu.c:1801-1831): pm_gpu_begin_wait, then 320x240 (+80x240 wide) Morton reads, 3 uncached byte loads per pixel.
2. FILTER_PAUSED: gfx_transfer_frame_to_depth (background_gfx.c:262-308) on the logic thread, a plain copy on the 3DS (coverage
   0x8421), then pm_tex_invalidate of 150 KB and a 320x240 re-convert on core 0: about 4-8 ms + 3-6 ms.
Console checks: phase mode line "D03: last frame copied to a frame buffer in <us>" (pm_main.c:293), pm_b7.cap_ticks/cap_max;
fsplit pcopen slot 3 (background_gfx.c:391).

## Start state
`sh 3ds/tools/wt_new.sh PAUSEOPEN`, branch PAUSEOPEN. PM_ROW=PAUSEOPEN, PM_LANE=PAUSEOPEN. One Haiku agent. Launch after BATUI merges
(shared files nd_rect.c, pm_gpu.c).

## Fix
Under TARGET_3DS take the pause picture with the GPU copy the game-over blur already uses (pm_gpu_grab_prev into an off-screen
slot), not the CPU path. The ROLE_CPUFILL arm keeps writing 0x8421. FILTER_PAUSED skips gfx_transfer_frame_to_depth. SHOW_PAUSED
draws the grabbed slot with the same fade (gPauseBackgroundFade) and keeps the wide-side strips. The CPU readback path for the pause
is deleted (no toggle).

## Touch only
3ds/src/nd_rect.c (CPUFILL arm), pm_main.c (cfb_capture_prev), pm_gpu.c (grab), src/background_gfx.c (FILTER/SHOW_PAUSED, TARGET_3DS).

## Done criteria
1. nd_host PASS, memcheck OK.
2. pause_spin_grab_det_w300: the shot shows the grey paused picture behind the menu (subject); byte-equal not expected.
   Phase line D03 near 0 us. Budget: 4 Azahar starts.
3. Console (user): prof push, open the pause menu 3 times; D03 and pcopen in the pulled log.
4. Delete this row file in the completing commit.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
