# VRAM map (inv_vram_map, 2026-11-03)

Question: what is in the 6144 KiB of VRAM, other than the texture cache, and what takes the room.

Source: HEAD c1fada795 (branch native). Run log: build 1884-native git c1fada795, one Azahar start, scenario `yard`, row VMAP (justification in 3ds/logs/perf/status_VMAP.txt). Log file: build3ds/lane-native/last_log.txt. The picture was not read. The subject is the boot and map log lines.

Units: KiB = 1024 bytes. The PICA has two banks of 3072 KiB: A at 0x1F000000, B at 0x1F300000 (pm_mem.c:176-177).

## 1. Correction to the earlier figure

The earlier note wrote the targets as 1536 KB. That is decimal: 4 x 384000 bytes = 1536000 bytes. In KiB the four target buffers are 1500 KiB. The gap of 36 KiB is bank rounding waste (section 2, rows 13 and 14). The measured cap of 1856 KiB is correct.

Check: 1500 + 192 + 2560 + 1856 + 36 = 6144.

## 2. VRAM blocks at boot (all allocated once at boot, none freed)

| # | Item | KiB | Bytes | Bank | Allocated | Freed by | Source |
|---|------|----:|------:|------|-----------|----------|--------|
| 1 | Top colour, 240 x 400 RGBA8 | 375 | 384000 | A | boot, pm_gpu_init | never | pm_gpu.c:1280, 1285; log line 25 |
| 2 | Top depth/stencil, 240 x 400 D24S8 | 375 | 384000 | B | boot | never | pm_gpu.c:1280, 1286; log line 25 |
| 3 | Right eye colour, 240 x 400 RGBA8 | 375 | 384000 | B (inferred) | boot, always, even in mono | never | pm_gpu.c:1295, 1297 |
| 4 | Right eye depth, 240 x 400 D24S8 | 375 | 384000 | A (inferred) | boot, always | never | pm_gpu.c:1295, 1298 |
| 5 | Small arena, 12 blocks of 64 x 64 RGBA8 | 192 | 196608 | 6 in A, 6 in B | boot (small_arena_init) | never; handed out once each | pm_gpu.c:339-349, 392-397 |
| 6 | Copy block, 256 x 512 RGBA8 (grab and full-image snap) | 512 | 524288 | A | boot (full_reserve) | never | pm_gpu.c:353-364, 1305; log line 26 |
| 7 | Full image 0, 512 x 256 RGBA8 | 512 | 524288 | A | boot | never | pm_gpu.c:365-369; log line 27 |
| 8 | Full image 1, 512 x 256 RGBA8 | 512 | 524288 | A | boot | never | log line 28 |
| 9 | Full image 2, 512 x 256 RGBA8 | 512 | 524288 | A | boot | never | log line 29 |
| 10 | Full image 3, 512 x 256 RGBA8 | 512 | 524288 | B | boot | never | log line 30 |
| 11 | Texture cache block A | 160 | 163840 | A, at 0x1F2D3800 | boot (slab_init) | block never; textures sub-allocate | pm_tex.c:463-482; log line 35 |
| 12 | Texture cache block B | 1696 | 1736704 | B, at 0x1F453800 | boot (slab_init) | block never; textures sub-allocate | pm_tex.c:463-482; log line 36 |
| 13 | Waste at top of A | 18 | 18432 | A | none | none | 0x1F2FB800 to 0x1F300000 |
| 14 | Waste at top of B | 18 | 18432 | B | none | none | 0x1F5FB800 to 0x1F600000; slab steps of 32 KiB, pm_tex.c:468 |

Bank totals:
- A: 375 (1) + 375 (4) + 96 (5) + 512 (6) + 1536 (7-9) + 160 (11) + 18 (13) = 3072.
- B: 375 (2) + 375 (3) + 96 (5) + 512 (10) + 1696 (12) + 18 (14) = 3072.

The log line at map time (line 127) reads A used 3054 KiB and B used 3054 KiB, with 0 unaccounted. Line 128 reads 36 KiB free. This matches the table.

Group totals: targets 1500 KiB (rows 1 to 4). Small arena 192 KiB (row 5). Copy and full images 2560 KiB (rows 6 to 10). Texture cache 1856 KiB (rows 11 and 12). Waste 36 KiB (rows 13 and 14).

Rows 3 and 4 split: the log does not print the right-eye bank. The cache start addresses fix the bank totals only. Either buffer can sit in either bank, as long as one is in A and one is in B.

## 3. What is not in VRAM (linear RAM, or CPU RAM)

