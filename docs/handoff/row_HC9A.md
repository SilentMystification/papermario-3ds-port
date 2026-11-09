# Row HC9A: hash index for pm_tex_slot_find (plan_hotcode.md stage HC9, part a)

## User facts
User (2026-11-09): "are there any large buffers that are unused or rarely used kept in L1 cache that eat up space causing the cache
misses?" Found: pm_tex_slot_find scans 1024 slots x 136 B (139 KB) linearly on a texture hint miss and on every model miss
(pm_gpu.c:3054 passes an empty hint); one call reads more than 8 times the 16 KB D-cache. XL de415917f mac_01: 20.2k D-miss/frame.

## Start state
`sh 3ds/tools/wt_new.sh HC9A`, branch HC9A. PM_ROW=HC9A, PM_LANE=HC9A. One Haiku agent. Only 3ds/src/pm_tex.c (+ its host test).
HC1 runs in parallel and may touch pm_tex.c debug sites: keep this row to the slot lookup.

## Steps
1. Read pm_tex_slot_find and every caller; name the key the scan compares (image address, tlut, fmt/siz, w/h, ...).
2. Add a power-of-two hash index (open addressing or a bucket chain in a small uint16_t array, about 2048 entries) keyed on the same
   fields, kept up to date on slot load, evict and reset. The lookup reads one index line plus one slot.
3. Delete the linear scan (no fallback path). No division (mask), no double.
4. Host test: insert, find, evict, reinsert over more than 1024 textures; the index and a reference scan agree on every lookup.

## Gates (light)
- nd_host PASS with the new test.
- Byte-equal against stored base shots: spot_mac01_det, mdl5_iwa10_det (texture loads unchanged).
- memcheck OK (index about 4 KB). Console (user): prof push, pmu_d per frame at mac_01 and kmr_10 vs de415917f.
