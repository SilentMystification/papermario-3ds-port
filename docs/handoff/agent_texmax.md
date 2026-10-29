# Handoff: texmax (VRAM texture cache size, full map sweep)

## State
- Cache is one VRAM block of 1536 KB at boot (pm_tex.c, BUDGET). If the block fails, slab_init tries 768 KB, logs TEXCACHE FALLBACK, and logs TEXCACHE OFF if both fail. tex_cap holds the size that came up.
- Two-slab attempt (2 x 1536 KB) failed textures from map 31 on (batch of 30 maps, linear pool peak 14094 KB of 14208 KB; cumulative tex failures 34 at hos_00, 2496 at hos_01). Reverted.
- Single 3072 KB block failed at boot (5394 KB free). Not a valid layout.
- 420-map full run and HEAD baseline run: NOT DONE. Run A (420 maps) was on a working-tree binary, not HEAD: void.

## Commits
- e7edbbbc3 VRAM low-water mark (pm_mem.c, pm_gpu.c, pm_tex.c hunks): vramfree min on HUD and texmap line, vram alloc fail lines.
- 71121e341 docs/handoff/sweep_texmax.md (batch 1 table, 3072 KB failure).
- 12c5e9887 partial commit, left HEAD unbuildable (slab global still used). Do not use as baseline.
- 447e0dcbf fix: slab code converted to pointer extents, one block, 768 KB fallback. HEAD builds. Boot check passed (30 maps, 0 tex failures).
- Other agent's commits (tex_inv1, tex_inv2, review bf994d037) are in history. Do not revert them.

## Sweep needs
- PM_ROW=sweep420 (new budget row). Set PM_ROW and PM_LANE=texmax in every shell.
- One start for all 420 maps: -Timeout 1800 -LockWait 180 -Milestone -Force -Until "tour: done".
- Debug string: "warp=arn_02:0 story=ch0_toad_town tour=@tour_all.txt shots shotwin=999999:1 shotmode=4:21 texmax". Switch string limit is 255 chars: use tour=@file.
- Tour files live in the SD folder: $APPDATA/Azahar/sdmc/3ds/PaperMario/. tour_all.txt has all 420 maps (map:0 per line). tb_00.txt ... tb_13.txt are 30-map batches.
- Full list source: 3ds/maps.cmake (src/world/area_<a>/<map>/). Copy kept in the scratchpad (maps_all.txt, tour_maps.txt).
- Each run writes 3ds/build3ds/lane-texmax/last_log.txt. Copy it after each run.
- Build: python 3ds/tools/gen_map_restore.py; PM_LANE=texmax sh 3ds/build.sh; then restore 3ds/src/map_restore_table.c from 3ds/src/map_restore_table.c.bak. Never commit that file.

## Log fields (per map)
- "tour: next <map>": map start. "tour: in map": loaded.
- "texmap since last map: hit N miss N up NKB inval N same N diff N evict N vramfree min N KB"
- "mem map ... texture failures N": cumulative tex failures. Delta above 0 = failure.
- "texmax N KB of 1536 KB before this map load": cache used.
- "vram alloc fail <what> <KB>": VRAM allocation failure (slab, offslot, snap, grab).
- Not logged yet: slot count. Add it if the trend question needs it.

## Open items
1. 420-map single run on HEAD 447e0dcbf. Record per-map table. Name the first map with tex fail delta above 0.
2. Baseline run on 71121e341 (single 1536 KB, before tex_inv2) with the same list. Build it in a worktree (git worktree add) with PM_LANE=base, then copy pm_3ds.3dsx into build3ds/lane-base/. Diff the two tables.
3. Shots: every 5th map plus first fault map. Compare the two layouts.
4. Effect scenes (pause, file select, battle start, flame): not run.
5. The HEAD build and the boot fallback were tested on 30 maps only.
