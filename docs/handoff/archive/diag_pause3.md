# diag_pause3: pause menu frame cost (row PAUSE3)

Scope: code read, stored Azahar logs. No Azahar start, no source edit.
Tags: [M] measured, [C] code, [E] estimate, [U] not measured. Azahar ms: direction only.
## (1) Readback: once per open
- [C] pm_gpu_read_frame (pm_gpu.c:1490) runs only from cfb_capture_prev (pm_main.c:262).
- [C] That path runs only from the BEGIN_PAUSED fill (pm_gbi.c:3616, background_gfx.c:353). One time per open.
- [M] Readback 17.4 ms per call (diag_cpu_gpu_crossing.md).
- [C] CPU filter gfx_transfer_frame_to_depth (background_gfx.c:264) runs once. Cost [U].
- [M] Open spike, run C: wall 112, 76, 37, 19, 28, 95 ms (f90 to f95).
- [C] Each frame redraws the saved picture (SHOW_PAUSED, background_gfx.c:375-412).

## (2) Gameplay versus pause, same run C (build 1301-ui), per frame
| state | ops | tri | closes | GBI ms | GPU ms |
|---|---|---|---|---|---|
| gameplay f73-89 | 5474 | 371 | 72 | 11.8 | 0.5 |
| pause steady f100-113 | 3329 | 0 | 217 | 16.1 | 1.7 |

- [M] Pause adds 4.3 ms GBI (+36 %) and 1.2 ms GPU with 2145 fewer ops. Closes rise x3.0.
- [M] MENULAG (build 1433): pause walk 11.1 to 12.9 ms, closes 135, ops 1310 to 1471.
- [M] Yard work in the catalog: 11.66 ms (yard_pace), 12.74 ms (yard_phase). Azahar shows no walk deficit.
- [M] Wall 33.4 ms in both pause runs. Azahar paces at 30 fps and cannot show the console drop.

## (3) World under pause
- [C] Logic stops: state_step_pause case 2 runs only pause_handle_input. Time freezes (main_loop.c:397).
- [C] Entities and NPCs are cleared at open. Draw continues: render_frame runs each frame (main_loop.c:203, 231). Pause does not set DISABLE_RENDER_WORLD.
- [M] Steady pause emits 0 triangles (run C f99 on; MENULAG tri 0). Ops still run.

## (4) Menu cost, steady frame
- [M] 217 closes (run C f105): 151 real tex, 18 real rect; empty closes 249.
- [M] Real close: 23.6 us (tex), 22.6 us (rect). The 97 us per close premise fails.
- [C] Empty closes only count (pm_gpu.c:2253-2263). Cheap.
- [C] Background: 40 tile loads, 40 texrects and 40 PipeSync per frame (background_gfx.c:391-412). wide is off by default.
- [C] Labels: one display list and one pause_draw_rect per label (pause_gfx.c:644-661).
- [C] Steady active pages use flags 0 (pause_main.c:387-391). Off-screen passes mainly in flip frames.
- [M] Texture misses: 0 to 1 per steady frame. Open: 74 at f95.
- [U] Off-screen count per steady frame. Evictions. Palette uploads.

## Top 3 causes
1. Background strip tile changes. [M] 151 real tex closes per frame. [E] 40 from strips, 23.6 us each = 0.9 ms Azahar.
   Fix: background_gfx.c, gfx_draw_background, SHOW_PAUSED loop (391-412). Under TARGET_3DS, one tile load and one texrect. Check pm_tex size support first [U].
   Expected: -39 tex closes, 0.9 ms Azahar. Console [U].
2. Label and glyph tex changes. [E] About 111 real tex closes, 23.6 us each = 2.6 ms Azahar.
   Fix: pause_gfx.c, pause_draw_rect path (644-661). One atlas. Asset build needed.
   Expected: up to -111 tex closes, 2.6 ms Azahar. Console [U].
3. Open spike. [M] Six frames above 33 ms, max 112 ms. Readback 17.4 ms once. Flip off-screen passes [C].
   Fix: none without timing. Time pm_main.c:262 and background_gfx.c:370 first.
   Expected: once per open, no steady effect. Console [U].

Next step: [U] Split the 217 closes by site with the gbiprof switch (pm_gpu.c:298-325) on pause_menu. Needs a catalog row. Not run.
