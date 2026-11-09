# Row MEMFIT: ROM no longer fits on the O3DS (need 26 MB, free 25 MB)

## User facts (2026-11-08 21:4x, O3DS XL .131, main d77987446 build 3299)
Boot stops: "Not enough memory for the ROM: need 26 MB, free 25 MB" (3ds/src/pm_os.c:257; cap = pm_heap_bytes() - 3458 KB). Build 3212 (e01685a71) ran on the same console.

## Cause (orchestrator map diff, 3ds/logs/builds/pm_3ds_3212.map vs pm_3ds_3299.map)
.bss grew 413,475 B: `pf_shape` 262,144 B and `pf_hit` 131,072 B (3ds/src/pm_prefetch.c:17-18, MAPX core-1 prefetch, static), `mfr_k` 11,904 B (3ds/src/pm_gpu.c:3068, TIK19 mfr debug table, static). .text +16,984 B.

## Fix
1. pm_prefetch.c: no static buffers. Allocate the two side buffers from the heap when a prefetch job starts (malloc/memalign 16, sizes as now), free them when decode_yay0 takes or rejects them (or at the map load end). If the allocation fails, skip the prefetch (the game thread decodes as today). One path only; pf_buf becomes the allocated pointers.
2. pm_gpu.c mfr_k: allocate the table on the first use under the `mfr` switch (or make it 4 entries); no static 11.9 KB.
3. Report: .bss before/after (from the map: total .bss, and that pf_shape/pf_hit/mfr_k are gone); check that no other new static array over 4 KB was added since build 3212 (the map diff; list them).

## Start state
Worktree build3ds/wt/memfit, branch memfit from main.

## Done criteria
1. Build; nd_host PASS; .bss at or below build 3212's size plus 16 KB.
2. xfer_map_mac01 (Azahar): the xfer line still shows pre 2 (prefetch used) and yay0 ms about as before.
3. spot_mac01_det byte-equal to a build of main.
4. The user pushes to the O3DS XL and the game boots (final gate).
Affected scenarios (no others): xfer_map_mac01, spot_mac01_det.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=memfit PM_ROW=memfit on every build.sh and bench call. No git stash. No toggles, no stubs.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=memfit PM_LANE=memfit sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=memfit PM_LANE=memfit python 3ds/tools/bench.py run <bench> --row memfit --lane memfit --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "memfit ready <hash>".
