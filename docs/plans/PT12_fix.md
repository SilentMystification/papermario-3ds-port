# PT12: battle transition is laggy. Is the blur and overlay on the GPU?

Status: cost measured, root cause proven. Fix not applied (pm_main.c, pm_gpu.c, pm_gbi.c are main-owned).

## Answer to the question
The draw is on the GPU. The picture that the blur reads is not. Per transition frame the port reads the last finished frame back with the CPU (D03, one detile of 320 x 240), converts it to N64 RGBA5551 in RAM, and then uploads it again as 33 to 40 separate textures of 320 x 6 pixels. So the overlay mesh and the strip draws run on the GPU, and the blur source makes a GPU to CPU to GPU round trip.

## What the transition draws
- src/encounter.c:1530 to :1545 sets OVERLAY_START_BATTLE with fadeOutAmount (accel 1 to 10 per frame, about 30 frames).
- src/screen_overlays.c:322 to :344: a rotating scaled mesh (D_8014E8F0), a full screen mesh (D_8014E9A8), and then `draw_prev_frame_buffer_at_screen_pos(0, 0, 319, 239, alpha)` (the blur, alpha = progress + 40, at most 170).
- src/screen_render_util.c:30 to :77: 40 strips. Each strip is gDPLoadTextureTile of 6 rows of the previous frame buffer plus one texrect.
- Port: pm_gbi.c:2350 (G_SETTIMG on the old frame buffer) calls pm_cfb_capture_prev (pm_main.c:215 to :230) once per frame. It calls pm_gpu_read_frame (pm_gpu.c:967 to :987): two nested loops, 76,800 pixels, a Morton index and a 4 byte read of VRAM per pixel, then pm_tex_invalidate of the whole buffer. Every strip then misses the texture cache (pm_tex_load, one decode and upload of 320 x 6 texels per strip) and ends the draw batch (pm_gpu_flush at pm_gbi.c:2593).

## Numbers (Azahar, row INVFX start 3, 3ds/logs/perf/invfx/s3/log.txt)
Scene: `fast fixedseed preset=toad loglevel=debug phase phasecsv prevtest stats`. The switch prevtest runs the real OVERLAY_BLUR (type 12, alpha 200, window 20,20 to 300,220, about 33 strips) every frame from scene frame 100 (pm_warp.c:473). Averages per drawn frame, ARM11 tick time in ms:

| | frames 30 to 99 (no blur) | frames 130 to 479 (blur) | change |
|---|---|---|---|
| wall | 18.78 | 47.95 | +29.17 |
| fbread (CPU detile, phase `rd`) | 0.00 | 8.11 | +8.11 |
| tex (decode and upload) | 0.56 | 7.97 | +7.41 |
| gbi (one batch per strip) | 9.88 | 12.29 | +2.41 |
| fbegin (wait for the GPU of the last frame) | 3.79 | 13.35 | +9.56 |
| texture misses per frame / KB per frame | 2 / 13 | 32 / 255 | |
| draws per frame | 58 | 109 | |

b7 line: `swirl capture 30 x 260,000 to 262,700 us max 8,766 us` per 30 frames, so 8.7 ms per capture, one capture per frame.
- CPU side of the blur: 8.11 + 7.41 + 2.41 = 17.9 ms of the 29.2 ms. The GPU wait adds 9.6 ms.
- O3DS hardware, same code: 3ds/logs/perf/hwm1_A.txt (MODE O3DS, build 907) b7 lines give `swirl capture ... max 8766 us` and 8,659 to 8,766 us per capture. The tick values match Azahar, so the ratio 3.5 does not apply to the tick numbers. A frame of 33.3 ms with a normal 18.8 ms cost plus 17.9 ms of CPU blur work and 9.6 ms of GPU wait gives about 46 ms (22 fps) for each of the 30 transition frames. The real START_BATTLE draw has 40 strips and two extra meshes, so it costs a little more than prevtest.

## Fix (design, not tested)
Row O10 of docs/plans/post_po3_path.md (A20, line 196) says "GPU path". Use the grab code that already exists (pm_gpu_grab, pm_gpu.c:1075 to :1135, role T2) for the old frame buffer (role T3):
1. In pm_cfb_capture_prev (pm_main.c:215) call a new pm_gpu_grab_prev() instead of pm_gpu_read_frame. In pm_gpu.c queue `GX_TextureCopy(target->frameBuf.colorBuf, ..., grab_tex.data, ..., 240 * 400 * 4 bytes, BIT(3))` before the first split and before the lazy clear of the frame (the queue runs in call order, see the comment at pm_gpu.c:1066 to :1074). The colour buffer still holds the last finished frame at that moment because the frame list has not run.
2. Draw the copy upright into an off-screen slot keyed to the old frame buffer address, as pm_gpu_grab does (quad with the rotated uv, pm_gpu.c:1115 to :1125). Use pm_gpu_off_begin(prev_cfb, 320, 2) so that the 40 strip loads find the slot like any T1 image. No decode, no upload, no pm_tex_invalidate.
3. Keep pm_gpu_read_frame only for game code that reads the pixels with the CPU (gfx_transfer_frame_to_depth, src/background_gfx.c:343: the pause background). Test with `nopg` style switch `prevcpu` to compare.
4. Expected gain: fbread 8.1 ms and tex 7.4 ms go to near 0. The strips stay 33 to 40 batches (gbi +2.4 ms) unless the slot is drawn as one full screen quad when the strips cover the whole screen (START_BATTLE). That is an optional second step.
5. Risk: the pause frame (gfx_transfer_frame_to_depth reads the buffer with the CPU, D03 path), the battle exit (OVERLAY_BLUR), Kammy and the mega quake. Test: `prevtest` phase run (same switches as above), golden pause1 to pause4, and one real encounter.
6. Cheaper step if the GPU path waits: take the readback once at the first frame of the transition and reuse it for the next frames. This changes the look (the blur no longer accumulates). Not recommended without the user.

## Not measured
- A real world encounter (OVERLAY_START_BATTLE with the swirl meshes). The `battle` switch skips the world part (pm_warp.c:362 to :376). The numbers above use prevtest, which draws the same blur function.
- The cost of the swirl mesh itself (D_8014E8F0). It is a mesh with a texture stencil on the GPU, the CPU cost is in the gbi phase only.
