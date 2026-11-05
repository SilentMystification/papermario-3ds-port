# diag_jump_f330: console log stops after the area 5 map 5 load (jumptest on)

Console facts (from the orchestrator): build 1639, git 06612dd89, widescreen off. Jumptest on: `jump f<n> ok value 004a5453`
at 11 checks f0-f300. Frame 327 loads area 5 map 5, then about 6 lines, then no `jump f330` line. Jumptest off, same build: the
same load, normal frames, clean exit. Azahar (build 1633 era, jumptest on) passed f420.
Earlier console fact (diag_jump_hang.md, build 1624): f330 layout `cont_word 12`, addr0 = buffer start + 48. The f330 line
printed, then watchdog at step 332.
State tags: CONFIRMED (source, doc disassembly, log) or INFERRED.

## 1. What the f330 jump does differently

- CONFIRMED, schedule: the jump is queued in pm_gpu_end once 1 s (SYSCLOCK_ARM11 ticks) has passed and state is 0
  (pm_gpu.c:1845). The result is read at the next frame end (pm_gpu.c:1844). f330 is the next 1 s slot after f300. The schedule
  has no map-load case.
- CONFIRMED, why addr0 moves: `jt_cont_word = off + 10 + pad` and `addr0 = pa(buf + jt_cont_word)` (pm_gpu.c:1161-1164). `off`
  is the word count of the frame list at the queue point, so addr0 follows the frame's command count. It is not a new
  allocation.
- CONFIRMED, f330 is a frame with no draw (build 1624 log): cont_word 12, so 2 words came before the 10 jump words. A drawn
  frame held 864-1538 words (diag_jump_hang.md, log facts). Every jump f0-f300 on build 1639 was in a drawn frame (the `ok`
  lines; the map load is the first frame with no draw at a 1 s slot). INFERRED for build 1639: its f330 layout line never printed.
- CONFIRMED, the continuation of a no-draw jump is FINALIZE x2. GPUCMD_Split writes FINALIZE and repeats it while `off & 3`
  (inv_citro3d_decode.md:41-43). The continuation starts on a 4-word boundary (pm_gpu.c:1162-1163). No draw means no FLUSH,
  INVALIDATE, EARLYDEPTH_CLEAR (diag_jump_hang.md section 2). So: 0 words + FINALIZE -> 2 words, `2 & 3` -> second FINALIZE.
  - Drawn jump (f0-f300): continuation = FLUSH, INVALIDATE, EARLYDEPTH_CLEAR (6 words) + one FINALIZE = 8 words.
  - Baseline no-draw frame (jumptest off): 2 words + one FINALIZE = 4 words. One FINALIZE.
  - So the f330 jump is the only list in the run with two FINALIZE writes inside a jump target. It is also the only jump in a
    16-word list (jump writes at words 2-11, inside the first 64 bytes).
- CONFIRMED, the sub-buffer is not freed or reset at the map load. `jt_sub = linearAlloc(32)` once (pm_gpu.c:1152). A grep of
  3ds/src finds no linearFree of jt_sub and no linear heap reset. The linearFree calls (pm_gpu.c:322, 365; pm_tex.c:599, 827,
  1233; pm_dspaud.c:63) free their own blocks only.
- CONFIRMED, an overwrite before the queue cannot reach the GPU: jt_queue rewrites all 4 sub words and flushes them
  (pm_gpu.c:1156-1160) in the same pm_gpu_end that submits the list (C3D_FrameEnd, pm_gpu.c:1849). No other code runs in
  this thread between the two.
- INFERRED, low: another thread (audio, pm_dspaud.c:63/107) writes into the 32 sub bytes through a stale pointer between the
  flush and the GPU read. No evidence for it.
- CONFIRMED, the continuation is inside the citro3d command buffer (C3D_Init 1.5 MB, pm_gpu.c:1198), not freed at map load.

## 2. SIZE0 at the map-load frame

- CONFIRMED: `jt_size0 = drew ? 4 : 2` (pm_gpu.c:1166). `drew` is 0 at frame begin (pm_gpu.c:1764) and is 1 only after a
  draw (pm_gpu.c:844, 1060). At the map-load frame with no draw, SIZE0 = 2 = 16 bytes = the FINALIZE x2 continuation. The
  size matches the tail on paper.
- CONFIRMED: a drawn frame does not leave the GPU waiting. The end-of-frame writes (FLUSH, INVALIDATE, EARLYDEPTH_CLEAR,
  FINALIZE) come from C3D_FrameEnd (pm_gpu.c:1849) after the jump writes, so they are in the continuation, and SIZE0 = 4
  covers them. Console: 11 drawn jumps `ok`, no hang.
- INFERRED, low: `drew` can disagree with the citro3d draw flag when a split (pm_gpu.c:290, 551, 1675) comes after the last
  draw. Then the tail is 16 bytes and SIZE0 = 4 reads 16 stale bytes. Each split path in the source is followed by draws
  (kick at 55/85 percent of the draws, grab split then the quad at 1678+), so this is unlikely at f330.

## 3. Hang path (INFERRED)

- The f330 list does not give the gx queue its completion interrupt (P3D). C3D_FrameBegin of frame 331 (pm_gpu.c:1779) then
  waits forever (gxCmdQueueWait timeout -1, diag_jump_hang.md section 3). jt_result runs after that wait (pm_gpu.c:1804 before
  1844), so no `jump f330` line prints. This matches build 1639.
- Build 1624 printed the f330 line and stopped at step 332: the gx queue counts any PSC0, PPF, P3D or DMA interrupt as one job
  (diag_jump_hang.md section 3). A map-load GX job (texture copy or fill) can give one extra count, so the stop moves one
  frame later. Same cause, different timing.
- Azahar does not model the jump on the console's terms (agent_jumptest.md:79), so Azahar f420 does not clear this path.

## Causes, most likely first

1. INFERRED. Two FINALIZE writes inside a jump target. The console stops the jump or loses the P3D when the continuation holds
   FINALIZE x2. Only the no-draw jump has it. All 11 jumps with one FINALIZE passed.
2. INFERRED. The jump sits in the first 64 bytes of a 16-word list (command fetch or prefetch of a very short list).
3. INFERRED, low. Sub-buffer overwrite or SIZE0 mismatch (section 1, 2).

## The one test

Change jt_queue (pm_gpu.c:1171-1172): when `!drew`, add one `GPUCMD_AddWrite(GPUREG_FRAGOP_CLIP, 0)` after the JUMP1 write
and the pad. That write sits at cont_word, so the continuation becomes CLIP=0 + one FINALIZE = 16 bytes. SIZE0 stays 2. The
no-draw frame, the short list and the map load stay the same. Only the FINALIZE count changes (2 -> 1).

- Pass: `load map area 5 map 5`, then `jump f330 ok value 004a5453` (f328-f332), and later `jump f<n> ok` lines. Cause 1 is
  CONFIRMED.
- Same stop (no `jump f330` line): cause 1 is out. Next test: skip the jump when `!drew` (cause 2 against the map load).
