# Memory budget (O3DS), 2026-10-18

Read-only audit. No code changed. Old audits: docs/HISTORY.md, sections M01, M02 ("Redundancy audit, step 2"), S30, H03. This file does not redo them. It adds the H03 effect and the new margin.

Tags: MEAS = measured now (Azahar log or link map of the build of 2026-10-18). M01/M02/S30 = from that result in HISTORY.md. EST = estimate. HW = only the S28 hardware run can confirm.
Assumption: the O3DS application limit is 64 MB (65,536 KB). This is an assumption. The boot line `mem commit limit` on hardware gives the real value (HW).

## 1. Finding: the Q7 number is old

Q7 says all 421 maps give about -1.1 MB. That number was computed with a 20 MB linear pool (before H03). H03 cut the pool to 14 MB. The heap on the O3DS gains 6,144 KB. All 421 maps now fit by estimate (section 4).

## 2. Region table (O3DS, 64 MB assumed)

| Region | KB | Holds | Tag |
|---|---|---|---|
| .text | 2,432 | code of 271 maps, engine, battle tree | MEAS (link map 0x262068 B) |
| .rodata | 111 | constants | MEAS (0x1bce4 B) |
| .data | 7,286 | linked map data, assets from PNG sources, engine data | MEAS (0x71d87c B) |
| .bss | 6,943 | game heaps, N64 base RAM 4 MB, buffers, log ring 96 KB | MEAS (0x6c7d88 B) |
| Image total (loader view) | 17,000 | text + rodata + data + bss, page aligned | MEAS (`used-by-image`) |
| Resident ROM (packed) | 27,106 | kept ROM ranges, one copy | MEAS (`rom resident`); 26,933 at M02 |
| Newlib heap, other use | about 440 | malloc by the game and port | MEAS (heap used 27,543 - ROM 27,106) |
| Linear pool | 14,336 | GPU command buffer, vertices, audio, scratch, render targets, texture cache | MEAS (H03) |
| - fixed part of the pool | 7,528 | all except texture cache | MEAS (H03) |
| - texture cache budget | 4,096 | peak used 1,209 | MEAS (budget), MEAS (peak, H03) |
| - pool margin | 2,712 | fragmentation reserve | MEAS (computed) |
| Stacks | in image or heap | game 256 KB (high-water 3 KB), audio 64 KB, log 32 KB, crash 2 x 16 KB | MEAS (boot log) |
| libctru and OS overhead | not itemised | inside the 64 MB limit and `used-by-image` | HW |
| Heap total on the O3DS | 34,200 | 65,536 - 17,000 - 14,336 | EST (Azahar shows 51,200 because it caps differently) |
| Heap used at map load | 27,543 | ROM + malloc | MEAS (Azahar) |
| Spare | 6,657 | 34,200 - 27,543 | EST |

Notes:
- The 4 MB N64 RAM block is not a separate allocation in `pm_mem.c`. The `pm_mem.c` comment says it stays free in the heap. I did not prove where it lives. If it is a separate 4 MB reserve, the spare drops by 4,096 KB (still positive, section 4). HW settles it.
- The GPU command buffer is 4 MB (`C3D_Init(0x400000)`, pm_gpu.c). I did not check whether it lies inside the 7,528 KB fixed part. The H03 measure counts all linear use, so it does.
- Audio: 8 x 8 KB linear buffers (M02). Inside the fixed part.
- The 3dsx file (10.9 MB) is not resident after load. It is not a copy.

## 3. Duplicates

| # | Copy | Bytes | Status |
|---|---|---|---|
| D1 | Resident ROM range and the game buffer that `nuPiReadRom` or `dma_copy` fills from it | bounded by live loaded data: heap_generalHead 1.5 MB, heap_spriteHead 1 MB, shape 256 KB, audio heaps (not measured) | EST: about 3 MB at most live at one time. Working set read in a 200 s run: 1.3 MB distinct (M02). Compressed ranges (Yay0) are not a byte copy: the loaded form is decompressed. |
| D2 | Linked map data and ROM map overlays | 0 | Removed: 14.4 MB of ROM ranges dropped (M01 6.4 MB, M02 5.7 MB) |
| D3 | `pm_unused_shape_data` (heaps.c copy of gMapShapeData) | 32 | M02: open, needs a shared heaps.c edit |
| D4 | gFrameBuf0..2 | 450 | Not a data copy. Real code uses them as scratch (M02). Keep. |
| D5 | Texture cache (decoded PICA copy) of N64 texels | peak 1,209 | Derived copy, not a duplicate. Needed for the GPU. |
| D6 | Extracted PNG, PNG.bin, splat output | 0 on device | Build time only. |
| D7 | ROM bytes kept but never read in the M02 run | 2,790 (of which 660 KB effect_gfx, 200 KB icons, 130 KB title bg) | Not a duplicate. Not proven dead (battle, pause, other maps were not traced). |

Total proven duplicate bytes: 32 KB (D3). Estimated live copy from D1: up to about 3 MB. Not 26 MB. The ROM is the only store of the data that is not loaded now. It is not a duplicate of it.

## 4. What the game reads from the ROM

Source: M01/M02 trace. 59,436 reads, 2,891 distinct ranges, 1,369,102 B distinct (M02, Azahar chain hos_05 to mac_00, 200 s, not all maps).

