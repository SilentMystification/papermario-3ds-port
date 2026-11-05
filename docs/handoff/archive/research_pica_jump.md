# Research: PICA200 command-buffer jump (CMDBUF_* registers, jump command)

Date of search: 2026-10-31. Scope: web and docs only. No source found names CMDBUF_SIZE, CMDBUF_ADDR or CMDBUF_JUMP. No source found a working jump example in citro3d or libctru.

## Access status

- 3dbrew.org GPU/Internal_Registers: HTTP 403 on every fetch. Not read.
- 3dbrew.org GPU/Commands: HTTP 403. Not read. Only search snippets were seen.
- libctru gpu.h source page (libctru.devkitpro.org/gpu_8h_source.html): HTTP 403. Not read.
- deepwiki libctru 5.1 GPU command management: fetched, no jump text.
- zitrus-docs PICA200 page: fetched, no jump text.
- Citra commit 579298862 (git.openpunk.com): HTTP 502. Not read.
- yuzu commit 8852fc6a: TLS certificate error. Not read. Snippet only.

## Sources

1. Citra commit 579298862ebf3a6749a9d284317fd42ab72d8db0 (git.openpunk.com, date not shown in result)
   - Stated (search snippet): an early commit added command list size, address and process registers. The process register write logged the size and address, with a TODO to process the list.
   - Stated (search snippet): a later change added a command buffer struct with two channels. Each channel has a size field and a trigger register. Trigger writes at offsets 0x23c and 0x23d set the list length from the buffer size.
   - Stated (search snippet): a code comment says games can set these registers to give a command-list subroutine mechanism.
   - Inference: this comment is the only source that names a subroutine mechanism. The jump and return logic was not read.
   - Working example or hang: none reported.

2. yuzu commit 8852fc6a87eaf8edbb26e93d44c1596f0167e3ac, "Merge pull request #822 from bunnei/pica-improvements" (git.morozoff.pro). Not fetched.
   - Stated (search snippet): the same two-channel description as source 1.
   - Working example or hang: none reported.

3. 3dbrew GPU/Commands, search snippet only (www.3dbrew.org, page 403 on fetch)
   - Stated (search snippet): "Trigger Command List Processing" converts an address to a physical address and writes the physical address and size to GPU registers at 0x1EF018E0.
   - Stated (search snippet): the command buffer size must be 16-byte aligned. A finalization write to register 0x0010 that sits in the last 8 bytes of a non-aligned buffer does not run, and the GPU hangs.
   - Stated (search snippet): command 0x0010 with value 0x12345678 must be the last command.
   - Working example or hang: the alignment pitfall is a reported hang cause. No jump is named.

4. citro3d issue #35, https://github.com/devkitPro/citro3d/issues/35 (opened 2017-07-13)
   - Stated: a render target used with no draw in the frame softlocks the program, after two cycles, on Citra and on hardware.
   - Stated: fincs called this API misuse. C3D_FrameSplit() is needed before an external operation on a framebuffer that already has drawing in the frame.
   - Stated: Swiftloke mentions APT_DoApplicationJump. The thread does not discuss command-buffer continuation.
   - Working example or hang: a hang. Fix was to clear the framebuffer inside the frame, or draw a solid quad.

5. Luma3DS issue #1559, https://github.com/LumaTeam/Luma3DS/issues/1559 (opened 2020-12-20, closed 2025-04-08)
   - Stated: after New Super Mario Bros 2 or Castlevania: Mirror of Fate, homebrew freezes on launch. A power cycle is needed.
   - Stated: a maintainer says "this seems to be a libctru/citro3d-related GPU crash issue," possibly caused by the game.
   - Stated: the maintainer says the issue is fixed in the latest libctru. Homebrew must be rebuilt. Current hbmenu avoids it.
   - Stated: a workaround is to open and close a Nintendo system app before launching homebrew.
   - Working example or hang: a hang that persists across boots. Fix is libctru update and rebuild.

## Findings

- No source names the register offsets for a jump or a CMDBUF_JUMP. The only offset-level data is the Citra trigger offsets 0x23c and 0x23d, and the 3dbrew address 0x1EF018E0. The 3dbrew values are from a search snippet. They are not confirmed from the page.
- Stated: no source reports a jump that returns to the caller. Inference: the Citra "subroutine" comment implies a call-and-return, but the code was not read.
- Stated: no source reports a stop at the first jump after a map load, or a stop after 300 frames.
- Inference: the closest match to our symptom is the 3dbrew note that a missing finalization write (0x0010) in a non-16-byte-aligned buffer hangs the GPU. Our buffer end must be checked for that.
- Inference: the citro3d issue #35 pattern (no draw, then stall) and the Luma issue #1559 pattern (GPU left in a bad state) show that a bad command list can hang the GPU. Neither names a jump.

## Next steps

- Read the Citra source for ProcessCommandList and its head and current pointer, from a mirror that serves the file.
- Read libctru gpu.h and citro3d source from a mirror, since the official host returns 403.
- Check the 16-byte alignment and the 0x0010 finalization write at the end of the jumped buffer.
