# plan_critpath.md: the critical path of one step (oracle NATIVE19; code read; edit in place)
Tags: FACT (log line or code read), GUESS (estimate), OPEN (not measured). Data: build3ds/out/live_cap2_170.txt f11489 (main 353b31c792, O3DS .170,
mac_01 0 0 200, prof, fps60 on), live_old_170.txt (display list build 3542), build3ds/hw_xl_log.txt f11321-f11621 (PMU sets, Nov 11 build, mac_00).
Line numbers: main 400d2ad3ab. citro3d: renderqueue.c (devkitPro clone). libctru: source/gpu/gxqueue.c.

## 0. Result in 6 lines
1. ft 41.21 ms (FACT) = core 0 work 25.16 ms (FACT: pmu cyc 6745604 per drawn step / 268112) + core 0 idle 16.05 ms (FACT arithmetic).
2. The idle is not core 0 work. It is wwait 1.03 (FACT) and about 15 ms of waits for the GPU and the pace (GUESS; c0i and c0k lines decide).
3. The GPU never runs while core 0 walks. KICK1 (pm_gpu.c:851-865) queues part 1 into a stopped GX queue: C3D_FrameBegin stops the queue
   (renderqueue.c:97, :170), gxCmdQueueAdd runs nothing while the queue is stopped (gxqueue.c:62-75), the queue runs at C3D_FrameEnd (renderqueue.c:240). FACT.
4. So the step is one serial chain: walk L (core 0) -> GPU draws L -> K copy submit -> GPU draws K -> next L begin (FrameBegin waits) -> walk L.
5. Core 1 is not on the chain: it waits 9.88 ms per step in pm_wq_frame_slot (FACT fw). This is why GD1 (core 1 -3.7 ms) did not move ft (FACT).
6. CPU proof that the idle is not work: in hw_xl_log.txt f11621 wk 18.05 and cyc 4.89M = 18.2 ms per frame at ft 33.5 (FACT): CCNT stops in a wait.

## 1. Timeline of step N+1 (G = game thread, core 0; L = logic thread, core 1; queue = nd_wq.c ring)
| # | who | what | file:line | waits for | ms |
|---|---|---|---|---|---|
| 1 | G | loop top, t0 of ft and wk | pm_main.c:582, :682 | - | - |
| 2 | G | pm_lg_draw_flag: waits until L starts the IDLE retrace (L ended step N) | pm_main.c:586, pm_logic.c:151-155 | L tail of step N | wwait 1.03 FACT |
| 3 | G | pm_lg_hand STEP N+1 (ring, LightEvent) | pm_main.c:729, pm_logic.c:142-147 | - | small |
| 4 | L | pm_step_run: posts begin N+1, step_game_loop, list build | pm_main.c:480-490 | - | about 27.5 busy GUESS |
| 5 | G | pm_lg_show pumps: begin N: tex upload, sync60_pace, C3D_FrameBegin(0) | pm_logic.c:163-168, pm_gpu.c:2983-2995 | GPU K of step N-1 | c0i pace, gx (GDW) |
| 6 | G | frame N: pm_frame_walk, the stream walk of the L frame | nd_wq.c:147, pm_nusys.c:124-139 | - | 23.42 sampled FACT |
| 7 | G | step_end N: pm_gpu_end_w: flush, kc_build (copy of the L buffer), C3D_FrameEnd: GPU starts L | pm_main.c:475, pm_gpu.c:3235-3295 | - | about 1 to 2 GUESS |
| 8 | G | wk ends | pm_main.c:757-760 | - | wk 38.00 FACT |
| 9 | G | idle pass: hand IDLE, pm_lg_show_keys pumps key N: sync60_pace, C3D_FrameBegin(0), K entries, FrameEnd | pm_main.c:645-651, pm_logic.c:172-177, pm_gpu.c:3150-3177 | GPU L of step N | ft - wk 3.21 FACT |
| 10 | L | nuGfxTaskStart: pm_wq_frame_slot waits until key N is walked | pm_nusys.c:169, nd_wq.c:162-163 | G step 9 | fw 9.88 FACT |
| 11 | L | pm_interp_lerp, frame push, audio, step_end and key posts, IDLE publish | pm_nusys.c:169-171, pm_main.c:495-504 | - | inside wwait |
- Longest chain (GUESS from the code order): 6 + 7 (core 0) -> GPU L whole -> 9 -> GPU K whole (overlaps 10, 11, 2, 3) -> 5 returns -> 6.
- Vblank: no frame begin waits for a vblank (fb_flags 0, pm_gpu.c:2989). sync60_pace (:2935-2946) keeps the begins on a 16.7 ms grid. The grid lets begin N+1 start
  33.4 ms after begin N. The period is 41.2 ms, so the pace does not hold the step in the steady state (GUESS; c0i pace and c0k kpace decide).
