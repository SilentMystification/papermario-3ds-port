# agent_jumptest: jump test (debug tool, SR4 step 1 proof)

Plan: docs/handoff/plan_stateful_records.md row SR4 step 1. Layout input: docs/handoff/inv_citro3d_decode.md section 3 (INFERRED, not confirmed).
Switch: `jumptest` (pm_debug_has, read once in pm_gpu_init, default off). Code: 3ds/src/pm_gpu.c, block JT1 (before pm_gpu_init) and the hook before PH_PUSH(PH_FEND) in pm_gpu_end.
Debug tool, not a finished path. The record build (SR4) does not use it.

## Command words (written by GPUCMD_AddWrite, order [param][header])

Header = 0x000F0000 | reg (mask 0xF, single write, n-1 = 0).

| Step | Where | Reg | Param | Header |
|---|---|---|---|---|
| 1 | frame list tail | CMDBUF_SIZE0 (0x238) | 2 (16 bytes >> 3) | 0x000F0238 |
| 2 | frame list tail | CMDBUF_ADDR0 (0x23A) | pa(continuation) >> 3 | 0x000F023A |
| 3 | frame list tail | CMDBUF_SIZE1 (0x239) | 2 | 0x000F0239 |
| 4 | frame list tail | CMDBUF_ADDR1 (0x23B) | pa(sub) >> 3 | 0x000F023B |
| 5 | frame list tail | CMDBUF_JUMP1 (0x23D) | 1 | 0x000F023D |
| 6 | frame list tail, only if pad | FRAGOP_CLIP (libctru enum) | 0 | 0x000F0000 \| FRAGOP_CLIP |
| 7 | sub-buffer, 16 B, linear | FRAGOP_CLIP | 0 | (same) |
| 8 | sub-buffer | CMDBUF_JUMP0 (0x23C) | 1 | 0x000F023C |
| 9 | continuation (written by GPUCMD_Split at FrameEnd) | FINALIZE x2 | 0x12345678 | 0x000F0010 |

Pad (step 6): one 2-word write when (off + 10) is not a multiple of 4 words, so the continuation starts on a 16-byte boundary.
The continuation size is 16 bytes only if GPUCMD_Split appends exactly the FINALIZE pair plus its padding (inv section 1), and no other write follows step 5 before the split.

## Inferred fields (not confirmed)

- SIZEn param = bytes >> 3. ADDRn param = physical address >> 3 (osConvertVirtToPhys).
- JUMP1 write starts the sub-buffer. JUMP0 at the end of the sub-buffer fetches channel 0 (ADDR0/SIZE0) = the continuation.
- No hardware return: the continuation is the rest of the frame list, so the test sits at the end of the frame list (before C3D_FrameEnd).
- CLIP=0 is used as the no-op write: it changes the clip register only if clip is on at frame end. No draw follows it in this frame.

## Log and readback

- Queued at the end of frame 1 (jt_state 1). Read at the end of frame 2 (jt_state 2): gspWaitForP3D, then one line.
- Line: `jump test ok size0=2 addr0=<cont> size1=2 addr1=<sub> jump1=1 cont_word=<word index>`. The values are the ones written, not read back from the GPU.
- `fail` is not printed. libctru gspWaitForP3D has no timeout. A hang shows as no line.
- Not run on Azahar (the Azahar GPU model does not run jumps). The console run decides.

## Gate

- Build: `python 3ds/tools/gen_map_restore.py`, `sh 3ds/build.sh`: exit 0. No warning on pm_gpu.c. map_restore_table.c restored from a kept copy, not committed.
- Picture: unchanged when jumptest is off (all code gated by jt_on).

## Open items

- Confirm on the console: the sub-buffer runs, the frame does not hang, and the picture matches the switch-off build.
- No GPU-side register readback. Needs a read path for CMDBUF registers (not in the tree).
- Continuation size 16 B depends on citro3d C3D_FrameEnd adding no writes before the split. Not checked.

## JT2: readback (replaces the JT1 log line)

- Sub-buffer (16 B) now writes `FRAGOP_CLIP_DATA3` (0x4B) = `JT_MAGIC` 0x4A54534B, header 0x000F004B, then JUMP0. The CLIP=0 write is removed from the sub. The tail pad still writes CLIP=0 when needed.
- Readback at the end of frame 2, after gspWaitForP3D and before C3D_FrameEnd: `GSPGPU_ReadHWRegs(JT_HW_BASE + 0x4B*4, 4)`, JT_HW_BASE 0x400000. Frame 2 is not submitted yet, so the register holds frame 1 plus the sub.
- Log lines: `jump test ok value <hex>` (equal to JT_MAGIC), `jump test fail value <hex>` (other value), `jump test none (readback result <hex>)` (GSP call failed). The layout line is debug only.
- INFERRED: GSPGPU_ReadHWRegs base 0x1EB00000 (libctru LCD regs use 0x400478), PICA regs at 0x1EF00000. Not confirmed. A wrong base gives `fail` or `none`, not a false `ok`.
- Visible mark: NOT built. Register writes alone change no pixel. A mark needs a draw in the sub-buffer (attribute buffer, vertex data, TEV and texture state). Open item.
- Build: gen_map_restore.py, build.sh exit 0, no pm_gpu.c warning.

## JT3: repeat once per second (replaces the one-shot run)

- Trigger: in pm_gpu_end, state 0 and (no jump yet or pm_loop_tick - jt_last >= SYSCLOCK_ARM11, one second). pm_loop_tick is the `now` the main loop already reads once per pass (pm_main.c, set after svcGetSystemTick at the top of the loop). No new svc call.
- Each trigger queues one jump in the frame command list (jt_queue). The next frame end reads the result (jt_result), then state returns to 0.
- Result lines, warning level, frame = pm_frame_count() at the queue:
  - `jump f<frame> ok value <hex>` (equal to JT_MAGIC)
  - `jump f<frame> fail value <hex>`
  - `jump f<frame> none` (GSP readback call failed; the result code is in a debug line)
- Layout line stays debug level.
- Switch off: no change (all code gated by jt_on).
- Interval: 1 s (SYSCLOCK_ARM11 ticks).
- Only the main loop's drawn step reaches jt_queue. The nusys path (pm_nusys.c) calls pm_gpu_end with the last pm_loop_tick value.
- Build: gen_map_restore.py, build.sh exit 0, no warning on pm_gpu.c or pm_main.c. Not run on Azahar.
