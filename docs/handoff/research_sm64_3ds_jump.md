# Research: PICA command-list jump in Epic0522/Super-Mario-64-3ds-port---Ultimate

Source: shallow clone of https://github.com/Epic0522/Super-Mario-64-3ds-port---Ultimate at commit ded2ad283290436687bc81cc4b52c9833e7b09c3 (2026-08-20), branch main. Clone path (scratch, not committed): scratchpad/sm64_3ds_ultimate.

GitHub code search returned HTTP 403 (rate limit) for the terms GPUCMD, GPUREG, C3D_CmdBuf, cmdbuf, CmdBuf, C3D_CmdBufUseTemp, shbin, GX_, GSPGPU_, DMA. Those terms were checked with local grep on the clone instead.

## Verdict

NO. The repo does not use the PICA command-list jump. No hit for CMDBUF_SIZE, CMDBUF_ADDR, CMDBUF_JUMP, GPUREG_CMDBUF_*, GPUCMD_*, GPUCMD_Split, GPUCMD_AddRawCommands, or a raw jump write. The hits for 0x23C and 0x23D are animation and collision data, not GPU registers. (Read from source: grep over the whole clone, excluding anims, assets, dxsdk, data.)

## GPU API used instead (read from source)

Only the citro3d high-level API is used. Frame and command setup:

- src/pc/gfx/gfx_3ds.c:103 `C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);` (read from source). The repo passes the citro3d default size. It does not set a custom buffer.
- src/pc/gfx/gfx_citro3d.c:1255 `C3D_FrameBegin(C3D_FRAME_SYNCDRAW);` (read from source)
- src/pc/gfx/gfx_citro3d.c:1317 `C3D_FrameEnd(0); // Swap is handled automatically within this function` (read from source)
- src/pc/gfx/gfx_3ds.c:433 comment: `// Citro3D handles swapping automatically in C3D_FrameEnd()` (read from source)

Draw and state calls (read from source, counts from grep): C3D_DrawArrays (13 uses, e.g. gfx_citro3d.c:1046, gfx_3ds_minimap.c:218), C3D_BindProgram (gfx_citro3d.c:750, 1129, 1151, 1212), C3D_FrameDrawOn (gfx_citro3d.c:772, 832, 847, 1180; gfx_3ds_menu.c:318), C3D_RenderTargetCreate (gfx_3ds.c:120, 126, 152), C3D_RenderTargetSetOutput (gfx_3ds.c), C3D_TexEnv*, C3D_DepthTest, C3D_AlphaTest, C3D_AlphaBlend, C3D_StencilTest, C3D_FragOpMode, C3D_SetViewport, C3D_SetScissor, C3D_FrameRate. GX_TRANSFER_* flags are used only as transfer flags for the display copy (gfx_3ds.c:111-154, gfx_3ds.h:46-50).

Build link (read from source): Makefile:510 `-lcitro3d -lctru`.

## Inference

- The repo relies on the citro3d default command buffer. It does not build or patch its own PICA command list. This is an inference from the absence of any custom buffer code, not from a file that states it.
- The grep found no custom GPU register write. A write that uses a computed address or a macro name not in the search list would not appear. This is a limit of the search, not a proven absence.
