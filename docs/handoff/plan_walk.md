# plan_walk.md: why walking lags and the Z spin runs faster (oracle NATIVE19; code read first; edit in place)
Tags: FACT (log line or code read), GUESS, OPEN. Line numbers: main 0eb73a88e7. Reference: capture 3 at idle (main 8e06a08fb3, window f1289):
fps 55.2, ft 35.60, wk 31.40, core 1 busy 28.36, c0i idle 1.70 pace 6.09 gx 1.44, c0k gpul 2.63 gpuk 7.85, c1s logic 2.47, iv 15/236/34/15 (FACT, coordinator).
No capture has walking or spin data (all at mac_01 0 0 200, player standing).

## 1. The main suspect: a ROM read on the logic thread drains the walker (FACT mechanism, rate OPEN)
- nuPiReadRom (3ds/src/pm_os.c:490-541) calls pm_walk_drain("rom") on every non-audio thread (:492-495). On the logic thread that is pm_wq_idle ->
  wq_block(c_empty, WQB_ALL) (nd_wq.c): core 1 stops until core 0 has run the whole queue.
- The player sprite reads its rasters from ROM on demand: spr_get_player_raster (src/101b90_len_8f0.c:332-363). A cache entry lives 2 steps after its last
  use (lazyDeleteTime = 2 at :341, :356; minus 1 per step in spr_render_init, src/sprite.c:911-912, called at src/main_loop.c:204). A raster that was not
  drawn in the last 2 steps is read again from ROM (:361). The call comes from the sprite draw (src/sprite.c:523), inside the list build of core 1.
- The queue at that time (GUESS from the step order, plan_critpath.md 1): the walk of the last frame, its step end, its key show (GPU L wait), and the begin
  of this step (pace and GPU K wait). Core 1 waits for all of it. Then the rest of the list build runs while core 0 has nothing to walk. The overlap is lost.
  Cost GUESS 8 to 15 ms per event, so a 2-vblank step becomes a 3-vblank step.
- Rate: idle 0.04 drains per step, all waited (FACT live_cap2_170.txt `core2 drain dr 0.04 drw 0.04`: about 12 per 300 steps). Walking: OPEN.
- Why the 3 states differ (GUESS from the code): walking cycles through many rasters, each used again only after more than 2 steps, so ROM reads come
  several times per second. ANIM_Mario1_Spin (src/world/action/spin.c:83) turns the sprite by spriteFacingAngle (:244, :254), so few rasters repeat every
  step and the cache keeps them: no ROM read, no drain. Standing: the idle animation changes a raster now and then (the 12 drains per window).
  This order (spin faster than standing, standing faster than walking) matches the user report.
- The drain is not needed for this read: the target entry is free, so it was last drawn 2 or more steps ago. The walker has walked that frame and
  its key frame before this step started (the slot wait, nd_wq.c:162-163). The queued frame draws only entries in use (lazyDeleteTime above 0). FACT, code read.
  Row ROMDR (section 4).

## 2. Other candidates (checked against the code)
| # | candidate | code | verdict |
|---|---|---|---|
| 2 | texture reload on core 0 when the player raster changes (the same cache address, new texels) | pm_tex.c load_miss :1414; sprite memo cnt smh/smm | on the core 0 chain; GUESS 0.3 to 1 ms per change; STEPX t counts it |
| 3 | model cache misses when the camera moves (cull range, lit or fogged entries) | pm_gpu.c model_vis_same and mver tests :4263-4265; nd_recs.c rec_miss :184 | OPEN; STEPX m counts it |
| 4 | the 33.4 ms pace grid: at idle the grid holds (pace 6.09 FACT); a step 2 ms late costs a whole vblank (16.7 ms of picture) | pm_gpu.c sync60_pace :2954-2965 | FACT arithmetic; it turns small spikes into visible lag |
| 5 | game logic when walking (collision, camera follow, partner) | c1s logic 2.47 at idle (FACT) | off the chain while core 1 has slack (fw); GUESS +0.5 to 1 ms; STEPX shows it |
| 6 | fps60 interpolation and the K copy when things move | pm_interp.c lerp; kc_build pm_gpu.c:3128 | the K copy cost does not depend on motion (a buffer copy); GUESS under 0.2 ms |
| 7 | FS1 frame skip (option fskip, default 1, pm_options.c:18) skips a draw when 2 vblanks have passed (pm_main.c:689-695) | ph line `st` against `w` | at idle st = w (no skip, FACT); walking OPEN |
| 8 | particles while walking | no fx_ call in the player walk paths (FACT grep: src/world/action/walk.c, src/7BB60.c, src/77480.c); the spin makes one fx_effect_46 (spin.c:121) | walking makes no dust in the world; the spin makes more effect work, not less: no fit with "the spin runs faster"; STEPX2 e counts effects |
| 9 | footstep sounds | try_player_footstep_sounds every 8 steps walking, 4 running (walk.c:64, :146; 7BB60.c:1118-1137); spin: one sound per spin (spin.c:111); VADPCM decode only at the first play of a sample (pm_dspaud.c:11) | GUESS small (a cached sample); no fit with "spin faster than standing"; STEPX2 a counts decodes; au, aj in the core2 wait line |
- The spin draws the player twice (main sprite and blur, appendGfx_player_spin src/77480.c:1593-1652, chosen at :1516-1519): more draw work. Only the raster
  cache (section 1) fits all 3 user facts (GUESS).

