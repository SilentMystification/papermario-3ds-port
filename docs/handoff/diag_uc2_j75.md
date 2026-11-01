# diag_uc2_j75: where frame word j=75 (0x00000110) comes from

Scope: read only. 3ds/src read. libcitro3d disassembly (scratchpad `c3d_dis.txt`, from the pm3ds-build:1 image). Linker map `build3ds/lane-UC2/pm_3ds.map` read. No Azahar start. map_restore_table.c not read.
Tags: CONFIRMED (source, map or disassembly line read) or INFERRED.
Acronyms: UC2 = reccheck epic. ctx = citro3d context (`__C3D_Context`). PICA = the GPU command set. GPUCMD = libctru command list API. RC1 = record capture. j = word index in the group.

## 0. Short answer
- CONFIRMED: frame word j=75 is not a copy of record data. No port code writes it. citro3d writes it live. `C3Di_EffectBind` copies ctx+0xF4 into the command buffer at each flush.
- CONFIRMED: the value 0x110 is not any value the port passes to `C3D_StencilTest`.
- INFERRED: ctx+0xF4 held 0x110 in memory at that flush. The writer is not found. Candidates are in section 5.

## 1. Facts from the log (CONFIRMED by log, from agent_uc2_ctx.md)
- ctx+0xF4 equals the last `stc_set` word at sites 1 to 3 (frame 35).
- UC2D line not yet logged. Record a[75] = 0x00000000. Frame b[75] = 0x00000110.

## 2. Trace of the word (write by write)
| # | Where | What | Status |
|---|---|---|---|
| W1 | pm_gpu.c 834, 839, 844, 855, 1023, 1033 (`stc_set`, body at pm_gpu.c 109 to 111) | Port calls `C3D_StencilTest`. Only port writer of the stencil state. | CONFIRMED |
| W2 | libcitro3d effect.o, `C3D_StencilTest`, dump line 1488 `str r3,[ip,#244]` (ip = `__C3D_Context`, reloc at dump 1491) | Stores the word at ctx+0xF4. Word = `im<<24 \| ref<<16 \| wm<<8 \| (fn<<4)&0x70 \| en` (dump 1474 to 1488). Sets dirty bit 0x10 at ctx+0x20 (dump 1483, 1484). | CONFIRMED |
| W3 | libcitro3d base.o, `C3Di_UpdateContext` dump 0x5c `tst r3,#16`, branch to 0x34c. 0x34c `bic r3,#16`. 0x350 `ldr r0` literal 0xD8 + `__C3D_Context` (dump 0x4bc). 0x358 `bl C3Di_EffectBind`. | At the dirty bit, calls `C3Di_EffectBind(ctx+0xD8)`. | CONFIRMED |
| W4 | libcitro3d effect.o, `C3Di_EffectBind` dump 0x50 to 0x58: `add r1,r4,#24` (= ctx+0xD8+0x18 = ctx+0xF0), `mov r2,#4`, `ldr r0` literal 0x800f0104, `bl GPUCMD_Add` | Sends 4 words from ctx+0xF0 to ctx+0xFC as one incremental group. Header 0x800f0104 gets count 3 from GPUCMD_Add, giving 0x803f0104. | CONFIRMED |
| W5 | libctru GPUCMD_Add (copy into the command buffer) | Copies the 4 words at call time. | INFERRED (libctru semantics, not read) |
| W6 | pm_gpu.c 2289 to 2297 `pm_gpu_rec_end` | Returns `w = rb_buf + rb_off` (pointer into the live command buffer). No copy, no write. | CONFIRMED |
| W7 | pm_gbi.c 3465 `memcpy(ra + at, w, n*4)` in `rec_done` | Record copy at capture. Record a = copy of the words of W4 at capture time. | CONFIRMED |
| W8 | pm_gbi.c 3438, 3440 (`rec_check`, `uc2d_first`) | Reads record a and live b. The compare writes only counters and `uc2d_msk`. | CONFIRMED |
| W9 | pm_gpu.c 2307 to 2323 (`pm_gpu_rec_walk`), 2327 to 2350 (`pm_gpu_rec_same`) | Writes only `rec_pt`, `rec_pa`, `rec_msk` and counters. No write to b. | CONFIRMED |
| W10 | pm_gbi.c 3438, 3440, 3460 to 3465 | Only reads of `ra + k->off`. No replay code copies the record into the command buffer. | CONFIRMED |
| W11 | pm_gpu.c 534 to 537, 1192 to 1198 | Port `GPUCMD_AddWrite` and `AddIncrementalWrites` calls. The 1192 to 1198 calls are at frame end (jump table). Not inside the 0x104 group. | INFERRED (position, not compared word by word) |

