# ndref: reference draw stream (format v4)

Test oracle for the native draw system (docs/handoff/plan_native_system.md 9.2 and 9.3). It is not read by the game.
Pinned commit: e95b21495 (NDREF1 hook on the walker of 620d457f7; the walker code is the same, the hook is off unless `ndref=<frame>` is set). The files were captured on build 2020 (commit 42f6fac7e). The S0-6 commit adds counters and a prof line only; it does not change the draws.
Written by pm_gpu.c (ndref_begin, ndref_draw, ndref_end, ndref_tag) and pm_gbi.c (pm_gbi_ndref_frame, prod and mdl tags).
Read by 3ds/tools/nd_ref_dump.py. All integers and floats are little-endian. Floats are IEEE 754 single.

## File

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | magic `NDR1` (bytes 4E 44 52 31) |
| 4 | 4 | version = 4 (v1 had no kind 4 records, v2 no kind 5, v3 no kind 6) |
| 8 | 4 | frame number (pm_frame_count) |
| 12 | 4 | draw record count |
| 16 | 4 | payload bytes (everything after this 20-byte header) |
| 20 | n | records, in the order the walker drew them |

## Record

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | kind: 1 draw, 2 producer tag, 3 model tag, 4 draw state, 5 palette source, 6 command words |
| 4 | 4 | size of the body in bytes |
| 8 | size | body |

Tag body (kinds 2 and 3, size 8): u32 id, u32 extra. Kind 2: id = producer id (pm_gbi_prod_mark), extra = 0. Kind 3: id = model id (pm_gbi_mdl_mark), extra = pan flag.

## State body (kind 4, S1-3 oracle)

One record per GPU draw call, written right before C3D_DrawArrays or C3D_DrawElements, after the kind 1 record of the same flush. Size 248 bytes = 62 x u32.

| Words | Field |
|---|---|
| 0 | pass: 0 main draw, 1 decal pass A, 2 decal pass B (final draw of a decal2 batch), 3 zmark pass 2 |
| 1 to 7 | z, zmark, zcmp, zupd, nocol, noise (a TEXTURE2 noise stage is bound), raw (graw) |
| 8 to 11 | plan key w0, w1, two, t1only: the inputs of pm_comb_build for the plan. 0 when combine is not 8 |
| 12 to 14 | omode_h, omode_l, depth image flag: the inputs of the last sync_render (pm_gbi.c), which gives zcmp, zupd, blend, atest, decal, nocol, memblend (nd_render_state). Zero before the first sync_render |
| 15 to 27 | citro3d context bytes 216 to 267, 13 words, the effect block that C3Di_EffectBind turns into register writes (libcitro3d.a disassembly). Index i of the 13: 0 reg 0x100 (colour operation), 1 reg 0x130, 2 reg 0x4D (depth map scale, f24), 3 reg 0x4E (depth map offset, f24), 4 bytes: cull (reg 0x40, low 2 bits), depth map enable (reg 0x6D), early depth enable (regs 0x62 and 0x118), early depth func (reg 0x61), 5 reg 0x6A (early depth ref), 6 reg 0x104 (alpha test), 7 reg 0x105 (stencil test), 8 reg 0x106 (stencil op), 9 reg 0x107 (depth and colour mask; reg 0x126 is derived from it), 10 reg 0x103 (blend colour), 11 reg 0x101 (blend func), 12 reg 0x102 (logic op, low byte) |
| 28 to 61 | citro3d context bytes 292 to 427 (34 words): TEV stage i at words 5 i to 5 i + 4 = regs 0xC0 + 8 i (0xF0 + 8 (i - 4) for i 4 and 5) +0..+4 (source, operand, combiner, colour, scale); block word 30 = reg 0xE0 (buffer update), 31 = reg 0xFD (buffer colour), 32 = reg 0xFE |

## Palette source body (kind 5, S1-4 oracle)

