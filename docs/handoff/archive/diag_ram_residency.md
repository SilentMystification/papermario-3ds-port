# RAM residency: what is in RAM and what is lazy (read-only check)

Sources: 3ds/src/rom_remap.h, 3ds/tools/gen_rom_remap.py, 3ds/src/pm_os.c:208-281, 3ds/src/pm_mem.c:18-20,104-132, 3ds/src/pm_tex.c:21-23,1053-1084, src/world/world.c:151-291, ver/us/baserom.z64 (mapfs table at 0x1E40020), log lines in 3ds/logs.

## Console and heap
- App region cap: PM_FCRAM = 64 MiB on O3DS. N3DS is capped to the same 64 MiB by this build (pm_mem.c:18, `avail > PM_FCRAM`).
- Linear pool: 14208 KB (`ours`, pm_mem.c:20 and :112). Linear fixed 4968 KB (pm_mem.c:20). Texture budget 1536 KB (pm_tex.c:22).
- Newlib heap: 51200 KB at full 64 MiB commit (derived: 65408 - 14208 KB). Logged `heap total 51200KB` and `45056KB`.
- Game heap cap for ROM: heap - 3458 KB = 47742 KB (logged `cap 47742KB`).
- Packed ROM 27,107 KB sits in the heap. Heap left after ROM: about 24 MB at most.
- Logged heap used 27,338 KB with the packed ROM: non-ROM heap use was about 231 KB at that log point (early boot, not peak).

## Table
| category | now | size MB | lazy or resident | fit on O3DS |
|---|---|---|---|---|
| Code (3dsx, ARM) | in 3dsx, native | ~2 (3dsx) | resident | yes |
| Packed ROM image, all kept ranges | heap, resident | 26.47 (27,757,166 B) | resident, copied at boot | yes, 27.1 of 47.7 cap |
| Packed ROM, splat type `code` ranges | heap, resident (not dropped) | 2.16 (8.1% of image) | resident | yes, but wasted |
| Audio banks (pm_sbn, raw) | heap, inside packed image | 10.26 | read at play, 0x2000 chunks (src/audio/core/engine.c:1421,1428) | yes |
| Map data, compressed (pm_map_data, raw) | heap, inside packed image | 9.75 | read at map load | yes |
| Map data, decompressed (shape+hit+tex) | built per map load, freed | 21.74 total (shape 17.09, hit 1.55, tex ~3.1) | lazy: world.c:152-155 (decode_yay0), :212-215 | yes if all resident, uses ~22 MB of heap |
| Sprites, raw (pm_sprites) | heap, inside packed image | 2.25 | lazy decode (pm_map.c spr_load path) | yes |
| Messages, raw (pm_msg) | heap, inside packed image | 1.01 | lazy: src/msg.c:223,831 | yes |
| Imgfx, icons, ci8/ia8 raw | heap, inside packed image | 1.20 | lazy | yes |
| Texture cache (pm_tex.c) | linear, 512 slots | budget 1.5; live 0.17-1.06 seen | lazy: upload on miss | yes |
| Texture decoded total | not counted | unknown | lazy | unknown |
| Display lists | per-frame build from map shape | n/a | lazy | yes |
| Save (flash) | heap mirror `flash_mem` | 0.0625 (8 sectors) | resident | yes |

## Lazy decode and read call sites
- Yay0 decode: decode_yay0 (src/world/world.c:154, pm_map.c:360). Map shape only. Time: `yay0` phase exists in pm_phase.c:22 (`phase phsum`).
- Texture decode: pm_tex.c miss path, 1024x512 RGBA5551 scratch plane (pm_tex.c:23). Misses per frame in steady play: 8 (log `texstat f9000: miss 8`).
- Display list fetch: from ROM via DMA (src/43F0.c:439-453, chunk 0x2000).
- Sound load: src/audio/core/engine.c:1421,1428.
- Model and texture DMA: src/model.c:2053,2085,2097,2142,2277.
- Packed image copy: pm_os.c:255-263 (one memalign, 16 B aligned).

## What is needed to keep each category resident
- Drop code from the packed image: saves 2.16 MB (8.1%) of the 27.1 MB image. Needs the generator (gen_rom_remap.py) to drop splat `code` segments. Not done.
- Keep asset data only, raw: 25.60 MB (27.76 - 2.16). Fits O3DS heap (47.7 MB cap) with ~22 MB spare.
- Decompress all maps resident: 21.74 MB decoded instead of 9.62 MB compressed (+12.1 MB). With raw audio and sprites, total about 37 MB. Fits 47.7 MB cap on paper, leaves ~10 MB for game heap. Not measured.
- Decoded textures: size not counted. Needs a count of decoded texture bytes (texbake.pak or the decode path).
- Decoded audio (VADPCM to PCM): size not counted. VADPCM expands about 3.5 times on paper: 10.26 MB would become about 36 MB. Does not fit with maps. Keep audio raw.
- Drop ROM image after decode: frees 27.1 MB only if every category above is decoded and stored separately. Otherwise no gain.

## Not verified
- Decoded texture bytes and decoded audio bytes (not counted).
- Which console produced the 45056 KB and 51200 KB heap lines (log has no console tag).
- Whether `splat code` ranges hold only MIPS code (not checked byte by byte).
- Peak heap during play (only one early-boot log line measured).