## 3. Capture plan (the user plays during one prof run)
- Build: main with STEPX, STEPX2 and DRAINC merged; ROMDR and SLOTL not merged (the run is the before). Words: `--profile prof warp=mac_01:0 pos=0:0:200 profdisk=1`, capture_check first.
- Push with -Seconds 150. The user: stand 30 s, walk in circles 30 s, spin with Z again and again 30 s, walk 30 s, stand 30 s.
  A window is 300 steps (about 10 s at 30 steps per second): each phase holds at least 2 pure windows. STEPX also buckets each step by the player action state.
- Read per window: stepx (n, ft and slow steps per state; slow against normal: GPU waits, pace, logic-thread drain wait, ROM reads, raster reads, texture
  misses, model misses; the longest step), core2 drain dr and drw, c0i, c0k, ph st and w, cnt smh smm, ndc spr.
- Decision: walk-bucket slow steps with r or s about 1 and d over 5 ms confirm section 1: merge ROMDR, then the same play script again.

## 4. Rows (ranked; ms GUESS)
| # | row | model | what | gate | effect |
|---|---|---|---|---|---|
| 1 | STEPX | Haiku | per-step census: state bucket, ft, slow-step causes, the longest step (page ws_STEPX) | spot_mac01_fps60_det, capture_check | 0 (decides) |
| 2 | ROMDR | Haiku | the player raster read skips the walker drain (page ws_ROMDR); merge after the STEPX capture | spot_mac01_fps60_det, spot_mac01_drec | walking: up to 1 vblank less per raster change; idle: about 12 slow steps less per window |
| 1b | STEPX2 | Haiku | 4 more stepx fields: audio decodes, ROM bytes, Yay0 decodes, effect instances (page ws_STEPX2), before the capture | spot_mac01_fps60_det, capture_check | 0 |
| 1c | DRAINC | Haiku | drains by caller: top 4 per window with count and ms (page ws_DRAINC), before the capture | spot_mac01_fps60_det, capture_check | 0 (picks the next drain row) |
| 2b | SLOTL | Haiku | slot and depth 1 wait for the L walk only, not the key show (page ws_SLOTL; section 5 row 2) | fps60_det, fps60_stereo_det, capture_check race 0 | GUESS -1 to -1.7 when not pace bound |
| 3 | TEXSPR | Sonnet | page after the capture if t is high in slow walk steps: the player raster texture upload outside the walk | - | GUESS 0.3 to 1 ms per change |
| 4 | MCAM | Sonnet | page after the capture if m is high in walk steps | - | OPEN |
- Det benches run serial (pm_logic.c:135-140): a drain there waits for nothing. So ROMDR is byte-equal by construction on Azahar; the console capture decides.
- PIPE status (plan_pipe.md 5): at idle c0i gx 1.44 is under 2 ms and pace 6.09 is over 1 ms, so PIPE3 does not pay at idle. PIPE0 (Azahar f390):
  `ring 0/394`: the first draw of every frame is a ring draw, so PIPE3 needs PIPE2 first. PIPE2 and PIPE3 wait for the walking data.