Written right after the kind 1 record of a raw draw with a palette (np > 0). Size 68 + 68 x np bytes. Body: 16 floats `post` (gv_post, C3D_Mtx memory order), u32 np, then per entry u32 gm (game matrix index, the same order as the kind 1 palette) and 16 floats of `pal_tab[gm]` (the game matrix, row-vector 4 x 4 as the walker stores it). The kind 1 palette holds pcomp[gm] = Mtx_Multiply(post, game) in C3D_Mtx order (pm_gpu.c pal_mul), which nd_mtx_pcomp must reproduce bit for bit.

## Command words body (kind 6, S3 oracle)

Written right after each GPU draw call (after its kind 4 record). Body: u32 n, then n u32 words: the words the citro3d command buffer got since the end of the draw call before (the direct GPUCMD writes before the call, the uniform, texture and state updates that C3D_DrawElements or C3D_DrawArrays flushes, and the draw trigger). n is 0 when the buffer was split in between. The words are the PICA command stream format: parameter word, header word (register in bits 0 to 15, byte mask 16 to 19, extra count 20 to 27, bit 31 consecutive writes).

## Draw body

| Offset | Size | Field |
|---|---|---|
| 0 | 76 | TevKey, 19 x u32 (pm_gpu.c): combine, tex, tex1, i8_0, i8_1, rgb_scale, r_mix, draw_3d, rect_2d, dview, off_active, off_i8, r_memblend, r_replace, r_blend, prim, cconst, mixcol, plan_gen |
| 76 | 84 | NdrefDraw: i32 z, graw, prog; u32 cull; i32 decal, blend, atest, sx0, sy0, sx1, sy1; i32 sa, sb (slots of unit 0 and 1); u32 tw0[4] and tw1[4] (pm_tex_slot_words: dim, param, lodParam, phys >> 3; zeros if the slot is not live) |
| 160 | 56 | uniforms: float uvso[4], cmul[4], cadd[4], zmix[2] |
| 216 | 304 | PmFx: 76 floats (lflag, amb, col[7], dir[7], lk0, lk1, fogu) |
| 520 | 4 | u32 np: palette entries (0 for a CPU draw) |
| 524 | np x 68 | per entry: u32 game matrix index gm, then the 16 floats of pcomp[gm] (post x game, C3D_Mtx) |
| after | 4 | u32 nv: vertices in draw order (3 per triangle) |
| after | nv x 16 | raw draw (graw = 1): ring vertex values, gring + 16 x gidx[gi_start + i] (x y z w as s16 and flags; byte 6 holds the palette index k) |
| after | nv x 40 | CPU draw (graw = 0): Vert, floats x y z w u v r g b a |

Draw order is the order of flush_draw closes. Triangles are consecutive vertex triples (GPU_TRIANGLES).

## Frame rules

- A frame is written only when `ndref=<frame>` is set and pm_frame_count equals that frame. Stereo frames hold both eyes in order (one buffer per frame).
- The hook is cold. With the switch off, no record is written and no buffer is allocated.
- Buffer cap: 2 MiB. An over-cap frame is written with the `over` flag in its log line and must not be used as reference.

## Log line

`ndref f<frame> done draws <n> bytes <payload>`. The line prints after the file is closed.

## Scenes (S0-1, S0-2)

| File | Scenario | Frame | Draws | Census batches (same run) |
|---|---|---|---|---|
| ndref_yard.bin | ndref_yard | 200 (kmr_20 entry 4) | 46 | 46 |
| ndref_pause.bin | ndref_pause | 250 (pause menu open) | 96 | 96 |
| ndref_battle.bin | ndref_battle | 180 (battle, kmr_03) | 73 | 73 |
| ndref_text.bin | ndref_text | 178 (pause menu text) | 96 | 96 |
| ndref_title.bin | ndref_title | 800 (boot, game mode 16; not the title, mode 2 not reached) | 70 | 70 |
| ndref_splash.bin | ndref_splash | 412 (fxtest 89 water splash, effect start at 400) | 141 | 141 |
| ndref_stereo.bin | ndref_stereo | 200 (yard, wide and stereo, both eyes) | 88 | 88 |
| ndref_toad.bin | ndref_toad | 200 (mac_01 entry 0) | 64 (810 triangles) | 64 (census of S1-1 run) |

All eight scenes are present.