- The pace split 178/96/26 (2/3/4+ vblanks) is the 41.2 ms period on the 16.7 ms vblank grid: 753 vblanks / 300 steps = 2.51 (FACT arithmetic). Steps do not
  miss a 2-vblank deadline. The serial chain is longer than 33.4 ms.

## 2. Cost accounting
### 2.1 Core 0, per drawn step (113 PICA draws per frame, FACT `gpu d 113`, sc_gd[0] at pm_gpu.c:1400)
- Work 25.16 ms (FACT). Walk 23.42 sampled (FACT), about 22.5 clean (GUESS: about 800 timer pairs x 1.1 us). Begin, end, kc_build, key show, loop: about 2.7 (GUESS).
- Walk by stage (FACT sampled ms; us per unit): model path rec 1.14 + key 0.95 + mtx 0.80 + hit 4.27 = 7.16 (95 models: 75 us each); submit sub 4.20 + upd 1.05
  + emit 1.15 = 6.40 (113 draws: 57 us each); rest 5.93 (cls rest 8.13 less upd and emit: NDT_SUPD, NDT_SEMIT are not in csum, pm_phase.c:610, nd.h:451-453);
  raw 1.89; ent 1.58; spr 0.46. Per PICA draw: 207 us = 55K cycles (FACT arithmetic). A minimal native draw: 2 to 5 us (GUESS).
- By cycle type (GUESS: hw_xl_log.txt calibration istall/imiss = 40.7 cycles, dstall/drmiss about 60): I-fetch stall 62026 x 40.7 = 2.52M (9.4 ms),
  data stall 28649 x 60 = 1.72M (6.4 ms), execute about 2.5M (9.4 ms). Per draw: 550 I misses = 17.6 KB of code fetched (FACT arithmetic).
- The per-draw path is larger than the 16 KB L1 I-cache (GUESS: text sizes FACT from build3ds/pm_3ds.map: model_draw 2648, nd_native_submit 2940,
  nd_emit_draw 2224, pm_gpu_model_hit 1384, pm_tex_bind 1216, gv_matrix_id 1448, C3Di_UpdateContext, the stream runners). Each draw evicts the code of the last.
### 2.2 Core 1, per step
- Busy 28.59 ms (FACT lgms 39.81 - bw 11.22). cyc1 8.58M = 32.0 ms: about 3.4 ms of other threads on core 1 (GUESS). Not on the chain (fw 9.88 FACT).
- The c1t, c1d, c1g, c1x timers run in serial sampled steps on core 0 (pm_logic.c:135-140). The walk runs inside nuGfxTaskStart then (nd_wq.c:155-156), so
  c1x gsub 6.38 (main_loop.c:292) holds the walk rest 5.93 (FACT code read; GUESS match). Non-walk timed sum about 23.3, untimed about 5.3 (GUESS). OPEN, low value.
- I-fetch stall 68013 x 40.7 = 2.77M (10.3 ms GUESS), data stall about 2.0M (7.6 ms GUESS).
### 2.3 GPU
- GPU ms per frame: OPEN in mac_01. C3D_GetDrawingTime = C3D_FrameEnd queue run to the queue end callback (renderqueue.c:238-240, :44-49, :249-252). Its readers
  (pm_main.c:514 interp line, pm_log_hw debug level; pm_phase.c:75-79 phase mode; pm_gpu.c:2996, :3002 switches) print nothing in a prof run: 0 `interp` lines in
  the 4 captures (FACT grep). Older debug logs, other scenes: gpu_us 6593 to 7872 (FACT hw_tidal1_live.txt, hw_menu1_log.txt). Here 2 frames x 7 to 8 ms = 14 to 16 ms
  per step fits the 15 ms of waits (GUESS).
- gb (pm_gpu.c:2998, :3156, :3176, :3293) is not GPU time: it averages 2 intervals, the second spans the next step's CPU and pace time (FACT code read).
- PICA load (GDW read, verified): the PICA transforms the vertices; the CPU makes one matrix product and palette per model (pm_gpu.c:4264-4265); model draws
  index cached vertices in chunk 0 (:1383-1386); about 58 words per draw (nd_emit_draw). OPEN: vertex convert of walker draws (nd_geom_vtx, gv_tri_core), uv scale, LUT builds.
### 2.4 The smallest timers that decide (no profiler)
- c0i (GDW, merged): idle, pace, gx of the L begin (nd_wq.c:56, pm_gpu.c:2990-2995). c0k (ws_CPT0): pace and gx of the key show, GPU ms of the L tail and of the K frame.
- Capture word `profrot` (no code): pmx s istall, dstall on both cores (pm_phase.c:311-317, :838-841): decides the stall share of 2.1.

