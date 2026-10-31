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
