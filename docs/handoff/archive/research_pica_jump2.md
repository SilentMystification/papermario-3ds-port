# Research 2: PICA200 command-list jump (GPUREG_CMDBUF_JUMP0/1)

Date: 2026-10-31. Scope: code hosts and forums. Pass 1 is in docs/handoff/research_pica_jump.md.

Labels: [SOURCE] = text or code from the named file. [INFERENCE] = my reading. [CLAIM] = a repo comment or a search snippet, not checked.

## Access status

- github.com raw files: read through `gh api` (authenticated). WebFetch on raw.githubusercontent.com worked for devkitPro/libctru gpu.c only.
- libctru.devkitpro.org gpu.h source page: 403 in pass 1. Not used. The repo copy was read instead.
- 3dbrew.org: 403 in pass 1. Not read.
- github.com/citra-emu/citra: "Repository not found" on clone. The upstream repo is no longer served. Azahar (azahar-emu/azahar) is its successor. Its gpu.cpp was read.
- forum.devkitpro.org: no thread found by search. Not fetched directly.
- GitHub code search: rate-limited (HTTP 403) after about 15 queries. The hits below are the set returned before the limit.

## Register definitions

- [SOURCE] devkitPro/libctru, libctru/include/3ds/gpu/registers.h, lines 597-602:
  GPUREG_CMDBUF_SIZE0 0x0238, SIZE1 0x0239, ADDR0 0x023A, ADDR1 0x023B, JUMP0 0x023C, JUMP1 0x023D. Comment on JUMP0/1 is only "Command buffer jump 0/1."
- [SOURCE] lib/libctru/include/3ds/gpu/gpu.h (lines 13-50): gpuCmdBuf, gpuCmdBufSize, gpuCmdBufOffset are host-side globals. GPUCMD_SetBuffer sets them. They do not write any register.

## Source 1: libctru (devkitPro/libctru, shallow clone, commit 1f8d386)

- [SOURCE] libctru/source/gpu/gpu.c, GPUCMD_Split, lines 61-73:
  - line 63: `GPUCMD_AddWrite(GPUREG_FINALIZE, 0x12345678);`
  - line 64-65: if `gpuCmdBufOffset & 3`, a second FINALIZE write. Comment: "16-byte align the buffer".
  - line 67-68: `*addr = gpuCmdBuf; *size = gpuCmdBufOffset;` (size in words).
  - line 70-72: buffer base advances, size reduces, offset resets to 0.
  - Result: GPUCMD_Split emits no jump. It returns a segment to the caller.
- [SOURCE] libctru/source/gpu/gx.c, GX_ProcessCommandList, lines 47-58: sends buf0 address, buf0 size, and flags to the GSP. No GPU register is written here. The GSP does the register writes.
- [SOURCE] No libctru file outside registers.h uses GPUREG_CMDBUF_JUMP0/1 (grep of lib/).

## Source 2: citro3d (devkitPro/citro3d, shallow clone, commit 9f21cf7)

- [SOURCE] c3d/source/base.c, C3Di_SplitFrame, lines 309-328:
  - if draw was used: writes FRAMEBUFFER_FLUSH, FRAMEBUFFER_INVALIDATE, EARLYDEPTH_CLEAR (lines 20-22 of that function).
  - calls GPUCMD_Split (line 25). Computes usage (lines 26-27).
  - No CMDBUF_JUMP write.
- [SOURCE] c3d/source/renderqueue.c:
  - line 175: `GPUCMD_SetBuffer(ctx->cmdBuf, ctx->cmdBufSize, 0);` (frame start).
  - lines 191-194 (C3D_FrameSplit): calls C3Di_SplitFrame, then `GX_ProcessCommandList(cmdBuf, cmdBufSize*4, flags)`.
- [SOURCE] c3d/source/base.c line 84: command buffer size rounded up to 16 bytes.
- [SOURCE] grep of citro3d for JUMP, CMDBUF: only C3D_DEFAULT_CMDBUF_SIZE and C3D_Init. No jump.
- [INFERENCE] citro3d does not use the jump. A frame is a list of segments. Each segment goes to the GSP as its own GX entry.

