# plan_pipe.md: PIPE, late frame begin: core 0 walks the next L frame while the GPU draws the K frame (oracle NATIVE19; user yes 20 Nov: design, pages only)
Tags: FACT (log line or code read), GUESS, OPEN. Base: plan_critpath.md. Line numbers: main 4a89ac5db7 (CPT0, KICK3 merged). citro3d renderqueue.c, libctru gxqueue.c, gpu.c (devkitPro clones).
The user reads this page before a PIPE coder starts.

## 1. Order today (FACT code read)
1. L begin (pm_gpu.c:2958-3035): bookkeeping, texture upload (:2987), sync60_pace (:2995), C3D_FrameBegin(0) (:2998): waits until the GX queue is empty
   (renderqueue.c:92-100, :170), so until the GPU has drawn the K frame of the last step. Then GPUCMD_SetBuffer(ctx->cmdBuf, size, 0) (renderqueue.c:175).
2. L walk (pm_nusys.c:124-139): about 22.5 ms (GUESS; 23.42 sampled FACT). Up to the kick (55 percent of the draws, pm_gpu.c:3279) the GPU is idle: the queue is stopped (renderqueue.c:97).
3. L end (pm_gpu.c:3245-3305): flush, kc_build (:3110: copies the L lists to kc_buf, patches the K words), C3D_FrameEnd (:3301): queue run; KICK3 (:860) starts part 1 at the kick.
4. Key show (pm_gpu.c:3156-3185): sync60_pace, C3D_FrameBegin(0) (waits for GPU L), K entries from kc_buf, the K tail list memcpy to cmdBuf word 0 (:3181), FrameEnd.
5. citro3d has one frame in flight: every C3D_FrameBegin waits for the whole queue. inFrame is static (renderqueue.c:11): no second open frame.
- Core 0 waits twice per step: at 1 for GPU K, at 4 for GPU L. Idle 16.05 ms per step (FACT arithmetic, plan_critpath.md 0).

## 2. New order (PIPE3, with KICK3 merged)
1. Early begin: the bookkeeping of 1 runs, but no C3D_FrameBegin. GPUCMD_SetBuffer(cmdBuf + PIPE_H, size - PIPE_H, 0) (libctru gpu.c). The frame buffer and the
   viewport are set as C3D_FrameDrawOn sets them (C3D_SetFrameBuf, C3D_SetViewport: no inFrame test). Core 0 starts the walk at once.
2. Open (pm_pipe_open, new): save GPUCMD_GetBuffer, sync60_pace, C3D_FrameBegin(0), GPUCMD_SetBuffer(the saved values), target->used = true, then the
   after-begin part of today (ring_done :3003, gb, kc_base, kc_rec, sr_frame_begin). The early words stay in place: one list from cmdBuf + PIPE_H, as today.
