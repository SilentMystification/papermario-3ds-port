# diag_jump_hang: jumptest readback "fail 00100200" and the watchdog hang at step 332

Build 1624, git 597b0f00e. Log: build3ds/ftp_170k/hw_o3ds_log.txt.
Sources: 3ds/src/pm_gpu.c, 3ds/src/pm_main.c, 3ds/src/pm_wdog.c, libctru and citro3d from the image `pm3ds-build:1`
(headers under /opt/devkitpro/libctru/include, objects disassembled with arm-none-eabi-objdump, reproduce commands at the end).
State tags: CONFIRMED (source, disassembly or log) or INFERRED.

## Log facts

| Log line | Fact |
|---|---|
| #20 | Boot frames draw 21 draws per frame. |
| #25-26 | f270 fail 00100200, cont_word 876, addr0 2018a9b0. |
| #32-33 | f300 fail 00100200, cont_word 1548, addr0 2018b430. |
| #40 | Frames 2-61 of the window: 22 draws per frame on average. |
| #46-47 | game mode 16 at frame 327, map load (area 5 map 5). |
| #54-55 | f330 fail 00100200, cont_word 12, addr0 20189c30. |
| last 2 | watchdog: no progress 10 s, phase 5, steps 332. |

Derived (CONFIRMED by arithmetic): addr0 - cont_word*4 = 0x20189C00 for all 3 jumps. So no split ran before jt_queue in any of
the 3 frames, and 0x20189C00 is the start of the citro3d command buffer. f330 has cont_word 12, so the frame list held 2 words
before the jump writes: the frame had no draw (map load). f270 and f300 held 864-866 and 1536-1538 words: frames with draws.

## 1. The readback address is wrong

- CONFIRMED: signature `Result GSPGPU_ReadHWRegs(u32 regAddr, u32* data, u8 size);` (libctru gspgpu.h:272). The call at
  pm_gpu.c:1180 passes a byte size of 4. The call form is correct.
- CONFIRMED: JT_HW_BASE is 0x400000 (pm_gpu.c:1142). The read goes to regAddr 0x40012C.
- INFERRED (3dbrew, GSPGPU:ReadHWRegs): regAddr is an offset from 0x1EB00000. So the read is physical 0x1EF0012C.
- CONFIRMED: the 0x1EF00000 block holds the external GPU and LCD registers. gspgpu.h:20-22 names "0x1EF00X90", "0x1EF00X70",
  "0x1EF00X78" as LCD framebuffer registers.
- INFERRED (3dbrew GPU/Internal Registers): the PICA200 internal registers (the GPUREG_* indexes) start at 0x1EF01000. GSP
  starts a command list with a write to 0x1EF018F0 = 0x1EF01000 + 0x23C*4 (GPUREG_CMDBUF_JUMP0, registers.h:601), which agrees.
