# diag_uc2_0x110: what writes 0x110 at yard_rec record word j=75

Investigation only. Read only. No source edit, no Azahar start, no map_restore_table.c access. Tags: CONFIRMED (code, header or disassembly read) or INFERRED.

## 0. Corrections to the earlier notes (CONFIRMED)
1. Bit 31 of a PICA header is the INCREMENTAL flag, not the non-incremental flag.
   - Source: libctru header `/opt/devkitpro/libctru/include/3ds/gpu/gpu.h` line 11 `GPUCMD_HEADER(incremental, mask, reg) = (incremental<<31) | ...`. Line 119: `GPUCMD_AddIncrementalWrites` uses `GPUCMD_HEADER(1, ...)`. Line 115: `GPUCMD_AddWrites` uses `GPUCMD_HEADER(0, ...)`.
   - The same header is noted in docs/handoff/inv_citro3d_format.md line 28.
   - The port labels `ni = (hd >> 31)` as "non-incremental" in pm_gbi.c 3428 and in the log line UC2D (pm_gbi.c 3429). The label is wrong. `ni 1` means incremental.
2. So the UC2E conclusion ("every data word writes the header register") is wrong for this group. Word j=75 is data word 1 of an incremental write. It writes register `reg + doff`.

## 1. Which register word j=75 writes (CONFIRMED)
- Log line (agent_uc2.md): `cmd 72`, `hdr 803f0104`, `doff 1`, `role data`.
- Header decode: incremental 1, mask 0xF, count `(hdr >> 20) & 0xFF` = 3, so 4 data words (words 74 to 77). Register 0x104.
- Data word k writes register 0x104 + k:
  - word 74 writes 0x104 (GPUREG_FRAGOP_ALPHA_TEST)
  - word 75 writes 0x105 (GPUREG_STENCIL_TEST)
  - word 76 writes 0x106 (GPUREG_STENCIL_OP)
  - word 77 writes 0x107 (GPUREG_DEPTH_COLOR_MASK)
- Register names: `/opt/devkitpro/libctru/include/3ds/gpu/registers.h` lines 281 to 284.
- The first differing word is 75. So word 74 (alpha test, register 0x104) is equal in record and frame. This explains why the alpha gate (rat 0, bad 4800 unchanged) did not change the count. Alpha state is not the cause of j=75.
- Value at register 0x105: record a[75] = 0x00000000, frame b[75] = 0x00000110. Mapping a = record, b = frame (pm_gpu.c 2310 comment, CONFIRMED).

## 2. Who writes the four words (CONFIRMED for the block, INFERRED for the flush trigger)
- The four words are the citro3d fragment-op block in the context struct. Disassembly of `libcitro3d.a` (in the pm3ds-build:1 image):
  - C3D_AlphaTest stores ctx+0xF0 (word for 0x104).
  - C3D_StencilTest stores ctx+0xF4 (word for 0x105).
  - C3D_StencilOp stores ctx+0xF8 (word for 0x106).
  - C3D_DepthTest stores ctx+0xFC (word for 0x107).
  - Each of these sets bit 4 (0x10) of the flags word at ctx+0x20 (`orr #16`).
  - `C3Di_UpdateContext` is the function that flushes the dirty state (INFERRED from the dirty bit and the offsets; the flush body was not decoded).
- Port callers of the stencil word (pm_gpu.c, CONFIRMED):
  - 811 `C3D_StencilTest(true, GPU_ALWAYS, 1, 0xff, 0xff)` (decal2 pass 1)
  - 816 `C3D_StencilTest(true, GPU_EQUAL, 1, 0xff, 0xff)`
  - 821 `C3D_StencilTest(false, GPU_ALWAYS, 0, 0xff, 0)` (decal2 end)
  - 832 `C3D_StencilTest(true, GPU_EQUAL, 1, 0xff, 0xff)`
  - 1000 `C3D_StencilTest(true, GPU_ALWAYS, 1, 0xff, 0xff)`
  - 1010 `C3D_StencilTest(false, GPU_ALWAYS, 0, 0xff, 0)`
- Stencil is NOT cached in the port (no `sh.` entry for stencil, unlike alpha at pm_gpu.c 492 and depth at 484). Every call reaches citro3d. So the value at ctx+0xF4 is the last stencil call before the flush.

## 3. Encoding of the stencil word (CONFIRMED from disassembly of C3D_StencilTest)
- Arguments: r0 enable, r1 function, r2 ref, r3 inputMask, stack writeMask.
- Word = `inputMask << 24 | ref << 16 | writeMask << 8 | (function << 4) & 0x70 | enable`.
- Decode of the port calls:
  - enabled ALWAYS ref 1, masks 0xff: 0xFF01FF11
  - enabled EQUAL ref 1, masks 0xff: 0xFF01FF21
  - disabled (false, ALWAYS, 0, 0xff, 0): 0xFF000010
- Frame value 0x00000110 decodes to enable 0, function ALWAYS, ref 0, inputMask 0, writeMask 1.
- Record value 0x00000000 decodes to all fields zero.
- CONFIRMED: no port call in pm_gpu.c produces 0x00000110 or 0x00000000 under this encoding. Every port call has inputMask 0xff.

## 4. Why the record copy does not see it (INFERRED)
- The record copies the words written between rec_begin and rec_end (pm_gpu.c 2260 to 2274). The block value comes from the citro3d context, not from the port state that the record key holds.
- RecKey (pm_gbi.c 3352) holds ver, gen, hgen, tgen0, tgen1, s0, s1, state, nb. It has no stencil field. A stencil change between capture and frame does not cause a recapture. CONFIRMED for the fields. INFERRED for the effect.
- Record 0 fits a capture before any stencil call (zeroed context). Frame 0x110 fits a state left by some other stencil write. The source of that write is not decoded.

## 5. Open point (not settled)
- No port call gives 0x110. Either the block is not ctx+0xF0..0xFC as assumed, or a stencil value is changed between `C3D_StencilTest` and the flush. The disassembly of the flush (`C3Di_UpdateContext`) was not read.

## 6. What would settle it (no run started)
1. Disassemble `C3Di_UpdateContext` in libcitro3d.a (same docker command as section 2). Check which ctx words it writes to 0x104 to 0x107, and whether it masks or merges the stencil word. Read only.
2. Log-only probe (one Azahar start, row UC2): print the args of each `C3D_StencilTest` call in pm_gpu.c (811, 816, 821, 832, 1000, 1010) with a counter, and the ctx+0xF4 value read before the group flush. Pick the window at the uc2d line 3429 event (`UC2D first`). If the logged args never produce 0x110, the block layout assumption is wrong.
3. Correct the label at pm_gbi.c 3428 and 3429 (`ni` means incremental). Log-only, no behavior change.

## 7. Acronyms
UC2 = reccheck epic. RC1 = record capture. RecKey = record key struct. ctx = citro3d context (`__C3D_Context`). PICA = the 3DS GPU command set (PICA200).
