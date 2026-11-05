# diag_readback_byte: jumptest readback 0x0054534b, written 0x4A54534B

Console fact: build 1633, git a5b984422, line `jump f0 fail value 0054534b`. The low 3 bytes match. The top byte reads 0x00.

## Most likely cause
CLIP_DATA3 (0x4B) is a 24-bit float register. The PICA stores bits 0-23 only. Bits 24-31 read as 0.
So the readback is correct, and the sub-buffer write ran. The jump into the sub reached CLIP_DATA3 (INFERRED from the low 24 bits).

## Claims

1. Register width (INFERRED, strong).
   - libctru `3ds/gpu/registers.h:85-88` names CLIP_DATA0..3 with "Unknown". No width in the header.
   - Our own clip code writes 24-bit floats to the same registers: `3ds/src/pm_gpu.c:466-469` (`f32tof24`) and `pm_gpu.c:511` (`GPUCMD_AddIncrementalWrites(GPUREG_FRAGOP_CLIP_DATA0, gp2_clip_dat, 4)`). CONFIRMED that the code does this.
   - libctru `3ds/gpu/gpu.h:89-93`: `f32tof24` "Converts a 32-bit float to a 24-bit float". CONFIRMED (header in image pm3ds-build:1).
   - The PICA documentation (3dbrew GPU/Internal Registers) lists FRAGOP_CLIP_DATA0..3 as float24 coefficients. Not in the repo. INFERRED from that outside source.
   - The console value 0x0054534B = 0x4A54534B & 0x00FFFFFF. CONFIRMED (arithmetic).
2. GSPGPU_ReadHWRegs order and length (CONFIRMED not the cause).
   - Prototype `3ds/services/gspgpu.h:272`: `Result GSPGPU_ReadHWRegs(u32 regAddr, u32* data, u8 size)`. Size is in bytes.
   - Call `3ds/src/pm_gpu.c:1181`: regAddr `0x401000 + 0x4B*4 = 0x40112C`, destination `&v` (one u32), size 4. One word, correct length.
   - The 3 low bytes are in the written order (4B 53 54). A byte swap or an offset error would move them. CONFIRMED by the value.
3. Command word drops the top byte (CONFIRMED not the cause).
   - `pm_gpu.c:1156-1157`: param 0x4A54534B, header `0xF0000 | 0x4B` = 0x000F004B. Mask bits 16-19 = 0xF = all 4 bytes enabled.
   - `gpu.h:11` `GPUCMD_HEADER` puts the mask at bits 16-19. `inv_citro3d_decode.md:22` CONFIRMED (0xF = all 4 bytes). The param word is passed whole (`inv_citro3d_decode.md:28-34`).
   - The command does not drop the byte. The register does.

## The one test
Change `JT_MAGIC` (`pm_gpu.c:1141`) from `0x4A54534Bu` to `0x004A5453u`. Nothing else.
- Expected readback: `jump f0 ok value 004a5453`.
- The value differs in all 3 low bytes from the old shot (0054534b), so a stale register value cannot give a pass.
- `0x003F0000` (f32tof24(1.f), the per-frame gp2 clip write, `pm_gpu.c:469`) means the sub did not run in that frame.
- Any other value: the register is not 24 bits wide, and claim 1 is wrong.
