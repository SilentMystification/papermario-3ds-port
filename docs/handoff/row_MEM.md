# Row MEM: smaller 3dsx, smart memory use, no duplicates (user, 2026-11-07)

User: "make sure our memory utilization is smart and we dont have duplicates of unneeded things." The O3DS XL netloader refused the 13.585 MB 3dsx ("not enough free ram"); no launcher workaround.

## Oracle plan MEM
Sources: build3ds/pm_3ds.map. Note this map is from the Nov 6 15:31 build; rerun the same awk on the current build. Section totals:
- .text 3.30 MB (game 3.00, port 0.46, libs 0.25);
- .rodata 0.20 MB;
- .mapstat 0.23 MB;
- .data 9.06 MB (game 9.0; no world/dead maps linked);
- .bss 9.58 MB (game 6.2, port 3.7).
The 3dsx carries .text + .rodata + .mapstat + .data, about 12.8 MB plus headers and relocations, which gives the 13.6 MB file. The netloader buffers the whole file, and 13.585 MB is just over the O3DS XL launcher's free RAM. So the file must shrink by about 1 MB or more for margin.
Ranked (file first, then runtime):
1. .text at -O2 for all game code (CMakeLists.txt:9). Game C is 3.0 MB.
   - Fix: -Os for GAME_TU, except the per-frame hot files named by the census and prof (model.c, sprite.c, npc.c, entity_model.c, animator.c, effects.c, the effect update files). Port code stays -O2.
   - Expect 0.5-0.8 MB less text. Gate the hot list with gbi-phase and step-time numbers within +2%.
2. .mapstat, 227 KB of zeros in the file. map_restore.ld starts the section with `LONG 0x0`, so it is PROGBITS and every map .bss block placed in it (for the __ms_*_b0/b1 bounds) is stored as zeros.
   - Fix: keep the boundary symbols but make the section NOLOAD (or move the bss part into .bss with the same symbols). The file loses 0.22 MB.
   - Gate: map_restore md5 rule unchanged; map re-entry test (tour) unchanged.
3. Map pristine copies are never freed (runtime duplicate). pm_mapdata.c:117 mallocs a copy of a map's .data at its first entry and keeps it for the rest of the run. This grows toward the sum of all map .data (world about 4.5 MB) over a playthrough, and duplicates bytes the 3dsx file already holds.
   - Fix: on re-entry, read the pristine bytes from the 3dsx file's .data segment on SD. The homebrew loader maps at 0x00100000, the link base, so the file bytes equal the loaded bytes; prove it with a memcmp check on first entry in one debug run. Alternatively keep an LRU of 4 copies and re-read on a miss.
   - Gate: tour run; the restore probe (pm_mapdata_probe_after) shows 0 differences.
