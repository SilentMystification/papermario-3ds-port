# ndref: reference draw stream (format v1)

Test oracle for the native draw system (docs/handoff/plan_native_system.md 9.2 and 9.3). It is not read by the game.
Pinned commit: e95b21495 (NDREF1 hook on the walker of 620d457f7; the walker code is the same, the hook is off unless `ndref=<frame>` is set). The files were captured on build 2020 (commit 42f6fac7e). The S0-6 commit adds counters and a prof line only; it does not change the draws.
Written by pm_gpu.c (ndref_begin, ndref_draw, ndref_end, ndref_tag) and pm_gbi.c (pm_gbi_ndref_frame, prod and mdl tags).
Read by 3ds/tools/nd_ref_dump.py. All integers and floats are little-endian. Floats are IEEE 754 single.

## File

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | magic `NDR1` (bytes 4E 44 52 31) |
| 4 | 4 | version = 1 |
| 8 | 4 | frame number (pm_frame_count) |
| 12 | 4 | draw record count |
| 16 | 4 | payload bytes (everything after this 20-byte header) |
| 20 | n | records, in the order the walker drew them |

## Record

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | kind: 1 draw, 2 producer tag, 3 model tag |
| 4 | 4 | size of the body in bytes |
| 8 | size | body |

Tag body (kinds 2 and 3, size 8): u32 id, u32 extra. Kind 2: id = producer id (pm_gbi_prod_mark), extra = 0. Kind 3: id = model id (pm_gbi_mdl_mark), extra = pan flag.

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
| ndref_stereo.bin | ndref_stereo | 200 (yard, wide and stereo) | not run | not run |
| ndref_toad.bin | ndref_toad | 200 (mac_01 entry 0) | not run (file overwritten) | not run |

ndref_stereo and ndref_toad are missing. Each needs one Azahar start (see the status file).
