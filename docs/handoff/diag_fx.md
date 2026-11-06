# Diag fx: Merlee upgrade spell hang and super block upgrade draw (row MERLEE, step 1)

Read-only trace. No source edit, no build, no Azahar start. Line numbers are on branch nds1 at the time of this trace.

## Bug 1: Merlee upgrade spell (dro_02) soft-hangs, card effect does not play

### Script and effect ids
- Cast script: `N(EVS_PerformRitual)`, `src/world/area_dro/dro_02/npc_merlee.c:606`. Started from `N(EVS_NpcInteract_Merlee)` via `ExecGetTID` (`npc_merlee.c` around :760).
- Orb effect at start: `EFFECT_RADIATING_ENERGY_ORB` (`npc_merlee.c:646`). It does not gate the script (the orb only dismisses after 30 frames, :649-650).
- Card effect: worker `N(card_worker_update)` / `N(card_worker_render)` made in `N(CreateRitualCards)` (`npc_merlee.c:172-193`). Card visuals are imgfx anims `IMGFX_ANIM_SHUFFLE_CARDS` (:178) and `IMGFX_ANIM_FLIP_CARD_1..3` (:181-187).
- Gate: `N(EVS_PerformRitual)` waits on `RITUAL_VAR_STATE == RITUAL_STATE_D` (`npc_merlee.c:657-661`). The state moves only in the worker.
  - State 2 to 3 is set in the render, not in the update: `npc_merlee.c:578-579`, `if (card0.unk_00 == 2 && (ret - 1) < 2)`. `ret` is the return of `imgfx_appendGfx_component` for the shuffle (`npc_merlee.c:255`). It must be 1 or 2.
  - State A to B is the same test on the flip 3 return (`npc_merlee.c:581-582`, call at :281).
- So the script waits forever if the shuffle imgfx returns 0 on the 3DS.

### Cause on the 3DS (most likely, 1 of 2)
`N(appendGfx_ritual_card)` declares `ImgFXTexture ifxImg;` at `npc_merlee.c:208` and never initializes it on the unk_00 == 2 path (`npc_merlee.c:249-258`). Its `alpha` is stack garbage.
- `imgfx_appendGfx_component` returns 0 at once when `ifxImg->alpha == 0` (`src/imgfx.c:773-774`), before `imgfx_appendGfx_mesh`. So no shuffle geometry is drawn (the card effect does not play) and `ret == 0` every frame (the state stays 2, the script waits at `npc_merlee.c:657`).
- The flip 1 and flip 2 calls on the unk_00 == 3 path (`npc_merlee.c:266-267`) read the same uninitialized `ifxImg`, so those draws are also dropped. Only flip 3 sets `alpha = 255` (`npc_merlee.c:280`).
- Why only the 3DS: the N64 build happened to leave a nonzero byte in that stack slot. The 3DS build (GCC for ARM11, different frame layout) does not.

### Second most likely cause (2 of 2)
The worker is not run, so state stays `RITUAL_STATE_INIT` (0) and the render never draws. Path:
- `update_workers` / `render_workers_scene` call the worker through `call_worker` (`src/worker.c:~30-40`). `call_worker` silently returns if the pointer is not 4-byte aligned or is outside `[__start__, __init_array_start)`. The comment there says texture bytes have spilled into the worker list before. A corrupted slot in `gWorldWorkerList` (BSS, 3DS-only overflow) makes the update and the draw both skip. Then `state` stays 0 and the script waits at :657.
- Check: a 3DS log line with the worker slot address and a count of `call_worker` refusals (add a debug print in `call_worker`, debug only, then remove it in the fix commit).

### Other checks (not the main cause)
- imgfx anim data for the 3DS is read from fixed ROM offsets (`src/imgfx.c:233-236`, values from `ver/current/splat.yaml:1532-1535`). A wrong offset gives a header that never sets `IMGFX_FLAG_ANIM_DONE` or `IMGFX_FLAG_4000`, so ret stays 0. Same symptom, check after cause 1.
- imgfx instance pool is 90 (`include/macros.h:115`). `imgfx_get_free_instances` returns -1 when full (`src/imgfx.c:404-417`). A -1 index makes `imgfx_appendGfx_component` return 0 (`src/imgfx.c:769-786`). Check the free count when the spell starts.

### Fix (30 lines or less, cause 1)
In `src/world/area_dro/dro_02/npc_merlee.c`, in `N(appendGfx_ritual_card)`:
- Line 208: `ImgFXTexture ifxImg = { .alpha = 255 };` (raster, palette, sizes are unused on the SKIP_TEX_SETUP path, so zero is fine).
- Check that the unk_00 == 3 path still sets `alpha = 255` (:280) for flip 3. Keep it.
- Same commit: remove the stale comment if any. No toggle, no stub.
- Gate: the scenario below shows the shuffle cards in a shot and the log line `dro_02 ritual state D` (or the effect of the next line of the script) appears. Azahar shows the shuffle as an overlay on the cards.
- If the scenario still stalls, go to cause 2 (print the worker slot and the `call_worker` refusal count).

### Azahar scenario that reproduces it (to add in the code agent's commit, not made here)
- `warp=dro_02:0 story=ch0_toad_town` plus a coin count of at least 50 (`N(MerleeCoinCosts)[0]`, `npc_merlee.c:58-61`). Coin setting: check the debug preset list (`azt_scenarios.json` has 10 `coins` hits; use the same switch).
- padscript: walk to Merlee, then talk (`N(EVS_NpcInteract_Merlee)`, :735+), answer the first choice with "yes" (`MSG_Choice_0018`), the spell cast starts.
- Shot: `shots shotwin=<spawn frame + 30>:6`. The shuffle cards should show.
- Until-line: `EVS_PerformRitual` end, or the state D line. Current binary: no end line, so the cap (25 s) hits: that is the hang.

