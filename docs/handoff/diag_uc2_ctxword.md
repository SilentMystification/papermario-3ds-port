# diag_uc2_ctxword: what writes the 0x105 word (ctx+0xF4) at the flush

Investigation only. Read only. No source edit, no Azahar start, no map_restore_table.c access.
Tags: CONFIRMED (disassembly or source line read) or INFERRED.
Disassembly: `arm-none-eabi-objdump -dr /opt/devkitpro/libctru/lib/libcitro3d.a` inside image pm3ds-build:1. Output kept in the scratchpad (not committed).

## 1. Flush path (CONFIRMED)
- `C3Di_UpdateContext` (libcitro3d.a, text section of the file, offset 0x464 in the dump) tests the dirty bit `0x10` at ctx+0x20 (`tst r3,#16`, dump line 489 to 490). When set, it clears the bit and calls `C3Di_EffectBind` with `r0 = &__C3D_Context + 0xD8` (literal at dump 0x4bc, relocation `__C3D_Context`).
- `C3Di_EffectBind` (effect.o) takes `effect = ctx+0xD8`. It sends one incremental group with `GPUCMD_Add(0x800f0104, effect+0x18, 4)` (dump 0x17c and 0x5c). The header becomes `0x803f0104` after GPUCMD_Add sets the count. This matches the log line `hdr 803f0104`.
- So the four data words are ctx+0xF0, ctx+0xF4, ctx+0xF8, ctx+0xFC. Word 74 is register 0x104 (alpha), word 75 is register 0x105 (stencil, ctx+0xF4). The data is read from memory at the flush, not copied at call time.

## 2. Writers of ctx+0xF4 in libcitro3d.a (CONFIRMED)
- A search of all `str ... #244` and `0xf4` stores in the dump finds one writer: `C3D_StencilTest` (dump 0x34: `str r3,[ip,#244]`).
- The other fragment-op setters write other words: `C3D_AlphaTest` writes ctx+0xF0 (dump 0x20), `C3D_StencilOp` writes ctx+0xF8 (dump 0x18), `C3D_DepthTest` writes ctx+0xFC (dump 0x20). Each sets dirty bit `0x10` (`orr #16`) at ctx+0x20.
- No generic copy or `memset` into the context was found in the search. C3D_Init was not decoded in full (INFERRED: it zeroes the context, so an initial value of 0 is expected).

## 3. Encoding of C3D_StencilTest (CONFIRMED, dump 0x0 to 0x38)
- Word = `im<<24 | ref<<16 | wm<<8 | (fn<<4)&0x70 | en`. Same as section 3 of diag_uc2_0x110.md.
- Value 0x00000110 = en 0, fn 1 (GPU_ALWAYS), ref 0, im 0, wm 1.

## 4. Port calls (CONFIRMED, pm_gpu.c)
- All C3D_StencilTest calls go through `stc_set` (pm_gpu.c 105 to 107). Calls: sites 1 (821 area, decal pass 1), 2, 3 (end of decal2), 4 (pass 2, B3b), 5 (B3b pass 1), 6 (end, line 1023).
- Input mask is 0xff in every call except site 3 and site 6, where the call is `false, GPU_ALWAYS, 0, 0xff, 0`.
- Port words: 0xFF01FF11, 0xFF01FF21, 0xFF000010. Each has im 0xff or the disabled case with wm 0. No port call gives writeMask 1 (0x100), so no port call gives 0x00000110.
- The port has no access to `__C3D_Context` (grep of `3ds/` finds no reference). The context struct is not in the libctru headers (only effect.h prototypes).

## 5. Conclusion (INFERRED from 1 to 4)
- If the flush reads ctx+0xF4 as the last value written by C3D_StencilTest, the frame word 0x110 cannot come from any port call. So one of three is true:
  a. ctx+0xF4 holds 0x110 in memory at the flush. Then a writer outside citro3d and outside the port's stencil calls changed it. Candidates: a stray store through a pointer (memory overrun into the context), or the context at another address than the one the port reads.
  b. ctx+0xF4 is correct at the flush, but the frame word is changed after `GPUCMD_Add`. Candidates: a patch of the command buffer by port code (the record or replay code in pm_gpu.c), or a word index offset of one (the header or param word is at j=73 or j=74, so j=75 may not be the stencil word).
  c. The flush does not happen at the draw the probe reads. Then the word is from an older flush.
- Which of a, b or c holds is NOT settled by the disassembly.

## 6. Proposed probe (log only, one Azahar start, row UC2)
- Declare `extern unsigned char __C3D_Context[];` in pm_gpu.c (read only, no write). At the `UC2D first` window (pm_gbi.c 3429) log, in one line:
  - `*(u32*)(__C3D_Context + 0xF4)` (the value in memory at the flush),
  - the value last passed to `stc_set` (the port word),
  - the group index j of the header that the record stores (to test the index offset in case b).
- Reading the result:
  - memory shows 0x110: case a. Next probe: log ctx+0xF4 after each `stc_set` and after each C3D_* call in the frame, to find the first draw where the word changes.
  - memory shows the port word, frame shows 0x110: case b. Check the patch code in pm_gpu.c and the index of j=75.
- Log only. No behavior change. Use the existing `stprobe` switch and add the one read.

## 7. Labels (CONFIRMED)
- `ni` in pm_gbi.c 3428 and the UC2D log line 3429 means incremental (bit 31 = 1). The label "non-incremental" is wrong. Same finding as section 0 of diag_uc2_0x110.md.

## 8. Acronyms
UC2 = reccheck epic. ctx = citro3d context (`__C3D_Context`). PICA = the 3DS GPU command set. GPUCMD = libctru command-list API. dump = the objdump listing of libcitro3d.a.
