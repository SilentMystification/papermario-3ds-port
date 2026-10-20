# PG2 loader design (texture bake, step 2)

Status: design only (PG2a). The loader edit goes to 3ds/src/pm_tex.c when the shared files are free. Line numbers are for pm_tex.c at commit 3e722a275 and must be checked again before the edit.

## What PG2a delivered
- 3ds/src/pm_texbake.h: pack format (TbHead, TbRec), request format (TbReq), key (tb_key), source length (tb_src_len). Plain C. The loader includes it.
- 3ds/tools/texbake.c: `texbake bake <req.bin> <out.pak>`. Writes the pack and prints count, pack size, index RAM and format counts.
- 3ds/tools/texbake_test.sh: host test. It extracts the real decode code from pm_tex.c (texbake_ref.py), builds a corpus (texbake_corpus.py: 1025 repo textures with every format of their bit size, 400 random cases with masks, mirror, stride, odd sizes, palettes), and compares the tool bytes to the bytes that load_inner gives to C3D_TexLoadImage. Result: 3384 requests, 0 mismatches. A mutation (one IA4 constant changed) gives 813 mismatches, so the test sees faults.

## Source of the request list
The game loads textures from the map and asset archives at run time, so an offline scan of display lists is not possible without the game. Two ways, both use the same request file:
1. Switch `texdump` (loader hook, below): at each miss of a plain texture the game appends one request to sdmc/3ds/PaperMario/texreq.bin. One full cycle gives the list of all textures of the demo path. Other maps come from the warp tour (tour=<maps>).
2. The repo corpus (texbake_corpus.py) is for the test only. Its formats are guesses.
Then on the host: `texbake bake texreq.bin texbake.pak`, copy the pack to sdmc/3ds/PaperMario/texbake.pak.

## Hooks in pm_tex.c (small commit)
All hooks are for a plain texture only: `lut == NULL`, `!src_native` (pm_fb_native(img) is 0), not an off-screen image (the early return at line 393 stays first). Everything else decodes as today.

1. Includes and state, near line 54: `#include "pm_texbake.h"`, `static FILE* tb_f; static TbRec* tb_idx; static unsigned tb_n; static int tb_on, tb_dump, tb_check; static unsigned tb_hit, tb_miss, tb_bad;`
2. pm_tex_init (line 280): if switch `texbaked` or `texbakecheck`: open texbake.pak, read TbHead, check magic and version, read the index (24 bytes per texture) into linear or heap memory. On any fault log one warning and leave tb_on 0. If switch `texdump`: open texreq.bin for write, write the 8 byte header (count is patched at exit, or the tool reads until end of file: use count 0xffffffff and let tb_walk stop at the end of file).
3. In load_inner, after the cache miss is certain, after line 468 (`src_native = pm_fb_native(img);`), before line 471:
   - build `TbReq q` from the final values: fmt, siz, ms = mk_ms, mt = mk_mt, w = width, h = height, stride, mw = (mk_on && mk_ws && mk_ws < width) ? mk_ws : 0, mh likewise with mk_wt and height, tlut_n = (fmt == FMT_CI && tlut) ? tlut_n : 0. These are the values expand() computes at lines 147 and 148.
   - `texdump`: write q, tlut bytes (tlut_n * 2) and tb_src_len(&q) bytes from img. Then go on to the normal decode.
   - `texbaked`: `key = tb_key(&q, tlut, img)`, binary search tb_idx. On a hit set pw, ph, gf from the record (map TB_GF_* to GPU_RGBA8, GPU_RGBA5551, GPU_LA8), `bpp`, `bytes = rec.bytes`, and skip lines 471 to 480 (expand) and 506 to 553 (tile copy): fseek and fread rec.bytes into `scratch + 256*256*4` (the second scratch half, the same place the tile copy writes) and set `upload` to it. Lines 481 to 505 (slot choice, C3D_TexInit, filter, wrap) and 554 to 578 (C3D_TexLoadImage, slot fields) stay as they are.
   - `texbakecheck`: on a hit decode as today, then memcmp the tiled bytes with the pack bytes. Count tb_bad and log the first 8 (fmt, siz, w, h). Use the decoded bytes for the upload.
   - A miss in the pack: decode as today. Count tb_miss.
4. pm_tex_prof_report (line 341): print tb_hit, tb_miss, tb_bad with the existing report.

The cache compare at lines 442 to 466 uses img, tlut pointers and stays unchanged: the bake only replaces the decode on a miss.

## Cost and limits
- The key hashes the source rows once per miss (FNV-1a, 32 bit words). Typical texture 512 B to 8 KB. It is far below the expand cost (one switch per texel).
- Corpus estimate (texbake output, every file in 1 to 3 formats, so an over-count): 3085 textures, pack 9.9 MB, index 74 KB RAM (24 B per texture), largest texture 64 KB. The real pack for the demo path is smaller. The full game (map archives) is 10 to 40 MB by the plan guess. A first pack of the demo cycle gives the number.
- The pack read per miss is one fseek and one fread of 0.5 to 64 KB. SD speed on the O3DS is not measured. Azahar reads from the host disk and cannot show it.
- Key size 64 bit. A collision gives a wrong texture. The record holds pw, ph and gf, and the loader must compare them with the values it expects (pot_at_least_8 of width and height, gf from gpu_fmt). A mismatch counts as a pack miss.
- A texture with a CPU-written raster or palette that changes in place (flash palettes, imgfx) changes its palette hash or source bytes, so its key changes and it decodes as today. A baked entry for a raster that changes in place is a fault: texbakecheck finds it (tb_bad above 0).

## Done check for the loader row
texdump full cycle writes texreq.bin. texbake bake writes the pack and prints its size. texbaked with texbakecheck full cycle: tb_bad 0, hit rate, texmiss and texKB per frame equal (0 frames differ), statehash equal, golden toad yard battle 0 differences. Then texbaked alone: host time by the recipe in 3ds/logs/perf/speed_recipe.txt and the tex phase ms from phase_report.py.
