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

## 3. Capture plan (the user plays during one prof run)
- Build: main with STEPX merged, ROMDR not merged (the run is the before). Words: `--profile prof warp=mac_01:0 pos=0:0:200 profdisk=1`, capture_check first.
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
| 3 | TEXSPR | Sonnet | page after the capture if t is high in slow walk steps: the player raster texture upload outside the walk | - | GUESS 0.3 to 1 ms per change |
| 4 | MCAM | Sonnet | page after the capture if m is high in walk steps | - | OPEN |
- Det benches run serial (pm_logic.c:135-140): a drain there waits for nothing. So ROMDR is byte-equal by construction on Azahar; the console capture decides.
- PIPE status (plan_pipe.md 5): at idle c0i gx 1.44 is under 2 ms and pace 6.09 is over 1 ms, so PIPE3 does not pay at idle. PIPE0 (Azahar f390):
  `ring 0/394`: the first draw of every frame is a ring draw, so PIPE3 needs PIPE2 first. PIPE2 and PIPE3 wait for the walking data.