## Source 3: Azahar (azahar-emu/azahar, successor of Citra), src/video_core/gpu.cpp, commit read 2026-10-31

- [SOURCE] lines 195-205, GSP SubmitCmdList command: `addr[0] = address >> 3; size[0] = params.size >> 3; trigger[0] = 1; SubmitCmdList(0);`
- [SOURCE] lines 396-411, register write handler: a write to `command_buffer.trigger[0]` calls `SubmitCmdList(0)`. A write to `trigger[1]` calls `SubmitCmdList(1)`. Trigger registers are the JUMP registers (0x23C, 0x23D), per src/video_core/pica/regs_internal.cpp lines for 0x23C/0x23D.
- [SOURCE] lines 466-491, SubmitCmdList(index): reads addr and size of that index (GetPhysicalAddress, GetSize). Checks that the memory range is valid. Calls `impl->pica.ProcessCmdList(addr, size, ...)`. Clears trigger[index].
- [INFERENCE] In Azahar, a write to JUMP0 with a value runs ProcessCmdList on the ADDR0/SIZE0 range, from inside the register write of the running list. When that call returns, the outer list continues at its next word. This is a nested call with an implicit return. I did not read ProcessCmdList, so the continuation point is not confirmed.
- [INFERENCE] Azahar treats ADDR and SIZE as 8-byte units (`>> 3`). This agrees with libn3ds and gdx-3ds below. It does not agree with the gbatek note below.
- The Azahar code does not check the jump value. A write of 0 also starts the list (line 407-410 does not test the value). The gbatek text says "writing any value" starts execution.

## Source 4: profi200/libn3ds (bare-metal ARM11 driver), source/arm11/drivers/gfx.c, lines 575-587

- [SOURCE] GX_processCommandList:
  - line 584: `p3d[GPUREG_CMDBUF_SIZE0] = size>>3;`
  - line 585: `p3d[GPUREG_CMDBUF_ADDR0] = (u32)cmdList>>3;`
  - line 586: `p3d[GPUREG_CMDBUF_JUMP0] = 1;`
  - Only channel 0 is used. No JUMP1 is written.
- [SOURCE] line 579-581: acknowledges the P3D interrupt and waits for the IRQ flag to clear before the start.
- [INFERENCE] This is a list start, not an in-list jump. It shows the units: size and address in 8-byte units (`>>3`). Whether it was tested on hardware: not stated in the file.

## Source 5: cruxxxxxx/gdx-3ds, port/3ds/gfx/gdx3ds_stereo_frame.cpp

- [SOURCE] line 505-510: `ch[0]` SIZE0 = bodyWords/2 ("in 8-byte units"). `ch[1]` SIZE1 = 2 (16 bytes). `ch[2]` ADDR0 = CmdBufAddr8(body). `ch[3]` ADDR1 = CmdBufAddr8(gpuCmdBuf + offset + 8). Then `GPUCMD_AddWrite(GPUREG_CMDBUF_JUMP0, 1);` (line 510).
- [SOURCE] lines 440-443: `CmdBufAddr8(p) = osConvertVirtToPhys(p) >> 3` ("physical, in 8-byte units").
- [SOURCE] lines 126-133 (comment): "The GPU runs P, B, jumps to R, runs B again for the right eye, jumps to E and raises one P3D interrupt." The comment also names "the command-buffer channel 1 return address" and "B's JUMP1 ... render-cache flush".
- [CLAIM] This project says the in-list jump returns (channel 1 is a return address). The return is its own design. Its notes say "hardware card-6 A/B". I did not check those notes. Shipping status: not shown in the file. Treat as in development.
- [INFERENCE] This is the only source found that uses a jump in the middle of a list with a return path. It writes JUMP0 and JUMP1 inside the list, and sets ADDR1 to a return point.

