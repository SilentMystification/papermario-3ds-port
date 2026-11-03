# BTLSTAR diagnosis: battle-start star transition (garbled, laggy)

Scope: read-only diagnosis. No source edits. Build not rebuilt (build3ds/pm_3ds.3dsx).

## 1. Transition in the decomp and in src (CONFIRMED)

- Start point: src/encounter.c:1530 (back layer, DEMO_STATE_CHANGE_MAP) and src/encounter.c:1538 (front layer) call `set_screen_overlay_params_*(OVERLAY_START_BATTLE, encounter->fadeOutAmount)`.
- Draw: src/screen_overlays.c:322-343 (`case OVERLAY_START_BATTLE`):
  - :331 `gSPDisplayList(D_8014E8F0)`: star mesh, one cycle, depth-only write (`CLR_MEM` blender, nocolor path). Vertices: assets/us/vtx/stencil1.vtx.inc.c (10 verts).
  - :335 `gSPDisplayList(D_8014E9A8)`: Z-buffer translucent surface, prim depth 20.
  - :343 `draw_prev_frame_buffer_at_screen_pos(0, 0, 319, 239, v0)`, with v0 = min(progress + 40, 170). This is a full-screen copy of the previous frame, blended over the star hole.
- Copy routine: src/screen_render_util.c:79-125 (driver), :30-77 (`appendGfx_draw_prev_frame_buffer`). It loads the previous colour buffer in strips of 6 rows: 240 / 6 = 40 `gDPLoadTextureTile` (320 x 6, RGBA16) plus 40 `gSPTextureRectangle` per frame.
- Other stencil paths (`appendGfx_screen_transition_stencil`, src/screen_render_util.c:258+) are not used by this transition.

## 2. Port path (3ds/src) (CONFIRMED by reading)

- The previous frame request goes to `cfb_capture_prev`, 3ds/src/pm_main.c:~236-264. It tries the GPU copy first (`pm_gpu_grab_prev`). If the result is not 2, it falls through to the CPU readback `pm_gpu_read_frame(prev)` at pm_main.c:263. Cost is logged only when phase logging is on (`pm_b7.cap_*`).
- CPU readback: 3ds/src/pm_gpu.c:1524-1560. One pass over 320 x 240 = 76800 pixels, with Morton (tiled) address math per pixel, plus 80 side columns when wide. This is the slow path.
- GPU copy: 3ds/src/pm_gpu.c:1702-1760 (`pm_gpu_grab_prev`, `grab_impl`). It returns 0 (CPU fallback) when:
  - `!target || !gpu_ok` (pm_gpu.c:1703-1704);
  - `grab_n > 0` in the same frame, i.e. a T2 grab (`grab_cur`, 3ds/src/pm_gbi.c:266-273) already ran this frame (pm_gpu.c:1724);
  - first frame of a run of requests: the copy is queued only at `pm_gpu_begin` (pm_gpu.c:1849-1850, `prev_pre`), one frame after the request.
- Per-strip loads: the frame-buffer texture is read again at each bind (pm_gbi.c:2654), and the native u16 path is in pm_tex.c:1010 (`pm_fb_native`). 40 strip binds per frame is the expected per-frame cost. Not measured.
- Memory blend: the star mesh is written as depth-only (pm_gbi.c:675-684). The memory blend (`pm_gpu_set_memblend`, pm_gbi.c:684) is only for two-cycle modes, so it does not apply to D_8014E8F0.

## 3. Measured run (battle_start, one start, BTLSTAR)

- Command: `azt.ps1 battle_start -Row BTLSTAR` (default build). Log checks 4 of 4 PASS. Shot line `shot 4 frame 182`.
- Frame interval in the shot window (frames 153 to 182): t = 5185 ms to 6154 ms, 29 steps, mean 33.4 ms per frame (about 30 fps under Azahar). Azahar numbers show direction only.
- The battle_start scenario does NOT run the star transition. The debug switch sets `ENCOUNTER_STATE_NONE` and calls `load_battle` directly (3ds/src/pm_warp.c:586-590). So OVERLAY_START_BATTLE never runs in this run. No log line shows the transition.
- Shot 0005 (frame 182): battle HUD (HP 10/10, FP 5/5, star points 0) with Mario and one Goomba, landscape image shown in the 240 x 400 portrait frame. No garble visible. This is the subject of the scenario, not the transition.
- Shot files: build3ds/azt/shots/battle_start/0000_0_2.ppm to 0005_0_2.ppm (PNG copies 0000_0_2.png and 0005_0_2.png in the same folder).

## 4. Hypotheses (INFERRED)

- H1 (slow): the first frame of the transition takes the CPU readback (pm_gpu.c:1524-1560, 76800 pixels). Also every frame where a T2 grab ran first (pm_gpu.c:1724). Cost is not measured.
- H2 (slow): 40 strip loads and 40 texture rectangles per frame (screen_render_util.c:30-77) with 240 rows of texture bind. Not measured.
- H3 (garble): the copy uses the grab slot (pm_gpu.c:1707-1760, 256 x 512 copy). A transition frame that mixes the CPU readback and GPU copy paths, or a frame with the wrong previous buffer parity (`nuGfxCfb` index, screen_render_util.c:43-49 and pm_main.c:~236), would show a wrong or mixed previous frame. Not reproduced.

## 5. What would settle it

1. A scenario that enters battle through the encounter path (walk into a world enemy, so encounter.c:1530/1538 run) on the same build. Its shot window must cover the transition frames. This is a new scenario, so it goes in azt_scenarios.json and the catalog in the same commit. The current battle_start cannot show the transition.
2. Phase logging on that run (`phase` and `phsum` switches, as in the console push line). Read `pm_b7.cap_n`, `cap_ticks` and `cap_max` (pm_main.c:~249-263) to count the CPU readbacks per transition frame. Zero CPU readbacks means H1 is out. A count of 1 on the first frame only means H1 is only a first-frame cost.
3. The shot from that run must show the star hole, the copied frame and any garble. A person must open it.
4. For H2, the frame time of the transition frames (phase line PH_TEX / PH_GPU) against the same frames without the transition.

## Notes

- Acronyms: CPU (central processing unit), GPU (graphics processing unit), RGBA16 (16-bit red, green, blue, alpha), PPM (portable pixmap image), PNG (portable network graphics), HUD (heads-up display), FP (flower points), HP (hit points), T2 and T3 (buffer grab roles in pm_gpu.c:1671 and pm_gpu.c:1699), PICA (the 3DS GPU), VRAM (video RAM), TMEM (texture memory).
