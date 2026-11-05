# inv_citro3d_format: GPU command list format (citro3d / libctru)

## Result

The command header, the add functions and the uniform helpers are on disk as headers only.
The bodies of GPUCMD_AddRawCommands, GPUCMD_Add, GPUCMD_Split and the uniform flush are not on disk.
No .c source for libctru or citro3d exists on this machine.
The jump-and-return command word is not defined in any header. Do not assume a format for it.

## Where the source is

| Location | Contents | Status |
|---|---|---|
| repo `3ds/include/`, `3ds/ld/` | port headers only, no libctru/citro3d | no GPU command definitions |
| repo grep (GPUCMD, C3D_, citro3d, gpu.h, GX_) | port users only: 3ds/src/pm_gpu.c, pm_gbi.c, pm_tex.c, pm_nusys.c, pm_phase.c, pm_main.c, pm_depth.c, pm_comb.c; 3ds/include/pm_port.h, pm_tex.h | no definitions |
| docker `pm3ds-build:1` (devkitpro/devkitarm base) | `/opt/devkitpro/libctru/include/3ds/gpu/gpu.h`, `.../gpu/registers.h`, `.../gpu/gx.h`, `/opt/devkitpro/libctru/include/c3d/*.h`, `/opt/devkitpro/libctru/include/citro3d.h` | headers present, read here |
| docker `/opt/devkitpro/libctru/lib/libcitro3d.a`, `libcitro3dd.a` | compiled objects | not readable as source |
| docker `/opt/devkitpro/citro3d` | absent | none |
| docker pacman `citro3d-1.7.1-2/files` | lists only the c3d/*.h headers, no .c | confirms no source |
| `C:\Users\Andrew\AppData\Local\Temp\citro3d_src` | empty folder | none |
| `C:\devkitPro` | absent | none |
| `C:\Users\Andrew\..` | no citro3d or libctru copy | none |

## Command header (gpu.h)

File: `/opt/devkitpro/libctru/include/3ds/gpu/gpu.h`

- line 11, `GPUCMD_HEADER(incremental, mask, reg)`: `(incremental << 31) | ((mask & 0xF) << 16) | (reg & 0x3FF)`.
  - bit 31: incremental flag (1 means the register address increments per parameter).
  - bits 19..16: 4-bit write mask (0xF means all bytes).
  - bits 9..0: register offset (word index, 10 bits).
- line 57, `GPUCMD_AddRawCommands(const u32* cmd, u32 size)`: declaration only.
- line 65, `GPUCMD_Add(u32 header, const u32* param, u32 paramlength)`: declaration only.
- line 72, `GPUCMD_Split(u32** addr, u32* size)`: declaration only. This is the split point. It returns the current buffer address and size.
- lines 23, 34, 45: `GPUCMD_SetBuffer`, `GPUCMD_SetBufferOffset`, `GPUCMD_GetBuffer` (inline, defined here).
- line 103, `GPUCMD_AddSingleParam(header, param)`: calls `GPUCMD_Add(header, &param, 1)`.

Write macros (gpu.h):

- line 109, `GPUCMD_AddMaskedWrite(reg, mask, val)`: header `GPUCMD_HEADER(0, mask, reg)`, one parameter.
- line 111, `GPUCMD_AddWrite(reg, val)`: masked write with mask 0xF.
- line 113, `GPUCMD_AddMaskedWrites(reg, mask, vals, num)`: header with incremental 0, `num` parameters.
- line 115, `GPUCMD_AddWrites(reg, vals, num)`: mask 0xF.
- line 117, `GPUCMD_AddMaskedIncrementalWrites(reg, mask, vals, num)`: header with incremental 1.
- line 119, `GPUCMD_AddIncrementalWrites(reg, vals, num)`: mask 0xF, incremental 1.

Note on the write headers: the header word is the first word of the command. The parameter words follow it. The function bodies that pad the list to an even word count are not on disk.

## Uniform write

- The CPU-side uniform staging is in `/opt/devkitpro/libctru/include/c3d/uniforms.h`.
  - line 4-5: `C3D_FVUNIF_COUNT 96`, `C3D_IVUNIF_COUNT 4`.
  - line 7-9: `C3D_FVUnif[2][96]`, `C3D_IVUnif[2][4]`, `C3D_BoolUnifs[2]`.
  - line 11-13: dirty flags.
  - line 15, `C3D_FVUnifWritePtr(type, id, size)`: marks dirty and returns the pointer.
  - line 30, `C3D_FVUnifMtxNx4`: writes a matrix as N rows of 4 floats.
  - line 38, `C3D_FVUnifMtx4x4`; line 43, `C3D_FVUnifMtx3x4`.
- The GPU registers for float uniforms are in `/opt/devkitpro/libctru/include/3ds/gpu/registers.h`:
  - line 720, `GPUREG_VSH_FLOATUNIFORM_CONFIG 0x02C0` (vertex shader).
  - line 721, `GPUREG_VSH_FLOATUNIFORM_DATA 0x02C1`.
  - line 689, `GPUREG_GSH_FLOATUNIFORM_CONFIG 0x0290`; line 690, `GPUREG_GSH_FLOATUNIFORM_DATA 0x0291`.
- The exact command sequence that flushes these to the GPU (which header value, which incremental flag, which index word) is in the libcitro3d body. It is not on disk. Not stated here.

## Jump and return

- Registers in `/opt/devkitpro/libctru/include/3ds/gpu/registers.h`:
  - line 597-598, `GPUREG_CMDBUF_SIZE0 0x0238`, `GPUREG_CMDBUF_SIZE1 0x0239`.
  - line 599-600, `GPUREG_CMDBUF_ADDR0 0x023A`, `GPUREG_CMDBUF_ADDR1 0x023B`.
  - line 601-602, `GPUREG_CMDBUF_JUMP0 0x023C`, `GPUREG_CMDBUF_JUMP1 0x023D`.
- The jump command word, the return encoding and the value written to JUMP0/1 are not defined in any header.
- `GPUCMD_Split` (gpu.h line 72) is the only split API. Its body is not on disk.

## Not on disk

- Bodies of GPUCMD_AddRawCommands, GPUCMD_Add, GPUCMD_Split (libctru gpu.c).
- Uniform flush sequence (citro3d source).
- Jump-and-return command word and encoding.
- Any .c file for libctru or citro3d. The .a files are present but are not source.

## Next step

To get the jump format, a source copy of libctru (gpu.c) or citro3d (renderqueue.c, uniforms flush) is needed. The user must supply it or allow a download from the devkitPro package source. No download was done in this investigation.