- Result: the test reads an external register at 0x1EF0012C, not GPUREG_FRAGOP_CLIP_DATA3. The value 0x00100200 is the same in
  all 3 reads (log #25, #32, #54), so it is a static configuration register (INFERRED). The "fail" lines give no information
  about the jump.
- Correct regAddr (INFERRED): 0x401000 + 0x4B*4 = 0x40112C. Not checked: whether CLIP_DATA3 reads back on hardware (some
  PICA registers can be write-only).

## 2. The sub-buffer words and the jump fields

- CONFIRMED: the sub words at pm_gpu.c:1156-1159 are [0x4A54534B][0x000F004B][1][0x000F023C]. This is the GPUCMD_Add order
  [param][header] (inv_citro3d_decode.md section 1, from gpu.o disassembly). The sub is flushed (pm_fl_dc, pm_gpu.c:1160).
  GPUCMD_AddRawCommands is not used. The 5 tail writes use GPUCMD_AddWrite (pm_gpu.c:1166-1171), 2 words each.
- CONFIRMED (log): addr1 0x206EB000 and every addr0 are 16-byte aligned physical FCRAM addresses.
- CONFIRMED (disassembly, libcitro3d.a base.o C3Di_SplitFrame 0x1c-0xbc, literals 0x000f0111, 0x000f0110, 0x000f0063): when
  context flag bit 1 (value 2) is set, C3D_FrameEnd adds 3 writes BEFORE GPUCMD_Split: FRAMEBUFFER_FLUSH (0x111),
  FRAMEBUFFER_INVALIDATE (0x110), EARLYDEPTH_CLEAR (0x63). C3D_DrawArrays sets that flag (drawArrays.o 0x108-0x114, `orr r3,r3,#2`).
- CONFIRMED (inv_citro3d_decode.md section 1): GPUCMD_Split then adds FINALIZE (0x010) and repeats it until the length is a
  multiple of 4 words.
- Result, continuation contents (CONFIRMED by the 3 facts above):
  - frame with a draw (f270, f300): FLUSH, INVALIDATE | EARLYDEPTH_CLEAR, FINALIZE = 32 bytes. SIZE0 = 2 (16 bytes) covers only
    FLUSH and INVALIDATE. FINALIZE is outside the jump target.
  - frame without a draw (f330): FINALIZE, FINALIZE = 16 bytes. Only this case matches the layout in agent_jumptest.md.
- The agent_jumptest.md open item "continuation size 16 B depends on C3D_FrameEnd adding no writes" is now answered: it adds
  6 words in every frame with a draw.

## 3. What can block phase 5

- CONFIRMED: phase 5 is set at pm_main.c:462 before pm_gfx_retrace, phase 6 at :464, phase 7 at :466 before pm_gpu_drew and
  pm_gpu_end (:467-472). Names: pm_wdog.c:16-19.
- CONFIRMED: jt_result (gspWaitForP3D, ReadHWRegs) runs inside pm_gpu_end (pm_gpu.c:1843), so in phase 7. It is not the
  hang site. The f330 result line printed, and jt_queue cannot run again until 1 s later (pm_gpu.c:1844). No jump code runs in
  steps 331-332 after that line.
- CONFIRMED: gspWaitForP3D is gspWaitForEvent(P3D, false) (gspgpu.h:186). gspWaitForEvent with nextEvent false waits on a
  sticky LightEvent, then clears it (gspgpu.o gspWaitForEvent 0x18-0x34, LightEvent_Init(...,1) = sticky). It returns at once
  if any P3D came since the last clear. It does not wait for the jump frame. The gx queue does not use this event.
- CONFIRMED: the only GPU wait in phase 5 is the deferred frame begin: pm_gfx_retrace -> first gbi task -> pm_gpu_begin_wait
  -> C3D_FrameBegin (pm_gpu.c:1778). C3D_FrameBegin calls gxCmdQueueWait with timeout -1 when C3D_FRAME_NONBLOCK is clear
  (renderqueue.o 0x20-0x3c). A grep of pm_gpu.c, pm_tex.c, pm_gbi.c, pm_main.c finds no other gspWait, C3D_Sync*,
  svcWaitSynchronization or LightEvent wait. GSPGPU_FlushDataCache does not wait for the GPU.
- CONFIRMED (gxqueue.o gxCmdQueueInterrupt 0x10-0x60): the queue counts each PSC0, PPF, P3D or DMA interrupt as one finished
  job. It does not check the type. One missing interrupt = the wait never ends. One extra interrupt = a job counts as done early.
- INFERRED: step 332 waits forever in C3D_FrameBegin for the queue of step 331 (its command list P3D or its display transfer PPF).
- INFERRED: the hang comes from the jump test. Four other console logs on the same boot route pass the frame 327 map load
  (build3ds/ftp_170, ftp_170b, ftp_170c, ftp_170f: "game mode 16 frame 327", no watchdog line near it).

## 4. Causes, most likely first

1. INFERRED. The jump runs on hardware, and the hang is a lost GPU completion interrupt after the first jump in a frame with no
   draw (f330). f330 is the only one of the 3 frames whose list was 2 words long and whose continuation is FINALIZE x2. The
   exact hardware path (the command processor reads past the 16-byte jump target, or runs the in-place continuation twice, or
   GSP keeps channel state from the jump) is not known. Evidence for "the jump runs": none yet, because the readback is wrong.
2. INFERRED. The jump does not run at all (JUMP1 from inside a list is ignored, or the size or address unit is wrong), and the
   hang has another cause in the jumptest build. Against it: f270 and f300 have a truncated continuation (section 2). If the
   jump had run and the processor had stopped at the end of 16 bytes, step 271 would have hung. So in drawn frames either the
   jump did not run, or the processor did not stop at SIZE0 (it reads on into EARLYDEPTH_CLEAR and FINALIZE, which sit in the
   same place in memory). The log cannot tell these apart.
3. INFERRED, weak. The GSPGPU_ReadHWRegs read of 0x1EF0012C has a side effect (an external status register). Against it: the
   same read at f270 and f300 did not hang.
4. INFERRED, weak. A game-side stall in the map load, not related to the jump. Against it: the 4 logs in the section 3 list.

## Next test (one push, one change set)

Change set for the code agent:
- JT_HW_BASE + 0x4B*4 -> 0x40112C (PICA internal block at 0x1EF01000).
- Add `draws_frame` and `pad` to the `jump f<n>` result line.
- Set PM_WD_PHASE(4) around C3D_FrameBegin in pm_gpu_begin_wait, then put back 5. The watchdog line then names the frame begin
  if that is the hang site.

Lines to read after the push:
- `jump f<n> ok value 4a54534b` in a frame with draws: the jump runs, and the processor does not stop at the end of SIZE0.
  Cause 2 is out. Fix: SIZE0 must cover the real tail (32 bytes in a frame with draws).
- `jump f<n> fail value <x>` with draws, `ok` without draws: the jump runs only in some frames. Compare `pad`.
- `fail` in every frame with a value that is not 00100200: the jump does not run (cause 2), or CLIP_DATA3 does not read back.
- `watchdog: ... phase 4 (gpu begin (frame begin))` directly after a no-draw `ok` line: cause 1 is CONFIRMED.
  Phase 5 again: the hang is not the GPU wait. Then causes 3 and 4 stay open.

## Update: Azahar evidence (coordinator, lane JTA)

- Azahar, same build: with jumptest the game stops at step 1, the watchdog exits, no jump line prints (scenario jump_azahar).
  Without jumptest the same build reaches frame 330 with no watchdog line (jump_off_azahar, commit 848d9ad84).
- The first jump is queued at the first pm_gpu_end (jt_last is 0, pm_gpu.c:1844). The boot frames draw (log #20: 21 draws),
  so the continuation is 32 bytes (section 2) and SIZE0 = 2 covers only FLUSH and INVALIDATE.
- INFERRED (Citra/Azahar command processor): a JUMPn write sets the read pointer to ADDRn and the length to SIZEn, and the
  P3D interrupt comes from the FINALIZE write (register 0x010). The processor stops at the end of the 16-byte target. FINALIZE
  never runs, P3D never comes, the gx queue never counts the list as done (section 3), and the next C3D_FrameBegin
  (pm_gpu.c:1778, gxCmdQueueWait timeout -1) waits forever. The result line cannot print: jt_result runs only after that wait
  (pm_gpu.c:1803 before 1843).
- This moves cause 1 down. New most likely cause (CONFIRMED layout, INFERRED hang path): **SIZE0 = 2 truncates the
  continuation in frames with draws, so FINALIZE is outside the jump target.** The console survived f270 and f300 probably
  because its processor reads past SIZE0 into the in-place EARLYDEPTH_CLEAR and FINALIZE (INFERRED). The console hang after
  f330 stays open.

### Single change to test in Azahar (one start)

pm_gpu.c:1166: `GPUCMD_AddWrite(GPUREG_CMDBUF_SIZE0, drew ? 4 : 2);` (32 bytes when the frame has a draw, 16 bytes when it
has none), and log the same value as size0 in the layout line. Pass: step 1 passes, `jump f<n>` lines print, no watchdog
line. If it still stops at step 1, the size is not the cause and the jump target or JUMP1 position is next. Note: a kick split
clears the citro3d draw flag; if a kick ran and no draw followed, the tail is 16 bytes. The boot frames have under 48 draws,
so no kick runs (pm_gpu.c:1827).

## Reproduce the disassembly

```
MSYS_NO_PATHCONV=1 docker run --rm pm3ds-build:1 sh -c 'B=/opt/devkitpro/devkitARM/bin; cd /tmp; \
 $B/arm-none-eabi-ar x /opt/devkitpro/libctru/lib/libcitro3d.a base.o renderqueue.o drawArrays.o; \
 $B/arm-none-eabi-objdump -dr --section=.text.C3Di_SplitFrame base.o; \
 $B/arm-none-eabi-objdump -dr --section=.text.C3D_DrawArrays drawArrays.o; \
 $B/arm-none-eabi-objdump -dr --section=.text.C3D_FrameBegin renderqueue.o; \
 $B/arm-none-eabi-ar x /opt/devkitpro/libctru/lib/libctru.a gspgpu.o gxqueue.o; \
 $B/arm-none-eabi-objdump -dr --section=.text.gspWaitForEvent gspgpu.o; $B/arm-none-eabi-objdump -dr gxqueue.o'
```
