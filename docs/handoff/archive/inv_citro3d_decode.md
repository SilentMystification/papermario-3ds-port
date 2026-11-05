# inv_citro3d_decode: PICA command words from libctru and citro3d

Source: docker image `pm3ds-build:1` (3ds/build.sh line 33). Read only.
Objects: `/opt/devkitpro/libctru/lib/libctru.a(gpu.o)`, `/opt/devkitpro/libctru/lib/libcitro3d.a(uniforms.o)`.
Tool: `arm-none-eabi-objdump -dr`. Addresses are offsets in the function section (`-ffunction-sections`, each function starts at 0).
Headers: `3ds/gpu/gpu.h` line 11 (GPUCMD_HEADER), `3ds/gpu/registers.h` lines 597-602 (CMDBUF), 689-690 (GSH float), 704-708 (VSH bool/int), 720-721 (VSH float).

Reproduce:
```
MSYS_NO_PATHCONV=1 docker run --rm pm3ds-build:1 sh -c 'B=/opt/devkitpro/devkitARM/bin; cd /tmp; \
 $B/arm-none-eabi-ar x /opt/devkitpro/libctru/lib/libctru.a gpu.o; $B/arm-none-eabi-objdump -dr gpu.o; \
 $B/arm-none-eabi-ar x /opt/devkitpro/libctru/lib/libcitro3d.a uniforms.o; $B/arm-none-eabi-objdump -dr uniforms.o'
```

## 1. Register write (GPUCMD_Add, gpu.o)

Header word (gpu.h line 11 plus GPUCMD_Add at 0xc0-0xc8):

| Bits  | Field | Source | State |
|-------|-------|--------|-------|
| 0-15  | register index (macro masks to 0x3FF) | gpu.h:11 | CONFIRMED |
| 16-19 | byte-enable mask (0xF = all 4 bytes) | gpu.h:11, 0xF0000 or-ed in uniforms.o | CONFIRMED |
| 20-27 | extra param count = n-1 | `c0: sub r8,r6,#1` / `c8: orr ip,r9,r8,lsl #20` | INFERRED (disasm only, not in gpu.h) |
| 31    | incremental (1 = reg+1 per param) | gpu.h:11 | CONFIRMED |

Word order in the buffer (GPUCMD_Add 0xd0-0xe4, 0x28-0x64):
```
d0: ldr r1,[r7]            ; param0
d8: str r1,[r2,r0,lsl #2]  ; buf[off+0] = param0
e4: str ip,[r2,r1,lsl #2]  ; buf[off+1] = header | (n-1)<<20
3c: bl memcpy              ; buf[off+2..] = param1..param(n-1)
40: tst r8,#1 / 64: strne r3(=0),[r0,r1,lsl #2]  ; pad word 0 when n-1 is odd
```
So a command is `[param0][header][param1..n-1][0 pad if needed]`, always an even word count (8-byte aligned). CONFIRMED by disasm. Param-first order is INFERRED as the hardware order (no header states it).

Chunking (0x84-0x8c, 0x68-0x70): n is split into chunks of at most 256 params (`cmp r5,#256`). When bit 31 is set (`cmp r9,#0 / addlt r9,r9,r6`), the next chunk header register is advanced by the chunk size. Non-incremental chunks keep the same register. INFERRED.
Bound check (0x98-0xb4): `off+1+n > gpuCmdBufSize` calls svcBreak. INFERRED.

GPUCMD_AddRawCommands (0x00-0x50): memcpy of `size` words to `buf+off`, `off += size`. No header, no padding, no bound check. CONFIRMED (gpu.h:57 signature, disasm).

GPUCMD_Split (0x18-0x34, 0x80-0x98, literal 0xa4 = `0x000f0010`, 0x9c = `0x12345678`):
- writes FINALIZE: param `0x12345678`, header `0x000F0010` (reg 0x0010, mask 0xF, single).
- repeats that write while `off & 3` (`tst r3,#3`) so the chunk length is a multiple of 4 words (16 bytes).
- returns `*addr = gpuCmdBuf`, `*size = off` (words), then `gpuCmdBuf += off*4`, `gpuCmdBufSize -= off`, `off = 0`. INFERRED (FINALIZE name is not in registers.h lines read; reg 0x0010 value is from the literal).

## 2. Uniform write block (C3D_UpdateUniforms, uniforms.o)

Shader select (0x10-0x18): `r8 = 0` for VSH, `r8 = -48` for GSH (`mvneq r8,#47`). 0x2C0-0x30 = 0x290 = GSH_FLOATUNIFORM_CONFIG. CONFIRMED (registers.h 689, 720).