## Source 6: RigleGit/gba-ds-3ds-specs, 3ds/gpuinternalregisterscommandlists.md (mirror of gbatek)

- [SOURCE] lines 13-20: ADDR0/1 "Physical address of 1st/2nd command buffer, in 16-byte units".
- [SOURCE] lines 1-6 and 22-30: SIZE0/1 "Size of command buffer, in 16-byte units".
- [SOURCE] lines 41-50: JUMP0/1 "(jump to ADDR0) (W)". "Writing any value starts CMDBUF execution, at ADDR0/1." "Executes the Command List, starting at CMDBUF_ADDR0/1, until reaching CMDBUF_SIZE0/1 (or until FINALIZE+AUTOSTOP)."
- [SOURCE] lines 52-54: "Unknown why there are two command lists ... maybe the second list is automatically started at the end of the other list?"
- [SOURCE] lines 27-29: FINALIZE with AUTOSTOP can end the list early. Padding is FINALIZE.
- [INFERENCE] The gbatek text names no return. It names an end at SIZE or at FINALIZE+AUTOSTOP. The units in gbatek (16 bytes) conflict with the three code sources (8 bytes). Trust the code over the note until a hardware test settles it.

## Source 7: dragonation/minun_mod_resolver, kitties/pkmsm/pica.js (a PICA emulator model, JavaScript)

- [SOURCE] lines 1823-1833: JUMP0 with nonzero parameter sets `commands.current = commands.buffers[0]`. JUMP1 does the same with buffers[1]. No return is stored.
- [INFERENCE] This model switches the capture buffer. It has no return. It is not an execution check of the hardware.

## Other hits (names or comments only, not read)

- azahar-emu/azahar, src/video_core/pica/regs_internal.cpp: register names at 0x238-0x23D. Names only.
- gdkchan/SPICA, SPICA/PICA/PICARegister.cs: names and offsets only.
- whovian64/pocket-native and fearkov/zakuro (engine/gpu/src/registers.rs): comment says writing JUMP0/1 "moves execution to one of them." [CLAIM] Not checked against code. The two repos share the same comment text. Treat with caution.
- RigleGit/gba-ds-3ds-specs: this is the gbatek mirror (Source 6).

## Summary of what the code does with the jump

| Source | Written | Units | Return after jump |
|---|---|---|---|
| libctru GPUCMD_Split | none | words (size) | n/a, no jump |
| citro3d C3Di_SplitFrame | none | words | n/a, no jump |
| libn3ds GX_processCommandList | SIZE0, ADDR0, JUMP0=1 (list start) | 8-byte (`>>3`) | n/a, list start |
| gdx-3ds stereo | SIZE0/1, ADDR0/1, JUMP0 (R), JUMP1 (B) | 8-byte (`>>3`) | [CLAIM] channel 1 is the return |
| Azahar gpu.cpp | JUMP0/1 handler calls SubmitCmdList(index) | 8-byte (`>>3`) | [INFERENCE] nested call, returns |
| gbatek mirror | JUMP0/1 | 16-byte (note) | not stated |
| minun pica.js model | JUMP0/1 switches capture buffer | n/a | none (model) |

## Shipping use

- libctru and citro3d: shipping. They do not write a jump. Neither uses the jump register.
- libn3ds: not shown whether shipping. It writes JUMP0 once as a list start.
- gdx-3ds: not shown whether shipping. In development per its own comments.
- Azahar, SPICA, minun model: emulators and tools, not games.

## Open items for our bug

- The units conflict. Three code sources use 8-byte units. gbatek says 16-byte units. A test on hardware can settle this. A list that is not a multiple of 16 bytes gives a bad end, per the 3dbrew pass 1 snippet.
- No source confirms a return after JUMP0 on hardware. Only gdx-3ds claims it, and Azahar appears to do it as a nested call (inference).
- The pass 1 claim that a finalization write in a non-aligned buffer hangs the GPU is still only a search snippet from 3dbrew. Not read from the page.
