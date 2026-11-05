# Texture cache eviction: investigation (TCE)

Scope: read-only trace of 3ds/src/pm_tex.c, 3ds/src/pm_mem.c, and one Azahar start (PM_ROW=TCE, mac_01 tour, lane native).

## Policy

1. Slot pick: a load takes the first of 512 slots (`pm_tex.c:21`) that is not live. With no free slot, it evicts (`pm_tex.c:1176-1178`).
2. Byte cap: a load evicts while `used + reserve > tex_cap` (`pm_tex.c:1177`). `tex_cap` is 3072 KB (`pm_tex.c:22`), or 1536 KB while bank B drains (`pm_tex.c:870-873`).
3. Fragmentation: if the best-fit extent search fails (`pm_tex.c:511-520`, `1205-1206`), the loader evicts one entry and retries until an extent fits or no entry is evictable. This can evict while `used` is below the cap.
4. Victim: `evict_oldest_done` (`pm_tex.c:734-749`) takes the lowest use stamp (`pm_tex.c:82`, stamp set on every hit and load). It skips only entries used in the current frame (`slots[i].epoch == epoch`). There is no age rule and no map-change purge in pm_tex.c. Age appears only in the stats (`pm_tex.c:834`, `855`).
5. HUD: `pm_mem.c:222` prints `tex %u/%uK` as `pm_tex_used()` (`pm_tex.c:795`, sum of live slot reserves, rounded to 128 B, min 512 B) over `pm_tex_budget()` (`pm_tex.c:796`, `tex_cap`). So the HUD number is live bytes over the current cap, not allocator pool use.

Early eviction: PROVEN that the code can evict below the byte cap (rules 1 and 3). INFERRED that this happens on a heavy map, because the one run below did not reach it.

## Azahar run (TCE, 1 start, 25 s cap)

- Scenario: warp mac_00:4, tour to mac_01:0, `loglevel=debug phase texstat`, until `texstat f300`. Log: `build3ds/lane-native/last_log.txt`.
- At frame 300: `texstat f300: hits 6633 miss 126 evict 0 (0KB)`.
- Live: 97 slots, 658 KB of 3072 KB. Texcache VRAM full, texfail slab: none logged.
- Result: no eviction on this map at frame 300. The cache held 658 KB, so the user's report is not explained by this run. It does not rule out eviction on a fuller map.

## Better policy

- Evict only when an allocation fails: keep the byte-cap check and drop the free-slot-only trigger (grow the slot array or reuse dead slots).
- Protect the previous frame too: skip entries with `last_frame >= tc_frame - 1`, not only the current `epoch`.
- Keep LRU by use stamp (already in place).
- On a fragmentation miss, free the LRU entry that is adjacent to a free extent before any other.

## Test

- Log: `texstat f<N>: ... evict E` with `live ... <KB>`. Pass when E > 0 and live is below 3072 KB, on a heavy map where the cache can fill.
- Also check `texcache VRAM full ... (used U)` (`pm_tex.c:1208`): U below the cap proves a fragmentation eviction. `texfail slab` (`pm_tex.c:1230`) gives free bytes and the largest hole.
- Use the `texcap=<KB>` debug switch (`pm_tex.c:787`) to force a full cache on a small map.

Files changed: docs/handoff/inv_texcache_evict.md only. Status line: 3ds/logs/perf/status_TCE.txt.