| ROM type | Read at run time | After link |
|---|---|---|
| Asset table, map shape, hit, texture assets (mapfs) | yes, per map load, 516 KB distinct | needed for every map: it is the data of the 421 maps |
| Sprites | yes, 145 KB distinct | needed |
| Audio banks and BGM stream | yes, 463 KB distinct, 1,280 B reads every few frames | needed |
| main / engine | 244 KB (8.5 KB inside `main`) | needed |
| Messages | 0.6 KB | needed |
| Compressed code overlays (engine1, engine2, evt, pause, filemenu, battle_code, map overlays) | no: linked in place, ranges empty | dropped (14.4 MB, M01/M02) |
| Effect graphics (effect_gfx, 660 KB) | read when an effect loads (S24) | keep |
| Never read in the run (2.79 MB) | no | unproven: needs a trace with all maps, battle and pause |

Answer: the ROM copy is still needed. It holds the asset data that is not linked (mapfs, sprites, audio, messages, entity models). Only a part of the 27.1 MB is needed. The part that is never read is at most 2.79 MB (EST, D7). The ROM image is not a copy of the linked code and map statics.

## 5. The 154 id-only maps and the new margin

- Size: +1,988 KB (kpa 340 MEAS in S30, the other five areas 1,648 EST in S30). It is static image (.text, .data, .bss) only. It does not count ROM bytes: map overlays are already dropped, so the figure holds no duplicate (D2). The mapfs assets of these maps are already inside the resident ROM (counted in the 27,106 KB). Not verified per map: the S30 split of 1,648 KB is an estimate.
- Blockers listed at S30: kmr_22 and kzn_19 (S19b, since done), flo_11 (needed battle.c, merged by M03). Not re-checked here.

| Line | KB | Tag |
|---|---|---|
| Spare now (271 maps, H03) | 6,657 | EST |
| 154 maps | -1,988 | EST |
| Spare, all 421 maps, all data resident, zero new duplicates | 4,669 | EST |
| Reserve limit used by the project | 500 | rule |
| Free above the reserve | 4,169 | EST |
| Worst case if the separate 4 MB N64 block is extra | 573 | EST |
| Worst case if the 154 maps cost 2x | 2,681 (or -1,415 with the 4 MB block) | EST |

Old Q7 view (pool 20 MB): 518 - 1,988 = -1,470 KB with the current image (-1.1 MB at S30 time). H03 adds 6,144 KB.

## 6. Options to remove the D1 duplicate and more

| Option | Saves | Cost and risk | No-stub |
|---|---|---|---|
| A. Do nothing more. Link the 154 maps. | 0 | Needs S30b only (maps_skip.txt, generator). Margin 4,669 KB EST. | Keeps the rule, removes the GotoMap guard |
| B. Read raw byte streams in place from the resident ROM (audio sample data, raw sprite data) instead of `nuPiReadRom` into a heap | audio heap size, not measured (measure first). Heaps stay in .bss, so RAM saved only if the heap shrinks. | Edits to shared `load_banks.c` and sprite load with TARGET_3DS. Alignment and endian risk. The ROM must not move. Medium. Most structs are big-endian and the port swaps them in place, so this works only for byte streams. | Keeps the rule if the real loader stays |
| C. Drop never-read ROM ranges (D7) | at most 2,130 KB (2,790 - 660 effect_gfx), EST | Needs a `romtrace` of all 421 maps, battle, pause. A dropped read gives zeros. Medium. | Keeps the rule |
| D. Delete `pm_unused_shape_data` | 32 KB | One shared heaps.c edit. Low. | Keeps the rule |
| E. Cut the texture cache budget 4,096 to 2,048 KB and the pool 14,336 to 12,288 KB | 2,048 KB to the heap | Peak was 1,209 KB (H03). More evictions. Texture failures skip draws. Low to medium. | Keeps the rule |
| F. Link map data by reference into the ROM instead of copying | 0 | Not possible. Map statics are mutable, little endian, with absolute pointers. They must live in RAM. Also there is no ROM copy of them (D2). | n/a |
| G. Store assets compressed in RAM, decompress on load | The ROM is already mostly compressed (Yay0). Extra gain unknown. | New container format and decode time on the ARM11. Large. | Needs no stub but is not needed |

## 7. Recommendation

Smallest change set: option A only. Link the 154 maps (S30b: edit `3ds/maps_skip.txt`, run `gen_map_sources.py`, remove the GotoMap guard). No new duplicate removal is needed for the fit.
- Spare after A: 4,669 KB EST (above the 500 KB reserve by 4,169 KB). All data stays resident. No SD streaming. No stubs.
- Take D (32 KB) and E (2,048 KB) only as backup if S28 shows less than 64 MB or a separate 4 MB N64 block.
- Take C only with a full trace.
- The user asked why memory was short. Answer: Q7 used the old 20 MB pool. The ROM (27.1 MB) and the image (17.0 MB) are real stores, not mostly duplicates. The duplicate part is small (32 KB proven, about 3 MB live copy at most).

## 8. What only S28 (hardware) can settle

| Item | Why |
|---|---|
| Real commit limit (64 MB assumed) | boot line `mem commit limit` |
| Real `used-by-image` and heap total | Azahar caps differently from the O3DS loader |
| Whether the 4 MB N64 block is extra | not proven in code |
| Libctru, OS, stack and thread overhead | not itemised |
| Linear pool fragmentation and texture failures (`mem map` line) | Azahar numbers only |
| Boot time of the 27 MB ROM read from SD | not timed on hardware |
| Real peak after all 421 maps run | EST until a hardware tour |