3. Open runs at the first of: a fence (section 3), the kick (KICK3: the GPU then starts part 1 at once), the frame end. A stereo frame opens at the early begin.
4. So the walk up to the open runs while the GPU draws K (useful core 0 work in today's wait). The pace grid moves into the open: the present spacing stays 16.7 ms.
- Expected (GUESS): the L begin wait (c0i gx) goes to about 0; ft = max(33.4 pace grid, core 0 work + key wait, core 1 + wwait) = about 33.4 (60 fps edge in mac_01).
- K copy: kc_build reads [kc_base, end) (:3121-3134); kc_base = cmdBuf + PIPE_H for every L frame (PIPE1), so kc_build does not change.

## 3. Fences: events before which the open must run (the GPU must have finished K first). PIPE0 counts them.
| kind | site | why |
|---|---|---|
| split | split_frame pm_gpu.c:304, cmd_split_forced :1546, sr_split :2521, grab_impl :2832 | C3D_FrameSplit does nothing without inFrame (renderqueue.c:192): a lost list |
| ring | nd_native_submit :1385 (a draw on the walker ring, not ndt_mdl, not gi_kind 1) | ring_frame_begin reuses chunk 0 every frame (pm_gpu_rect.c:28-32): K reads the last L's vertices there |
| tex | load_miss pm_tex.c:1412, glyph cell fill :342 | the CPU writes texels; K may sample the slot (epoch e-1 slots are kept, :976, but a cell is rewritten in place) |
| off | pm_gpu_off_begin pm_gpu.c:2229 | C3D_FrameDrawOn of the off-screen target needs inFrame (renderqueue.c:181) |
| room | pm_gx_room :2642 (24 entries), pm_gx_drain :2659 | they wait for and clear the queue |
- The static vertex area is append-only inside a map (pm_gpu.c:3614-3617 comment): no fence. A map load drains the walker (pm_walk_drain).
- If ring fences come before the kick (PIPE0 line), PIPE2 gives each frame parity its own chunk 0 (2 buffers, the static area written to both).

## 4. Rows (ranked by ft value; ms GUESS)
| # | row | model | what | needs | gate | ft |
|---|---|---|---|---|---|---|
| 1 | PIPE0 | Haiku | census line `pipe`: first draw index per frame of each fence kind and of the kick | none | spot_mac01_fps60_det, capture_check | 0 (decides) |
| 2 | PIPE1 | Haiku | L frame lists start at cmdBuf + PIPE_H (32768 words); the K tail stays at word 0 | none | spot_mac01_fps60_det, spot_mac01_drec | 0 (needed by 4) |
| 3 | PIPE2 | Sonnet | ring parity: rchunk[0] vtx and idx swap between 2 buffers at ring_frame_begin; gstat writes go to both | PIPE0 ring < kick | fps60_det, drec | 0 (needed by 4) |
| 4 | PIPE3 | Opus | early begin and pm_pipe_open at the first fence, the kick or the frame end (section 2) | PIPE1, KICK3, CPT0 data | fps60_det, stereo_det, spot_mac01_drec, entcape_kmr03_drec | -4 to -7 |
| 5 | PIPE4 | Sonnet | key show without the wait for GPU L: the K submit waits for the next poll point with an empty queue | PIPE3 | the same 4 benches | -2 to -3.5 |
Why Opus for PIPE3: it changes when citro3d is in a frame; every inFrame call (C3D_FrameDrawOn, C3D_FrameSplit) and every write to GPU-read memory needs a read.
Why Sonnet for PIPE2, PIPE4: PIPE2 changes a buffer that 7 readers use (pm_gpu.c:537, :732, :1385, :1588, :1905, :3622, :3625); PIPE4 keeps the K pace and noise order.

## 5. What the console lines must show (CPT0 c0k, GDW c0i) before a row is worth it
- PIPE3: c0i gx at least 2 ms per step (the L begin waits for GPU K) and c0i pace under 1 ms; PIPE0: first fence after 30 percent of the draws (or PIPE2 first).
- PIPE4: c0k kgx at least 2 ms per step after KICK3 (the key show waits for GPU L).
- No PIPE row pays when c0i pace is large: then the 33.4 ms grid holds the step, and only core 0 work rows (plan_drawobj.md DO2, DO3) move ft.
- gpuk (K GPU ms) is the ceiling of PIPE3: the gain is at most min(c0i gx, walk time before the open).

## 6. Risks and fallbacks
- Hang: a GPUCMD write between a C3D_FrameEnd and the early begin writes through a NULL buffer (renderqueue.c:206). Rule: the early begin is the first walker
  step of a frame (pm_gpu_begin_w :3042). Fallback PIPE3: revert (open at the begin, as now).
- A lost list: a split or a target switch before the open (C3D_FrameSplit, C3D_FrameDrawOn need inFrame). The fence list prevents it; Azahar shows it as a DIFF.
- Wrong K picture: the CPU writes memory K reads (ring, a texture slot, the K tail). PIPE2, the tex fence and PIPE_H (K tail at word 0 < PIPE_H) prevent it.
  kc_tail_n over PIPE_H: open at the early begin (counted).
- Tearing and a missed swap: no new path. The swap stays in citro3d's queue callback (renderqueue.c:62-66); the pace grid keeps the present spacing.
- Input latency: no change in the pad path (pm_main.c:699); a picture can appear up to the saved wait earlier (GUESS 0 to -7 ms).
- Fallback per row: PIPE0 delete the census; PIPE1 revert (lists at word 0); PIPE2 fence at the first ring draw instead (smaller gain); PIPE4 revert.

## 7. Memory (static margin 382 KB, coordinator FACT; linear pool 14208 KB FACT `mem boot`)
- PIPE0: about 100 B .bss and code (GUESS); a 4 KB page step is possible (hw_guidelines.md). PIPE1: 0 (inside the 1.5 MB command buffer, pm_gpu.c:1855;
  worst fill 369 KB + 128 KB FACT comment). PIPE2: +640 KB linear heap, 0 static ((16384 + 16384) x 16 B + 65536 x 2 B, pm_gpu_int.h:68, FACT arithmetic);
  free linear memory OPEN: the PIPE2 page reads a mem line first. PIPE3, PIPE4: under 1 KB code (GUESS).

## 8. Order
1. PIPE0 and PIPE1 now (Haiku; pages ws_PIPE0, ws_PIPE1). CPT0 and KICK3 are merged.
2. Capture (plan_critpath.md 6) after PIPE0 and PIPE1 merge. Read c0i, c0k, pipe.
3. Section 5 decides: PIPE2 page (if ring < kick), then PIPE3 (Opus, user yes for the model), then PIPE4.
