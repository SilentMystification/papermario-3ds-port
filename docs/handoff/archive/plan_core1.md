# plan_core1: game producer on core 1, one frame ahead (epic CON, row CORE1)

Plan only (Opus). Rules: CLAUDE.md 2, 5, 7; docs/handoff/hw_guidelines.md. Goal: O3DS wall under 36 ms.
Tags: [M] measured (log line named), [C] read from code, [I] inference, [E] estimate.
Console data: build3ds/ftp_170/hw_o3ds_log.txt, build 1587 (git aa54485e5, after MR1 and LB1b), widescreen off, Toad Town.

## 0. Decision: NO-GO
CORE1 does not reach 36 ms. It moves the critical path from the producer to the GPU tail. Expected gain 0 to 2 ms, not 3 to 6 ms. Section 5 has the arithmetic. Section 6 names the reopen rule and the one measurement that decides it.

## 1. Console data
| fsplit | logic | dl | gbi | flush | gwait | swap | rest | wall |
|---|---|---|---|---|---|---|---|---|
| f8100 (#1003) | 3.3 | 3.8 | 34.6 | 0.2 | 0.1 | 0.7 | 0.4 | 43.1 |
| f8400 (#1030) | 3.3 | 3.8 | 34.6 | 0.2 | 0.1 | 0.9 | 0.4 | 43.3 |
| f8700 (#1079) | 3.3 | 3.8 | 34.6 | 0.2 | 0.1 | 0.8 | 0.5 | 43.3 |
| f9000 (#1102) | 3.7 | 3.9 | 33.8 | 0.2 | 0.1 | 0.9 | 0.4 | 43.0 |

- pmu cyc 11.22 to 11.39 M per frame on core 0 (#999, #1026, #1075, #1098) = 41.9 to 42.5 ms at 268 MHz. Core 0 is busy for about 98 % of the wall [M].
- Instruction-fetch stall 4.50 to 4.67 M cycles per frame, about 40 % of core 0 [M #1026, #1098].
- pace: 23.1 to 23.3 steps/s. Intervals: 2 vblanks 44 to 51, 3 vblanks 66 to 71 per 5 s [M #990, #1004, #1010, #1017].
- Build 1579 before LB1b (plan_con_gate.md 1): logic 3.2, gwait 2.3 to 2.6, rest 1.0. Then the bg task waited before the dl build [M].

## 2. Where the frame work sits today (one thread, game_thread on core 0)
- 3ds/src/pm_main.c:682 creates game_thread on core 0. pm_frame_loop 315. Per drawn step: 445 pm_gpu_begin(1) (sets begin_pend), 461 pm_gfx_retrace, 463 pm_audio_step, 470 pm_gpu_end (C3D_FrameEnd, swap).
- 3ds/src/pm_nusys.c:153 pm_gfx_retrace -> gfx_func(0) = gfxRetrace_Callback (src/main.c:110). On the step call (D_80073E0A == 0): step_game_loop (logic, src/main_loop.c:56-156), gfx_task_background (158-174), gfx_draw_frame (187-303). The two builds are the "dl" split.
- nuGfxTaskStart (pm_nusys.c:99-125). The bg task is stored (LB1b, 105-107). The swap task kicks audio (102), runs the bg list, then the main list through gbi_task (76-86). gbi_task calls pm_gpu_begin_wait (78), then pm_gbi_run (the "gbi" split).
- pm_gpu_begin_wait (3ds/src/pm_gpu.c:1671-1700): resets the vertex ring, index ring, palette ring, off-screen slots and grab state, uploads staged textures (1696), then C3D_FrameBegin(0) at 1700. C3D_FrameBegin(0) waits until the GPU has finished the previous frame. This is "gwait".
- The GPU runs a frame in parts: two C3D_FrameSplit kicks at about 55 % and 85 % of the draws (kick_check, pm_gpu.c:541-554), and the rest at C3D_FrameEnd. The work after the last kick is the "GPU tail".

## 3. Shared data and race risk if logic + dl of step N+1 run on core 1 while core 0 walks step N
| Item | Writer (core 1) | Reader (core 0 walk) | Risk | Required action |
|---|---|---|---|---|
| DisplayContext D_80164000[2]: mainGfx, backgroundGfx, matrixStack, camPerspMatrix (src/main_loop.c:15) | dl build of N+1 into context i^1 | walk of N in context i | LOW. The game alternates contexts for the N64 RCP overlap [C main_loop.c:301, model.c:2812 comment] | Producer starts step N+1 only after walk N-1 is complete (depth 1). The DISABLE_DRAW_FRAME flip (main_loop.c:192) then stays safe |
| model->savedMtx, model->finalMtx (src/model.c:2810-2855) | logic and dl | G_MTX of N | LOW. savedMtx is written only when freshness reaches 0, from the other context copy [C] | none |
| group transformMatrix (model.c:3221), gCameras[].mtxBillboard (model.c:2039) | scripts and camera update of N+1 | G_MTX of N by pointer | MEDIUM. Same exposure as the N64 RCP. A torn or one-frame-new matrix is possible [I] | Accept as N64 behaviour, or copy into matrixStack at emit (2 sites) |
| Vertex buffers: ImgFX (src/imgfx.c:330-336, 1410+), msg (msg.c, msg_draw.c) | dl of N+1 | G_VTX of N | LOW. Indexed by gCurrentDisplayContextIndex [C] | none |
| Player raster cache (src/101b90_len_8f0.c:324-354) | DMA of a new raster | texture load of N | LOW. lazyDeleteTime = 2 keeps a used slot for 2 steps [C] | none |
| Texture cache slots and index (pm_tex.c:918 pm_tex_invalidate) | every ROM read (pm_os.c:394, up to 6,946 per step [C pm_tex.c:416]) and background_gfx.c:304 | pm_tex_load probes in walk N | HIGH. Unsynchronised writes to the slot table and lidx during probes: wrong bind or freed slot [I] | Queue the spans on core 1 (fixed ring, overflow sets an all-stale flag). Core 0 applies the queue before walk N+1. pm_os.c:394 already tests the thread (pm_on_audio) |
| pm_dlc_gen (pm_dlc.inc.c:7, bumped pm_tex.c:854, pm_map.c:379) | map shape write | dlc_try of N (441-447) | HIGH. A bump in the middle of walk N resets dlc during the walk | Same queue: the gen bump is applied by core 0 at the next walk |
| Map shape data, map heap, model and entity display lists | load_map, battle load, heap reset | G_DL targets of N | HIGH. Walk N follows pointers into a buffer that is being decoded (pm_map.c:375). Crash class [I] | Join (wait for walk N) before pm_map.c:358-379 decode, before any heap reset, and at set_game_mode |
| nuGfxCfb_ptr (pm_main.c:408), gDisplayContext | frame loop and producer per step | pm_is_cur_cfb (pm_main.c:191), pm_gbi.c:236-250, 3267, pm_gpu.c:1731 | HIGH. The walk tests the current frame buffer against a value of the next step | Store cfb_ptr and the context pointer in the task record. The walk reads the record |
| G_SETCIMG / G_SETTIMG of a frame buffer: grab and capture (pm_gbi.c:2881, 3363; pm_main.c:203, 265) | none on core 1 | core 0 only | LOW once cfb is per task | none |
| gRandSeed and game globals (gGameStatus, gOverrideFlags) | logic | walk does not read them [C grep of pm_gbi.c, pm_gpu.c, pm_tex.c] | NONE | none |
| pm_interp records (FI_OPEN/FI_CLOSE in gfx_draw_frame) | dl | pm_interp_lerp in walk | HIGH with fps60 on | fps60 uses depth 0: the producer waits for the walk before its step (one code path, depth 0 or 1) |
| Audio kick and join (pm_main.c:279-287, pm_nusys.c:102, 148, 154) | producer | audio thread on core 1 | MEDIUM. Protocol is per game thread already. The producer and the mixer now share core 1 under the 80 % limit (pm_log.c:610) | Producer priority below the audio thread |
| Options menu and warp (pm_main.c:484 pm_audio_join_at(3): "warp and fxtest can call game code"), pm_wide | core 0 frame loop | producer | HIGH | Run on the producer between steps, or join first |
| fsplit, phase counters (FS_MARK, PH_PUSH, pm_ph_cnt) | both threads | log | LOW (debug) | per-thread marks; logic and dl move to a core-1 line |

## 4. Shape of the change (only if reopened, section 6)
- New producer thread on core 1 (stack 256 KB as game_thread, pm_main.c:682), priority below the audio thread. It runs pm_pad_scan, pm_gfx_retrace and the FS1 skip decision (pm_main.c:361-440).
- nuGfxTaskStart (pm_nusys.c:99) records {list, size, kind, cfb_ptr, context, invalidate queue} in a 2-slot ring. Core 0 loop: wait for a slot, pm_gpu_begin, drain the invalidate queue, run bg then main through gbi_task, pm_gpu_end, release the slot.
- Join points: pm_map.c:358-379, heap reset, set_game_mode, options menu, exit and suspend (aptMainLoop hook runs on the thread that calls it).
- Files: 3ds/src/pm_main.c (loop split, thread), 3ds/src/pm_nusys.c (task ring), 3ds/src/pm_os.c:394 and 3ds/src/pm_tex.c:918 (queue), 3ds/src/pm_map.c:375 (join), 3ds/src/pm_main.c:191 and pm_gbi.c:236-250, 3267 and pm_gpu.c:1731 (cfb from the record). About 300 lines [E plan_toad_33].
- Cost: +1 step of input latency (33 ms) with depth 1.

## 5. Why the gain is small: the GPU tail becomes exposed
- C3D_FrameBegin(0) (pm_gpu.c:1700) blocks until the GPU finishes the previous frame. citro3d and the port reuse one command buffer and one vertex ring per frame (pm_gpu.c:1686-1689), so walk N cannot start before the GPU tail of N-1 ends [C].
- Build 1579 order (before LB1b): FrameEnd, rest 1.0, logic 3.2, then the wait 2.3 to 2.6 ms. So the GPU tail after FrameEnd is about 1.0 + 3.2 + 2.4 = 6.6 ms [M-derived, plan_con_gate.md 1].
- Build 1587 order (after LB1b): rest 0.4, logic 3.3, dl 3.8 = 7.5 ms before the wait, so gwait is 0.1 [M]. The producer work hides the tail today.
- With CORE1, core 0 goes from FrameEnd to the next walk in about rest 0.4 ms. gwait grows to about 6.6 - 0.4 = 6.2 ms [E].
- Core 0 frame with CORE1 = gbi 34.6 + flush 0.2 + swap 0.8 + rest 0.4 + gwait 6.2 = about 42.2 ms [E]. Today 43.1 ms. Gain about 1 ms (range 0 to 2 ms, the tail may vary by 1 ms). Core-1 bus traffic on O3DS FCRAM (no L2) can slow the walk, which is already 40 % fetch stall [I].
- Best case even with a zero GPU tail: gbi 34.6 + 0.2 + 0.8 + 0.4 = 36.0 ms. Not under 36 ms.
- The 3 to 6 ms estimate (plan_dl_finish.md, plan_toad_33.md) assumed gwait stays at 0.1 ms. It does not.
- Judgement: 300 lines, 4 HIGH races, a crash class at map loads, +33 ms input latency, for about 1 ms. Not worth it.

## 6. Reopen rule and the deciding measurement
- Row GTAIL (diagnostic, about 15 lines, no pixel change): with fsplit, log per 300 frames the time from C3D_FrameEnd (pm_gpu.c:1724 area) to GPU idle (gspWaitForP3D or the C3D queue-done callback time stamp), and the GPU busy time per frame. Scenario: mac01t_fsplit for the line only. The value comes from the console at the next epic push.
- CORE1 reopens only if both hold on the console: GPU tail at most 2 ms, and gbi + flush + swap + rest + tail under 36 ms. Today gbi alone (33.8 to 34.6 ms) leaves 1.4 to 2.2 ms for the rest. So the walk must also drop first.
- If the tail is large, the next row is a deeper GPU pipeline (two command buffers and two vertex rings, so walk N can start while the GPU finishes N-1), not CORE1. That row needs its own plan. If the GPU busy time per frame is above 36 ms, no CPU row reaches the goal.
- The path to 36 ms today is the walk itself: gbi 34.6 ms with 40 % instruction-fetch stall.

## 7. Gate if CORE1 is ever coded (rule 5)
- Quantity: console wall from fsplit, Toad Town steady, widescreen off: under 36.0 ms, against 43.0 to 43.3 ms (build 1587). Azahar shows direction only; Azahar gwait is 0.0, so Azahar cannot show the exposed tail.
- Scenario: mac01t_fsplit (log 4 of 4, top shots 2 of 2 byte-equal to HEAD). The change does not touch the shared draw path (pm_gbi.c ops, pm_gpu.c draw, pm_tex.c lookup); only the frame buffer read and the invalidate queue move. Byte-equal scenes that use those paths: pause_menu (frame buffer grab of the pause background) and one map load scenario from test_catalog.md (the join at pm_map.c). No other scenes.
- Stop rule: a changed shot, a crash or hang at the map load, or console wall not below 41 ms at the epic push: revert all row files. No second try.