Float uniforms, per run of dirty vec4 [start, end) (0x40-0xe8):
```
40: add sl,r8,#0x2c0           ; reg = 0x2C0 (VSH) / 0x290 (GSH)
54: orr r3,sl,#0xf0000         ; header0 = 0x000F02C0
b4: orr r2,r5,#0x80000000      ; param  = 0x80000000 | start
cc: bl GPUCMD_Add (n=1)        ; -> [0x800000ss][0x000F02C0]
d8: add r0,sl,#1 / dc: orr r0,r0,#0xf0000   ; header1 = 0x000F02C1, non-incremental
e0: lsl r2,r7,#2               ; n = (end-start)*4 words
e4: add r1,r3,fp,lsl #4        ; params = &C3D_FVUnif[type][start] (16 B per vec4)
e8: bl GPUCMD_Add
```
| Word | Field | State |
|------|-------|-------|
| CONFIG header 0x000F02C0 | reg 0x2C0, mask 0xF, single | CONFIRMED |
| CONFIG param bit 31 = 1 | 32-bit float mode (f32 x4 per vec4) | INFERRED |
| CONFIG param bits 0-7 = start index | | INFERRED |
| DATA header 0x000F02C1, bit 31 = 0 | reg 0x2C1, non-incremental, n-1 in 20-27 | CONFIRMED reg/mask, INFERRED count |
| DATA params | 4 f32 words per vec4, order as stored in C3D_FVec (w,z,y,x memory order of the struct) | INFERRED |
Runs above 64 vec4 (256 words) split into 256-word chunks, all to 0x2C1. Pad word 0 when n-1 is odd (n is a multiple of 4, so n-1 is always odd: one pad word per chunk).

Shader-binary constants (0x1b4-0x204, C3Di_ShaderFVecData):
header `0x800F02C0` (`1c8: orr fp,r0,#0x80000000`, `1d0: orr fp,fp,#0xf0000`), n = 4 (`1d4: mov r2,#4`), params = 16-byte entry from the shader DVLE: config word then 3 data words (24-bit float packing). Incremental: 0x2C0, 0x2C1, 0x2C2, 0x2C3. INFERRED.

Int uniforms (0x128-0x198): loop reg 0x2B1..0x2B4 (`158: add r4,r0,#1` with r0 = 0x2B0+r8, end `r9 = 0x2B5+r8`), header `reg|0xF0000`, one param = C3D_IVUnif word. CONFIRMED regs (registers.h 705-708).

Bool uniforms (0x22c-0x250): header `(0x2B0+r8)|0xF0000`, param `0x7FFF0000 | C3D_BoolUnifs[type]` (`240`, `244`). CONFIRMED reg (registers.h 704). Upper 16 bits 0x7FFF INFERRED.

## 3. Jump to a sub-buffer and the return

NOT FOUND in disassembly. No function in libctru.a, libcitro3d.a or libcitro2d.a writes registers 0x238-0x23D (objdump grep for those immediates and literals: only false hits on pc-relative offsets). No source or header under /opt/devkitpro uses CMDBUF_SIZE/ADDR/JUMP outside registers.h. The repo 3ds/ tree has 0 uses.

What the headers give (CONFIRMED): channel 0 = SIZE0 0x0238, ADDR0 0x023A, JUMP0 0x023C. Channel 1 = SIZE1 0x0239, ADDR1 0x023B, JUMP1 0x023D. The header word for each write is the section 1 layout, e.g. `GPUCMD_HEADER(0,0xF,0x23C) = 0x000F023C`.

INFERRED from the general PICA documentation (3dbrew), not from these binaries:
- SIZEn param = sub-buffer length in bytes >> 3. ADDRn param = physical address >> 3.
- A write of any value to JUMPn makes the command processor fetch from ADDRn for SIZEn. It must be the last command of the current buffer chunk (the processor does not return to words after it).
- There is no hardware return. The sub-buffer ends with its own SIZE/ADDR/JUMP writes that point back to the caller's continuation (or the caller pre-loads channel 1 with the return target and the sub-buffer ends with a JUMP1 write).
- Every buffer reached by a jump must be 16-byte aligned in length, which is what GPUCMD_Split padding enforces (section 1). Sub-buffers must be in linear memory and flushed before the GPU reads them.
- A suggested sequence (one command per write, section 1 layout):
  `[size>>3][0x000F0238] [pa>>3][0x000F023A] [any][0x000F023C]` at the end of the caller chunk.
  Do not use one incremental write for 0x238..0x23C: it also writes the channel 1 registers 0x239 and 0x23B between them. Use 3 single writes.
None of these values is CONFIRMED by disassembly. Verify on hardware before use.