## 3. Old display list build 3542 against now (FACT live_old_170.txt f1589, live_cap2_170.txt f11489)
| per step | old | now | note |
|---|---|---|---|
| ft / presents per step | 54.8 / 1.5 (kf 150) | 41.2 / 2 (kf 300) | K is a buffer copy now (kc_build pm_gpu.c:3104, key show :3150): no per-draw CPU for K |
| core 0 work (cyc) | 28.2 ms | 25.2 ms | old walked the K frame on half the steps |
| core 0 walk per frame (nd) | 20.4 | 23.4 | old: rec 5.2, sub 3.4, tex 1.7, comb 2.0, rest 2.9 |
| core 0 I misses | 80.4K | 62.0K | |
| core 1 busy | 12.6 (44.1 - 31.5) | 28.6 | the recorder: rmdl 5.25, rnr 3.78, alt 2.03, snap 1.59, rent 1.06, rspr 0.71 |
| core 1 I misses | 33.6K | 68.0K | |
- Done twice now: (1) each state unit is decoded on core 1 (nd_ds_apply, snap) and run again by the core 0 stream walk (rest 5.93); (2) each model is recorded on
  core 1 (rmdl 5.25) and its key, matrix and hit are made again on core 0 (7.16). The old build did each once, on core 0 (rec 5.2).
- Cheaper in the old build: no recorder on core 1 (-16 ms core 1, off the chain); a lighter submit (sub 3.4 against 6.4: no C3Di_UpdateContext per draw, upd 113 now).

## 4. Earlier GUESSes that the data contradicts
- "Core 1 is the long pole" (plan_perf.md 6b, 8): no. fw 9.88 ms of slack per step (FACT).
- "Core 0 30.7 ms" (plan_60fps.md 1, = wk - wwait): no. Core 0 work is 25.2 ms (FACT cyc); wk holds GPU waits.
- DO3 "core 0 -3 to -4": inert with fps60 (NATIVE18 FACT). DO3F merged: next capture (c0s upd, fv).
- GD8 "ft 0 to -5" (plan_gamedraw.md 3): about 0 (GUESS). The L tail (wwait 1.03) runs while the GPU draws K, so the slot wait is not on the chain.
- KICK1 "starts the GPU on the first part" (pm_gpu.c:838-841): no (section 0 item 3).
- "226 draws per step" (2 x 113): the GPU draws 226; the CPU builds 113 (K is a copy). Per draw CPU: core 0 207 us, core 1 232 us (FACT arithmetic).
- Counts: ndc (model 95, ent 11, spr 6, trect 33: records) and gpu d 113 (submits, pm_gpu.c:1400) count different things. A record makes 0 to n draws
  (pm_gpu_model_hit loop :4273-4282, e->n 0 at :4237). No conflict (FACT code read).

## 5. Ranked structural causes (ms on ft; mac_01)
| # | cause | ms on ft now | row | ft gain | model |
|---|---|---|---|---|---|
| 1 | GPU and core 0 strictly serial: GPU L after the walk, the next walk after GPU K | about 15 GUESS | CPT0 (measure), KICK3 | -3 to -4.5 GUESS | Haiku |
| 2 | core 0 walk of the L frame: 113 draws x 207 us, I-cache stalls | 22.5 GUESS | DO2, DO3 step 2, K1 (plan_drawobj.md) | -3 to -7 GUESS | Sonnet |
| 3 | the next walk waits for GPU K (citro3d: one frame in flight) | about 7 GUESS | PIPE: late frame begin, design pass | -4 to -7 GUESS | Opus, user yes |
| 4 | frame end, kc_build, key show, begin on core 0 | about 2.7 GUESS | after CPT0 | -0.5 to -1 GUESS | - |
| 5 | core 1 work (off the chain) | 0 | GD7, GD6, DO4 give margin only | 0 now | - |
- Target: 30 steps per second (60 fps) needs the chain at 33.4 ms or less: -7.8 ms. Rows 1 and 3, or rows 1 and 2, reach it (GUESS). Then core 1
  (28.6 + about 1 ms tail) is the next limit, under 33.4 (GUESS).
- PIPE design (not a page): record L N+1 into a second command buffer before C3D_FrameBegin; do C3D_FrameBegin at the kick point and submit that buffer.
  Needs per-frame ownership of the ring chunks (pm_gpu.c:2975, :2997 ring_done), the texture staging gate (pm_tex.c:698, :795), the off-screen slot reset
  (pm_gpu.c:2962-2971), kc_base and the K record (:3011-3016), the stereo record. Fallback: the frame begin as now.

## 6. Order
1. CPT0 and KICK3 (Haiku, disjoint lines: pm_gpu.c :3154-3155 and :2995 against :859; pm_phase.c only CPT0). Merge both.
2. One capture: `--profile prof warp=mac_01:0 pos=0:0:200 profdisk=1 profrot`, capture_check first, 120 s. Read ft, wk, c0i, c0k, pmx s, c0s, cls.
3. With gpul much smaller than gpuk, KICK3 works. Then ask the user for the PIPE design pass (Opus). DO2 and DO3 step 2 pages go ahead (core 0 walk).