## 5. Every drain, fence and wait between core 1, core 0, the walker queue and the GPU queue (ranked by ms; user rule: wait only for the true dependency)
| # | wait | file:line | protects | true dependency | cost per step | targeted form | row |
|---|---|---|---|---|---|---|---|
| 1 | logic-thread drain before a ROM read | pm_os.c:492-495 -> nd_walk.c:469-476 -> nd_wq.c:173-180 | RAM a queued walk may read | none for a free raster entry (section 1); else the frames that read the target | idle dr 0.04, drw 0.04 (FACT); GUESS 8 to 15 ms each; walking OPEN | no wait (ROMDR); others by caller (DRAINC) | ROMDR, DRAINC |
| 2 | slot before the lerp and depth 1 before the push | nd_wq.c:165, :152 | rtab, repl (pm_interp.c:127-131); the context chain | the L walk of the frame before only (rtab read only with pm_interp_repl, nd_mtx.c:48, nd_mtx_set.c:115); the key frame is a copy | the key show sits on the chain: c0i idle 1.70 (FACT capture 3) | count L frames only | SLOTL |
| 3 | L begin C3D_FrameBegin | pm_gpu.c:2998 | command buffer, ring chunk 0, citro3d frame state | the GPU end of K for chunk 0 and the queue | gx 1.44, pace 6.09 (FACT capture 3; the pace is the 30 step grid, intended) | PIPE1 (done), PIPE2, PIPE3 | plan_pipe.md |
| 4 | key show C3D_FrameBegin | pm_gpu.c key show (:3156-3185) | citro3d one frame in flight | the GPU end of L | kgx OPEN in capture 3 (gpul 2.63 is its ceiling, FACT) | PIPE4 | plan_pipe.md |
| 5 | game thread waits for the logic (draw flag, show, show keys, hand) | pm_logic.c:153, :166, :173, :142 | the D flag, posted step ends and key frames | the next posted entry | c0i idle 1.70 (FACT capture 3) | shrinks with row 2 | SLOTL |
| 6 | sampled step quiesce (prof only) | pm_phase.c pf_arm -> pm_logic.c:111-116 | the serial timers | none in normal play | 10 serial steps per window at about 64 ms (FACT s10 fts 64.39): they fall in the iv 4+ bucket | none (a measurement cost); STEPX skips them | - |
| 7 | pm_gx_room, pm_gx_drain, depth read after a kick | pm_gpu.c:2642, :2659, the depth read (pm_gx_drain when kick_done) | the 32-entry GX queue; GPU writes the CPU reads | real (the GPU must have run) | PIPE0 room 0 of 394 frames (FACT Azahar) | - | - |
| 8 | full walker queue, audio join, logic ring | nd_wq.c:118; core2 aj; pm_logic.c:142 | the 64-entry queue; the audio buffers; the 4-entry ring | real | wqf 0.00, aj 0.00 (FACT cap2) | - | - |
| 9 | ring chunk reuse, glyph cell reuse, texture staging free | pm_gpu_rect.c:41 (stamp < ring_done), pm_tex.c:322 (cell_fr + 2), pm_tex.c:795 (pm_tex_gate) | GPU reads of old frames | already frame stamps: the model of the targeted wait | 0 (no wait, FACT code) | - | - |
- A general frame-id wait (wait until the walker has passed frame F) needs F, the last frame that reads the target. The game code gives F only where it
  keeps use stamps (player rasters: lazyDeleteTime; chunks and cells: stamps above). So it does not replace ROMDR: for a free raster F is already passed,
  and no wait is the same result with less code. DRAINC names the other drain callers; each one gets its own narrow row from its own stamp or none.
- Race checks (det benches run serial and show no race): ROMDR counts romrace (a raster loaded after the walked frame); SLOTL counts slotl race (a lerp during
  an L walk). Both print in prof windows and must stay 0 in the console capture.

## 6. Addendum: play capture 2 and every nuPiReadRom caller (FACT build3ds/out/live_play2_170.txt, main a05f62c545)
- Step time by state: idle 37.45 ms (68 percent slow), walk or run 44.10 (88 percent), spin 36.34 (56 percent). Drains per step: 0.98 walking, 0.02 idle;
  drw = dr. drainc: spr_get_player_raster+0x12c in 286 of 290 steps; dma_copy 1 per window. c0i idle 5 to 9 ms walking against 1.0 at idle (all FACT).
- Reading: the walk penalty (+6.65 ms per step against idle, FACT arithmetic) is about the c0i idle rise (4 to 8 ms, FACT): the drain is the walking cost.
  It is not larger than walking in mac_01: the other callers drain once per window. Idle slowness (68 percent slow at 0.02 drains) has another cause: the
  stepx slow fields of the idle windows decide.
| caller | file:line | when (GUESS from the code) | can queued walker work read the destination | form |
|---|---|---|---|---|
| player raster | src/101b90_len_8f0.c:362 | each new raster: about 1 per walking step (FACT) | no: a free entry (lazyDeleteTime 0) | ROMDR (first row) |
| HUD and item icon raster, palette | src/hud_element.c:198, :234; src/item_entity.c:312, :345 | a new icon: menus, pickups, the status bar | no while the cache only grows (append at gHudElementCacheSize, :191-199); yes in the first step after a cache clear reuses the bytes | later row HUDDR (Sonnet): own read when no clear happened since the last frame push |
| NPC sprite asset | src/101b90_len_8f0.c:179, :201 | NPC spawn, map load | yes: general_heap_malloc (:194) can return a block freed this step that a queued frame still draws | keep the drain |
| player raster sets and headers | src/101b90_len_8f0.c:267, :296, :319, :323 | sprite set change, map load | header tables only; rare | keep the drain |
| dma_copy | src/43F0.c:439, :443 | map, battle, overlay and effect loads (1 per window, FACT) | yes: fixed slots and overlays are reused | keep the drain |
| battle partner | src/battle/190B20.c:1514 | battle start | yes (heap) | keep the drain |
- One rule inside nuPiReadRom does not replace the per-caller rows: the drain protects the destination, and only the caller knows a free test for it
  (raster lazyDeleteTime, the icon cache size, nothing for heap blocks). A copy from the resident ROM image (the PAUSEDR form) makes the read faster but
  does not remove the wait. So each safe caller is one call-site line (nuPiReadRomOwn, as ROMDR) plus its free test: later callers are cheap rows.
- Order: ROMDR (merge after its coder merges main) -> a play capture (walk bucket ft, dr, c0i idle) -> HUDDR page only if drainc shows hud_element or
  item_entity callers in a menu or pickup capture.