No patch path and no index offset exists between W4 and the compare. The record copy (W7) and the frame words (W4) use the same walker layout.

## 3. Index check (CONFIRMED)
- UC2D data (agent_uc2_ctx.md): `cmd 72`, hdr `803f0104`, first diff j=75.
- Walker (pm_gpu.c 2307 to 2323, pm_gbi.c 3418 to 3429): p0 at word 72, header at word 73, count 3, data words 74, 75, 76, 77.
- j=75 is data word 1 (`doff 1`). Register = 0x104 + 1 = 0x105 (stencil). Matches W2.
- j=73 and j=74 are equal in a and b. The first diff is at j=75, so the frame header at 73 is 803f0104 too. No layout shift exists for this group.

## 4. Why 0x110 is not a port value (CONFIRMED unless marked)
- Decode of 0x00000110: im 0x00, ref 0x00, wm 0x01, fn bits 0x10 (ALWAYS), en 0.
- Port words (pm_gpu.c `stc_set` formula): site 1 0xFF01FF11, site 2 0xFF01FF21, site 3 0xFF000010, sites 4 to 6 the same set. All port calls use im 0xFF or the disabled form with wm 0. No call gives wm 1 with im 0. CONFIRMED.
- Only libcitro3d.a and libcitro3dd.a reference `C3D_StencilTest` under /opt/devkitpro (grep of all .a files in pm3ds-build:1). No other library calls it. CONFIRMED.
- Literal-offset scan of the whole dump for a store at ctx+0xF4 (`#244`, or effect `#28` from the effect base): one hit, W2. Other `#28` stores are light (C3D_LightInit dump 2916, C3D_LightEnvInit dump 3569) or render target (C3D_RenderTargetSetOutput dump 5413, base r4). CONFIRMED for literal offsets. INFERRED for full coverage: stores through a computed pointer were not excluded.

## 5. Where the 0x110 memory value could come from (INFERRED, not settled)
- (a) Port out-of-bounds write into the .bss that holds `__C3D_Context`. Map `build3ds/lane-UC2/pm_3ds.map`: `__C3D_Context` at 0x014D8A08, size 0x2D0 (base.o). ctx+0xF4 = 0x014D8AFC. Lower neighbours: `hookCookie` 0x014D89F8 (libcitro3d base.o), `starpoint_lights2` 0x014D89E0 (src/starpoint.o), `level_up_lights` 0x014D89B0 (src/level_up.o). An overflow from a lower object must cross about 0x11C bytes. Plausible only for a large copy. Not checked.
- (b) citro3d store through a computed pointer (not in the literal scan). Low likelihood.
- (c) The record was captured before any stencil call. Then a[75] = 0 is the zeroed context (ctx zeroed by C3D_Init, INFERRED). Every later frame with a nonzero stencil word differs at j=75. This explains why j=75 differs. It does not explain the value 0x110. INFERRED.

## 6. One log-only probe (proposed, not run)
Binary: current UC2 lane build (already has `stc_set` ctx read and `pm_gpu_stc_ctx_log("uc2d_first")`). No code change.
- Scenario: `yard_stc`, row UC2. Add `reccheck` to the debug string. Extend the until-line so it passes the first `UC2D first` event (frame of the first differing record).
- Quantity: ctx+0xF4 read at `uc2d_first`, plus the UC2D line (b[75]).
- Reading:
  - `UC2P uc2d_first ctxF4 00000110`: memory held 0x110 at the flush (W4 reads the same value). The writer is outside the port stencil calls. Next probe: read ctx+0xF4 at `rec_begin` and after each C3D draw in the window, to find the first change (one quantity per shot).
  - `UC2P uc2d_first ctxF4 ff000010` with `UC2D ... b 00000110`: the chain W4 to W7 is wrong. Next: read the command buffer word at the group index directly, at `rec_end`, before the compare.
- Budget: UC2 has 1 start left (agent_uc2_ctx.md). Use it only with the justification line in 3ds/logs/perf/status_UC2.txt.
- Check without a start: grep the UC2 log for the first `stc` line and the first `rec` capture frame. If the first stencil write comes after the first capture, section 5(c) holds for j=75.

## 7. Minor
- pm_gbi.c 3429 comment and label `ni` still say "non-incremental". Bit 31 is incremental (diag_uc2_0x110.md section 0). Log text only.