4. ROM duplicate: gen_rom_remap.py keeps the effect_gfx_* ROM ranges ("the port does not load yet, but a fix will need"), while src/effects/gfx/*.o is linked in .data (0.65 MB in the map).
   - Fix: drop those ranges from pm_rom_keep unless the ROMKEEP drop counter (pm_rom_drop_stat) shows reads. That saves their size from the 26.3 MB resident ROM and from the boot read (overlaps BOOT item 1).
   - Measurement: the sum of the effect_gfx ranges from splat, and one tour with the drop stat on.
5. Over-sized N64 heaps in .bss:
   - heap_generalHead 1.5 MB, heap_spriteHead 1.0 MB, display contexts D_80164000 1.34 MB, TextureHeap 0.42 MB, AuHeapBase 0.35 MB, gBackgroundImage 0.26 MB.
   - Fix: a high-water line per heap (general, sprite, collision, world entity, display list fill per context), run over the catalog. Size each to its high mark plus 25%. Likely 1.5-2.5 MB back.
6. Unused N64 buffers:
   - pm_unused_shape_data (32 KB) and pm_unused_80200000 (D_80200000 renamed in heaps.c, about 224 KB): delete if the grep finds no reader.
   - gFrameBuf2 (150 KB): nuGfxCfbNum is 2. Drop it if no reference uses the third buffer.
7. Port static tables in .bss (the deltas between symbols in the map):
   - the nd_* block after nd_st (about 0.94 MB: model cache, dlc and entity pools), pm_tex tables (0.45 MB), nd_mv (0.22 MB), log queue and buffers (about 0.45 MB), pm_au2 (0.15 MB), census and prof tables (pm_ph_*, os_*, mdc_*, nd_nat_why, about 0.1-0.2 MB).
   - Fix: size the pools from their logged high marks (ring, pool and cache hi lines). Census and prof tables move to one cold calloc made only when prof/census is switched on.
8. Walker-era buffers: the CPU vertex batch (vert_base linearAlloc in pm_gpu.c) and the Vert batch go with RECT-3/TRI-4. Check that the allocation itself is deleted, not just unused. The dd_on verts[] drawlog goes in WK-4.
9. Linear heap:
   - the live-log socket buffer (1 MB) is taken whenever the app is netloaded (pm_main.c: `live = netloaded || livelog`). Make it opt-in (`livelog` only), so netloader launches do not pay 1 MB.
   - C3D command buffer 1.5 MB against a worst seen of 369 KB: keep it (4x rule).
   - Check that VRAM staging copies are freed after upload (pm_tex vram_stage): a kept staging copy is a second copy of every texture.
Measurement:
- Per build: the map totals and the top 30 .data/.bss symbols (the awk used here, written as 3ds/tools/map_sizes.py).
- The boot `pm_mem_log("boot")` line and `mem` tick lines.
- The boot_mem bench linear_kb.
- The new heap high-water lines.
- The pm_mapdata copy total per map entry.
- One O3DS netloader send.
Gate:
- 3dsx at most 12.5 MB (at least 1 MB under the failing 13.585 MB), received by the O3DS XL netloader.
- Peak heap after a 10-map tour lower by at least 3 MB (boot mem line plus mapdata total).
- mac_01 byte-equal; golden_multi and a tour unchanged; every heap high mark below its new size with 25% margin.
Shared with BOOT: items 1, 2 and 4 also shorten the 3dsx load and the ROM read.

## Rows (two agents in parallel, own worktrees)
MEM-A FILE SIZE (first, urgent): 3ds/tools/map_sizes.py (map totals, top 30 .data/.bss/.text objects) on the current build; item 2 (.mapstat NOLOAD, keep the boundary symbols); item 6 (unused N64 buffers: grep proof, then delete); item 1 (-Os for game TUs except the hot list, hot list from the census/prof files named in item 1; gbi-phase and step time within +2 percent at mac_01 and iwa_10). Gate: 3dsx at most 12.5 MB; mac_01 byte-equal; golden_multi (title) and one map change by eye. Hand back the branch at once when the 3dsx is under 12.5 MB: the orchestrator pushes to the console.
MEM-B RUNTIME: item 9 (live-log buffer only with the livelog switch; push_3ds.sh passes livelog so console pushes still get the live log), item 3 (pristine map copies), item 4 (effect_gfx ROM ranges, with the drop counter evidence), item 5 (heap high-water lines, then sizes), item 7 (pools from high marks, census/prof tables in one cold calloc), item 8 check (CPU vertex batch allocation deleted with RECT-3/TRI-4; coordinate, do not delete it yourself). Gate per item as the plan says; peak heap after a 10-map tour lower by at least 3 MB.

## Oracle plan MEM-B v2
Accepted: premises a-e were wrong. Netloader: only the 3dsx file size counts (the launcher buffers the file); .bss is not in the file, so MEM-B does not touch the netloader limit. That is MEM-A. MEM-B is about the game's own peak.
That peak does matter on O3DS today. The log says "MODE = O3DS / 64MB cap". pm_rom_init (pm_os.c) leaves only 3458 KB of heap after the 26.3 MB packed ROM; the ROMFIT note records that a 3584 KB leave was 94 KB short. The image, .bss (9.6 MB), the linear heap and the ROM share that cap, so every MB of .bss returned is margin for the ROM and runtime allocations.
Real large users (in .bss unless noted):
- N64 heaps enlarged for 3DS (include/macros.h:140-152): GENERAL_HEAP_SIZE 0x180000 (N64: 0x54000, so 1.17 MB extra), SPRITE_HEAP_SIZE 0x100000 (N64: 0x40000; the comment cites the intro cast at about 386 KB decoded). Also the display contexts D_80164000 (1.34 MB), TextureHeap (0.42 MB), AuHeapBase (0.35 MB), pm_shape_buf (256 KB), pm_hud_aux_cache.
- Port tables, about 3.7 MB: nd_* pools (model cache, dlc and entity pools), pm_tex tables, nd_mv, the log queue and buffers, census and prof tables.
Instrumentation (one row, cold, no hot-path cost above an add):
- Heaps: link-wrap _heap_malloc, _heap_malloc_tail and _heap_free (src/43F0.c:78, :150; --wrap as the port already does for other functions). Per head pointer (general, sprite, battle = gFrameBuf2, collision, world entity, the map/auxiliary heaps) keep: used (plus the size on malloc, minus the HeapNode length on free), hi_used, and hi_top (the highest block end above the base, which is the fragmentation-relevant number).
- Display lists: per context, the max of gMainGfxPos - mainGfx per frame.
- Port pools: one `poolhi` line with the existing high marks (ring vtx and idx, gv_palmax, tmpl_n, nd_mc entries and pool bytes, entity and dlc pool bytes, pm_tex slots, nd_mv entries, the log queue depth max).
- Print `heaphi f<n> <heap> used/hi_used/hi_top/size ...` and `poolhi ...` every 300 frames, at exit and in the crash line.
Shrink rule:
- New size = max(hi_top over the gate runs) x 1.25, rounded to 16 KB. Never below the N64 size for the N64 heaps.
- Census and prof tables move to one cold calloc made only when prof or census is switched on; they cost 0 in a normal run.
- Pools sized from poolhi, with the same 1.25 margin. A pool that overflows takes its existing named path (flush or recompile) and logs it, and never corrupts.
Gate:
- Runs: a 10-map tour (the existing tour scenario), battle_start plus one boss battle, pause_spin, intro_native (the intro cast for the sprite heap), golden_multi.
- Every hi_top below 80% of its new size; no heap-full or ASSERT line; mac_01 byte-equal.
- The boot mem line shows the heap free after the ROM grown by the .bss saved. One O3DS push for the same lines.
Decision on the 2 unused decomp symbols: delete them. In src/heaps.c, put the pm_unused_shape_data (gMapShapeData copy, 32 KB) and pm_unused_80200000 (16 KB) definitions under `#ifndef TARGET_3DS`, and drop the -D renames in CMakeLists.txt. Proof: a grep shows no reader (the renames guarantee it; heaps.c is the only user). Gain 48 KB.

## Oracle decision MEM-B v3
How free memory is computed (pm_mem.c:72-116 __system_allocateHeaps):
- avail = commit_max - commit_cur (the process commit limit, minus what the loader already committed: the 3dsx text, data and .bss, plus stacks), then capped at PM_FCRAM, then minus the 128 KB socket block.
- One MEMOP_ALLOC_LINEAR of avail: the first PM_OURS - soc bytes are the linear pool (14208 KB); the rest is the newlib heap (sbrk). The ROM (pm_os.c, 26.3 MB) is malloc'd from that heap.
- So .bss competes with the heap only when the commit, not PM_FCRAM, is the binding limit. Your Azahar line (heap 51200 + linear 14208, about 64 MB, sbrk-left 25 MB, region 96 MB) is PM_FCRAM-bound. That is why removing 48 KB changed nothing, and why my "3458 KB left" premise (the ROMFIT note in pm_os.c) does not hold there.
- On a 64 MB O3DS region the commit binds: avail = 64 MB - (image + .bss ≈ 23 MB) - stacks. There every KB of .bss saved is a KB more heap.
Decision:
1. First get the console boot line (pull-o after one boot) with commit_max, commit_cur and avail; pm_mem.c already stores commit_max and commit_cur, so print them in the boot line.
2. If commit_max - commit_cur < PM_FCRAM on the O3DS XL, shrinking matters; go to steps 3-4.
3. Corrected measurement. hi_used is inflated because the counters are not reset when a heap is re-created: general, collision and battle heaps are rebuilt with _heap_create at map and battle load. That is also how collision shows 154 KB in a 96 KB heap. The fix: reset a head's counters in a wrap of _heap_create. At each heap re-create (before the reset) and every 60 frames, walk the node list (cold) and record used = Σ(length + sizeof(HeapNode)) over allocated nodes, top = max allocated node end - base, and the largest free block. Size each heap by the max top.
4. Shrink: general back to the N64 size 0x54000 if the corrected top is at most 268 KB (Azahar shows 188 KB; this saves 1.2 MB). Sprite to roundup(top x 1.25, 64 KB), likely 768 KB (saves 256 KB). Collision and battle keep their N64 sizes. Same gate as v2 (tour10, bowser_intro_fire, intro_native, pause_spin, a battle; tops under 80%).
5. If PM_FCRAM binds on the console too: close MEM-B after the cold census calloc. The O3DS action that matters is then the 3dsx file size (MEM-A: -Os for game code, .mapstat NOLOAD) for the netloader.

## Rules
Code first (CLAUDE.md rule 5). Own worktree per row (mema, memb) from nds1; copy assets/us and ver/us/build. No toggles (switches only for debug/measure), no stubs, no attribution lines, never commit 3ds/src/map_restore_table.c (the map_restore md5 rule stays). Keep Azahar open. Rebase on nds1 before each commit; check merge markers per commit. Delete this file when both rows are done.
