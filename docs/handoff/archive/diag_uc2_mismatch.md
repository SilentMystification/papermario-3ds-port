# diag_uc2_mismatch: which record word differs from the full walk (UC2 reccheck)

Investigation only. Base HEAD 271b205f4. No source edit, no Azahar start, no git change. Read: agent_uc2.md, plan_uniform_cut.md, uc2_change.patch (scratchpad), 3ds/src/pm_gpu.c, 3ds/src/pm_gbi.c, the citro3d `drawElements.o` in image pm3ds-build:1 (read-only objdump), lane logs build3ds/lane-UC2/last_log.txt.
Tags: [C] CONFIRMED (read from code, disassembly or log). [I] INFERRED.

## 0. Result

- The first unmasked word that differs is the parameter word of each index-buffer address write: the word before header 0x000F0227 in the group. [C] for the register and the code path. [I] for the frame and draw in the logs.
- The address is `(phys(gidx + gi_start) - ctx base) | (type << 31)`. It changes each frame. [C]
- Patch table: no entry covers it. Mask: no entry covers it. The compare treats it as a plain word, so it counts as `bad`. [C]
- Single change: in `pm_gpu_rec_walk`, record the offset of each 0x227 param word, and in `pm_gpu_rec_same` set that word in the mask. Expected: yard `bad` 0 per window. `hit` and `miss` unchanged. Pixels unchanged (the mask does not touch the draw).

## 1. The index-buffer write sequence (citro3d drawElements.o)

Disassembly of `C3D_DrawElements` (scratchpad drawel_dis.txt). Each `GPUCMD_Add` writes `[param0][header]` for one register. [C]

| Disasm | Header | Register | Param | Note |
|---|---|---|---|---|
| 0x60 | 0x0002025E | 0x25E | r6 or 0x300 | primitive mode |
| 0x74 | 0x000F025F | 0x25F | 1 | |
| 0x90 | 0x000F0227 | 0x227 | `r4 \| r8 << 31` | index buffer config. r4 = phys(indices) - ctx[+0x40] (0x3C to 0x7C) |
| 0xA4 | 0x000F0228 | 0x228 | r7 = count | NUMVERTICES |
| 0xB8 | 0x000F022A | 0x22A | 0 | vertex offset |
| 0xD4 | 0x00010245 | 0x245 | 0 | |
| 0xE8 | 0x000F022F | 0x22F | 1 | draw trigger |

The order of the first six rows is the order in the group. [C] The param of 0x227 is written by `orr r2, r4, r8, lsl #31` at 0x78. [C]

Header-to-word order (GPUCMD_Add, per inv_citro3d_decode.md section 1): buf[off+0] = param0, buf[off+1] = header. [C from the disasm, the note in that doc]. So for a 0x227 command with one param, the group holds `[addr word][0x000F0227]`. The address word is at the header index minus 1. [C layout, I for the exact group index: it depends on the group prefix.]

## 2. The address changes per frame

- `C3D_DrawElements(..., gidx + gi_start)` at pm_gpu.c 1054. [C]
- `gi_start = gidx_n` at pm_gpu.c 1062 (batch close), and at 860, 866, 873 (empty closes). [C]
- `gidx_n = 0` at each frame start, pm_gpu.c 1764. [C]
- So `gi_start` is the count of indices already queued in the frame. It differs between frames unless the frame order is the same. [C for the code, I for the value at each frame.]
- `pm_gpu_rec_begin` (pm_gpu.c 2260) calls `flush()`, which closes the open batch. Then `pm_gpu_rec_end` (2266) calls `flush()` again. So one group holds one batch, and `gi_start` of a group is the frame position, not a group constant. [C]

## 3. The walker and the compare do not cover that word

- `pm_gpu_rec_walk` (pm_gpu.c 2284 to 2300) fills the patch table only for header 0x2C1 (uniform data). Header 0x2C0 sets `cur`. Every other register is counted in `pk` (0x280 to 0x2DF) or skipped. No entry for 0x227. [C]
- In the UC2 patch (uc2_change.patch lines 97 to 103), `pm_gpu_rec_same` masks only `rec_pa[e]` entries: split (off, off+2, off+3, off+4) or (off .. off+3). The 0x227 address word has no entry, so `a[j] != b[j]` counts it as `bad`. [C]
- The length check (patch line 96): when `na != nb`, `bad` is counted once and no word is compared. [C]

## 4. Counter evidence

Lane log build3ds/lane-UC2/last_log.txt, the `rec f` lines (the window counters reset at each report, pm_gbi.c rec_report). [C for the values]

| Line | Frame | hit | bad | miss | Note |
|---|---|---|---|---|---|
| #127 | f348 | 2375 | 4800 | 0 | includes the capture window (cap 48, build 9) |
| #137 | f648 | 2430 | 4560 | 0 | |
| #142 | f948 | 2400 | 4500 | 0 | steady: 8 compares per frame, 15 bad words per frame |

- 2400 hits and 4500 bad in 300 frames. The average is 1.875 bad words per compare. [C arithmetic]
- A model with N draws has N 0x227 words. If the address is the only differing word, bad per compare = draws per record. An average of 1.875 is a plausible draw count for these records. [I]
- Count (0x228) is key-pure for a group, because the group is one batch (section 2). It is not expected to differ. [I from section 2]

## 5. Mac miss

- mac01t_rec (agent_uc2.md): miss 300 in the 300-frame window, 0 on yard. That is about one miss per frame. [C from agent_uc2.md]
- A patch-table difference means a different count or start index of uniform entries (pm_gpu.c 2289 to 2296). The forced shape (UC1) fixes the count for the same inputs. So the cause is a per-draw input that is not in the key: pal_n, use5 (lights, lflag) or tex1. [I]
- The patch table compare does not depend on the 0x227 mask. The fix in section 0 leaves miss at 300. [C from the code]

## 6. Single change and expected counters

Change (one row, about 12 lines, pm_gpu.c only):
1. `pm_gpu_rec_walk`: for header reg 0x227 without incremental bit, store the offset `i` (the address word) in a new table `rec_ix[]`, with a count. Same walk, same check of the end.
2. `pm_gpu_rec_same`: after the patch mask loop, set `rec_msk[rec_ix[k]] = 1` for each entry of the record walk.

Do not use `a[j+1] == 0x000F0227` as the test. A data word can equal that value. [I]

Expected result on the same windows (same scenes, same build):
- yard_rec: bad 0 per window, hit 2400, miss 0. [I]
- mac01t_rec: bad falls by the number of 0x227 words. miss stays at 300. [I]
- Picture: unchanged (the mask only affects the compare). [C from the code: pm_gbi.c rec_check does not write the draw]

If yard bad stays above 0 after the change, the next candidate is the cull word (0x40, g_cullm from gv_wind, pm_gpu.c 2100 to 2104). [I, plan_uniform_cut u6]. Name the register and the group index with a fresh investigation before a code agent.

## 7. What this does not show

- The frame and the draw of the first differing compare. The logs give window sums only. The first compare is the first frame after a build with the same key (the build at rec_done, pm_gbi.c 3410 in HEAD). [I]
- The group index of the first 0x227 word. It depends on the group prefix (the state writes before the draw). A dump of one record would name it. The dump is an Azahar or console step: not run here.
- Replay (UC3) must patch the address word with `phys(gidx + gi_start_now) - base` per draw. The masked check does not prove the patch. [C from plan_uniform_cut 2d]
