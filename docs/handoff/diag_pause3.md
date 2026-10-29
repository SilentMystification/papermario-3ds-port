# diag_pause3: why the pause menu runs slower than gameplay (row PAUSE3)

Scope: static code read, stored Azahar logs, no new Azahar start, no source edit.
Tags: [M] measured in log, [C] code read, [E] estimate, [U] not measured.
Azahar ms show direction only. Console ms are not measured.

## (1) Readback: once per open, not per frame
- [C] pm_gpu_read_frame (3ds/src/pm_gpu.c:1490) runs only from cfb_capture_prev (3ds/src/pm_main.c:262).
- [C] cfb_capture_prev(1) runs only from the G_FILLRECT ROLE_CPUFILL op (3ds/src/pm_gbi.c:3616).
- [C] That op runs only in BEGIN_PAUSED (src/background_gfx.c:353-363), one time per pause open.
- [M] Readback cost 17.4 ms per call (docs/handoff/diag_cpu_gpu_crossing.md).
- [C] The CPU filter gfx_transfer_frame_to_depth (src/background_gfx.c:264) runs once, in FILTER_PAUSED. Cost [U].
- [M] Open spike, run C (docs/handoff/diag_menu_lag2.md): wall 112, 76, 37, 19, 28, 95 ms at f90 to f95. GPU 14.3 ms at f95.
- [C] Each frame still redraws the saved picture: SHOW_PAUSED (src/background_gfx.c:375-412).

## (2) Same run, gameplay versus pause (run C, build 1301-ui)
| state (per frame) | ops | tri | closes | GBI ms | GPU ms |
|---|---|---|---|---|---|
| gameplay, same map, f73-89 | 5474 | 371 | 72 | 11.8 | 0.5 |
| pause steady, f100-113 | 3329 | 0 | 217 | 16.1 | 1.7 |

- [M] Pause costs +4.3 ms GBI (+36 %) and +1.2 ms GPU with 2145 fewer ops.
- [M] Closes rise x3.0. Closes are the driver: 97 us per close premise fails, real close is 23.6 us (tex) and 22.6 us (rect).
- [M] MENULAG (build 1433, Azahar): pause walk 11.1 to 12.9 ms, closes 135, ops 1310 to 1471, texmiss 0 to 1.
- [M] Yard work in the catalog: 11.66 ms (yard_pace) and 12.74 ms (yard_phase). Azahar shows no pause deficit in the walk.
- [M] Wall 33.4 ms in both pause runs: Azahar paces at 30 fps, so Azahar cannot show a fps drop.

## (3) Does the world still render or update under pause
- [C] Logic stops. state_step_pause case 2 (src/state_pause.c) runs only pause_handle_input. update_npcs, update_player and update_effects run once, in case 0.
- [C] Time freezes: TIME_FREEZE_POPUP_MENU (src/main_loop.c:397-401). Entities and NPCs are cleared at open.
- [C] Draw continues: gfx_draw_frame calls render_frame(false) and render_frame(true) (src/main_loop.c:203, 231). Pause does not set GLOBAL_OVERRIDES_DISABLE_RENDER_WORLD.
- [M] Steady pause emits 0 triangles (run C f99 and later; MENULAG tri 0/0). The ops come from the background and the windows and text.
- [E] The world camera pass costs state and ops only. Its cost is not split out [U].

## (4) Menu cost per steady frame
- [M] Closes 217 (run C f105): tex class 213 (151 real, 62 empty), rect class 205 (18 real, 187 empty).
- [C] Empty closes are cheap. pm_gpu_flush_at only counts and does rectangle bookkeeping (src/pm_gpu.c:2253-2263). No svc, no GPU call.
- [C] Background strips: 40 gDPLoadTextureTile, 40 texrect and 40 PipeSync per frame (src/background_gfx.c:391-412). wide is off by default (pm_options.c:12), so no side strips.
- [E] The 40 strips are 40 of the 151 real tex closes.
- [C] Labels: one gSPDisplayList and one pause_draw_rect per label (src/pause/pause_gfx.c:644-661). About 111 real tex closes.
- [C] Steady active page windows use flags 0 (src/pause/pause_main.c:387-391). Hidden inactive pages are skipped (src/windows.c, WINDOW_FLAG_HIDDEN). Off-screen passes run mainly in flip frames.
- [U] Steady off-screen passes per frame (no counter).
- [M] Texture cache: steady texmiss 0 to 1 per frame (run C f100-113). Open: 74 at f95, 47 at f96.
- [U] Evictions and palette uploads in the pause.

## Top 3 causes
1. Texture tile changes in the background strips. [M] 151 real tex closes per frame. [E] 40 from strips, 23.6 us each: 0.9 ms Azahar.
   Fix: src/background_gfx.c, gfx_draw_background, SHOW_PAUSED loop (lines 391-412). Under TARGET_3DS, one tile load and one texrect for the 320 x 240 saved frame. Check that pm_tex accepts the native u16 buffer at that size [U] first.
   Expected: -39 tex closes, about 0.9 ms Azahar. Console [U].
2. Label and glyph tex changes. [E] About 111 real tex closes per frame, 23.6 us each: 2.6 ms Azahar.
   Fix: src/pause/pause_gfx.c, pause_draw_rect path (lines 644-661). One atlas for the label set. Asset build needed.
   Expected: up to -111 tex closes, 2.6 ms Azahar. Console [U].
3. Pause open spike. [M] Six frames above 33 ms, max 112 ms. [M] Readback 17.4 ms once. [C] Flip frames run off-screen passes (pm_gpu.c:1520-1545, draw_box.c:430).
   Fix: none that keeps the picture without a measurement. Add a timer around pm_gpu_read_frame (pm_main.c:262) and gfx_transfer_frame_to_depth (background_gfx.c:370) first.
   Expected: once per open, no steady-state effect. Console [U].

## Not the cause
- Per-frame readback: not present (see 1).
- Texture cache misses in steady frames: 0 to 1 per frame (see 4).
- Empty flush calls: cheap by code (see 4). The earlier 0.097 ms per close figure is not valid.

## Next step
- [U] Split the 217 closes by site: gbiprof switch (3ds/src/pm_gpu.c:298-325) on pause_menu. Needs a catalog row first. No run made here.