## Bug 2: super block partner upgrade, flash and dots draw wrong

### Script and effects
- Script: `N(SuperBlock_OnHit)` (`src/world/common/entity/SuperBlock.inc.c`, from ~:488). Upgrade sequence: `N(SuperBlock_ShowUpgradeEffects)` (:438-466), ExecWait from :536.
- Dots (energy orbs): `N(SuperBlock_AnimateEnergyOrbs)` (:250-366). It makes 3 `fx_motion_blur_flame` effects (EFFECT id `EFFECT_MOTION_BLUR_FLAME`, created at :268) and moves them each frame by writing `data.motionBlurFlame->pos` (:362-365). It returns DONE2 at scatter state 2 (:~333), so the script waits 30 * DT after it (:465).
- Flash: `N(SuperBlock_WhiteScreenFlash)` (:371-), called at :445 and :448. It uses the front screen overlay: `set_screen_overlay_color(SCREEN_LAYER_FRONT, 208, 208, 208)` (:380), then `set_screen_overlay_params_front(OVERLAY_VIEWPORT_COLOR, alpha)` (:~ 415). The alpha goes through the `f32 zoom` parameter.
- Other effects in the same sequence: `fx_radial_shimmer` ids 4 and 9 (:~306-318), `fx_sparkles` (:~322-326), `fx_energy_orb_wave` (:~228, glow).

### Draw path on the 3DS
- `src/effects/motion_blur_flame.c` is a framebuffer readback effect. `motion_blur_flame_appendGfx` (:126-) takes strips of the current frame buffer with `gDPLoadMultiTile(... nuGfxCfb_ptr + uly * SCREEN_WIDTH ...)` (:248), one strip per rectangle, and draws them back as texture rectangles.
- On the 3DS that load goes to `grab_cur` (`3ds/src/pm_gbi.c:270-276`, `pm_gpu_grab(nuGfxCfb_ptr, y, y + h)`). The grab copy (`3ds/src/pm_gpu.c`, `grab_tex`, `grab_dirty`, `grab_dy0/grab_dy1`, :240-249, :880-882, :962-964, :1257-1259) is tracked by a single row range per frame.
- The fx_api call path is `src/evt/fx_api.c:619` (`EFFECT_RADIATING_ENERGY_ORB` at :609 is a different effect). `3ds/src/effect_loads.c:463-465` forwards to the same `motion_blur_flame_main`, so the orb path is the same code as N64.

### Cause 1 of 2 (dots wrong): stale framebuffer grab for flame strips
- Three flames draw strips in the same frame, with rows written by the earlier flame rectangles. The grab is a copy taken once per frame, and the `[grab_dy0, grab_dy1)` range is a union of all rectangle rows (`pm_gpu.c:880-882`). If a later flame loads a strip that is outside the copied window (or the copy is taken before the earlier flame's rectangles), it reads old pixels: the dots draw in the wrong place or with the wrong colours.
- Not verified: I did not read the flush path that decides when `grab_dirty` forces a re-grab (`pm_gpu.c` around :962-1000). Read that before the fix.

### Cause 2 of 2 (flash wrong): front screen overlay on the 3DS
- `set_screen_overlay_params_front(u8 type, f32 zoom)` (`src/screen_overlays.c:371-393`) takes the fade value as `zoom`. `N(SuperBlock_WhiteScreenFlash)` passes `functionTemp[1]` as an integer 0 to 255 (`SuperBlock.inc.c:~415`). The other callers pass 255.0f and -1.0f (`src/encounter.c:1442`, :1851). If the viewport colour path for OVERLAY_VIEWPORT_COLOR does not scale this value on the 3DS, the flash is full white or never fades.
- Not verified: the 3DS draw of the front overlay (the overlay is probably drawn with a blend the 3DS path does not support). Read `src/screen_overlays.c:371-420` and its 3DS draw before the fix.

### Fix (30 lines or less, cause 1)
In `3ds/src/pm_gpu.c` (flush/grab bookkeeping, not `pm_gbi.c`):
- When a texture rectangle writes rows that a later flame strip reads (`rect_y0..rect_y1` overlap the strip rows), set `grab_dirty = 1` before the strip load, so the next load re-grabs the full window.
- Keep the one-copy-per-strip rule for other textures (R13c). Only the flame path needs the re-grab.
- Check `pm_gbi.c:270-276` still calls `pm_gpu_grab` for the window (no change).
- Gate: the scenario shows three dots moving onto the partner, each dot a clean blur (picture check), and the log line `SuperBlock upgrade effects done` (or the next script line after `Call(N(SuperBlock_AnimateEnergyOrbs))`).

### Azahar scenario that reproduces it (to add in the code agent's commit)
- `warp=<map with a super block, e.g. the first super block in the game> story=ch0_toad_town` with a partner that can be upgraded (the partner must be enabled and below max level, `N(SuperBlock_get_partner_rank)` :82-95).
- padscript: walk into the super block (hit it from below), pick the partner in the popup (`N(SuperBlock_ShowSelectPartnerMenu)`), "no" to the switch-partner question.
- Shot: `shots shotwin=<hit frame + 40>:6` (the orbs and the flash are in frames 40 to 110 after the hit: Wait(10*DT) + Wait(85*DT)).
- Until-line: `ShowUpgradeEffects` done (the Return at :~465), then the picture.