| Item | KiB | Memory | Source |
|------|----:|--------|--------|
| citro3d command buffer (C3D_Init 0x180000) | 1536 | linear | pm_gpu.c:1270 |
| Raw vertex ring gring, 16384 + 16384 vertices x 16 B | 512 | linear | pm_gpu.c:115, 1326 |
| Raw index ring gidx, 65536 x 2 B | 128 | linear | pm_gpu.c:1327 |
| Vertex buffer vert_base, 32768 x 40 B (Vert = 10 floats) | 1280 | linear | pm_gpu.c:30-37, 1370 |
| Noise texture, 64 x 64 L8 | 4 | linear data (per the comment at pm_gpu.c:372; citro3d source not on disk, not checked) | pm_gpu.c:85-87 |
| Texture upload stage (vram_stage), one per upload, freed two frames later | varies | linear | pm_tex.c:638-641 |
| Pause side arrays pm_wide_side and pm_wide_side_pause, 2 x 40 x 240 x 2 B each | 37.5 each | static CPU RAM | pm_gpu.c:1615-1616 |

The bottom screen has no VRAM target. The bottom screen is the libctru console (consoleInit in pm_gpu_init). The map line shows 0 unaccounted VRAM, so no other VRAM use exists.

The wide mode has no 800 x 240 target. pm_wide only changes the projection and the stretch (pm_gpu.c:211-212, 1267).

## 4. Runtime allocation after boot

- No `vramMemAlignAt`, `C3D_RenderTargetCreate`, or `C3D_TexInitVRAM` call runs after boot. The grep covered 3ds/src and 3ds/include.
- The only per-frame VRAM use is texture slots inside the cache (`slab_alloc` in pm_tex.c, freed on eviction).
- An off-screen pass uses a boot block: full images use full_blk (pm_gpu.c:383-390), 64 x 64 images use the arena (pm_gpu.c:392-397). A pass creates no VRAM block.
- The pause snapshot uses copy_blk for full images (pm_gpu.c:414) and an arena block for 64-wide images (pm_gpu.c:405).
- The grab uses copy_blk (grab_tex.data = copy_blk, pm_gpu.c:1771-1779).
- The debug switches vramfill and vramfilla allocate at boot and never free. Their default is 0 (pm_tex.c:483-495).

## 5. Boot line not to use

The line `mem boot vram free 0KB` (log line 21) reads 0 KiB before the first VRAM allocation (line 25). Use the map line (line 128, 36 KiB free) for the total.

## 6. Oversized or avoidable items

| ID | Item | Now KiB | Option | Saved KiB | Risk |
|----|------|--------:|--------|----------:|------|
| R1 | Right-eye target, when the 3D slider is down at boot | 750 | Do not create it at boot when the slider is at 0. Cache grows from 1856 to 2606 KiB (+40%). | 750 | Stereo cannot start after boot, because the cache takes all free VRAM. A reboot is needed. Change in pm_gpu_init. Medium. |
| R2 | Right-eye depth, a second D24S8 buffer | 375 | Point the right target at the top depth buffer. | 375 | Depth and stencil must clear for the right eye in every frame. The clear at pm_gpu.c:1751 runs only when r_wantclear is set. Medium. |
| R3 | Top and right colour as RGB565 (188 KiB each, not 375) | 750 | RGB565 render buffer and transfer format. | 375 | 8 bit to 5 or 6 bit colour. Every scene changes, so a picture check is needed. Not byte-equal. Medium to high. |
| R4 | Off-screen full images (siz 2 pass) as RGBA5551 (256 KiB each, not 512) | 2048 | 5551 texture and render format, pm_tex format path. | 1024 | Format change in tex_hdr, pm_tex bind, and the snap copy. 1-bit alpha. The N64 source is 16-bit 5551, so the result may match the source more closely. Not measured. High. |
| R5 | OFF_N from 4 to 3 full images | 512 | Fewer full images. | 512 | A fourth image in one frame draws on the screen (V05 log line). Per-frame demand must be read from logs first. Medium. |
| R6 | Small arena from 12 to 8 blocks | 192 | Fewer 64 x 64 blocks. | 64 | Blocks are never freed. Demand per run must be measured (V05, FX2a logs). Low to medium. |
| R7 | Bank rounding waste | 36 | Slab steps of 128 B, not 32 KiB (pm_tex.c:468). | 36 | None known. Small gain. Low. |

Items to keep:
- Depth D24S8 (375 KiB each): stencil is used (stencil_on, pm_gpu.c:1076). A D16 buffer has no stencil, so it cannot replace D24S8 now.
- Copy block (512 KiB): the grab copies a 240 x 400 strip into a 256 x 512 power-of-two box. No smaller box fits.
- Full image (512 x 256): the 320 x 240 game picture needs a 512 x 256 power-of-two box. Only the format (R4) can shrink it.

## 7. Summary

- VRAM is 6144 KiB. Targets use 1500 KiB (24.4%). Arena uses 192 KiB. Copy and full images use 2560 KiB (41.7%). Texture cache uses 1856 KiB (30.2%). Waste is 36 KiB (0.6%).
- Largest avoidable item by bytes: R1 (750 KiB), then R4 (1024 KiB, high risk).
- Nothing in VRAM is allocated or freed after boot, except the cache slots.
