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

## Rules
Code first (CLAUDE.md rule 5). Own worktree per row (mema, memb) from nds1; copy assets/us and ver/us/build. No toggles (switches only for debug/measure), no stubs, no attribution lines, never commit 3ds/src/map_restore_table.c (the map_restore md5 rule stays). Keep Azahar open. Rebase on nds1 before each commit; check merge markers per commit. Delete this file when both rows are done.
