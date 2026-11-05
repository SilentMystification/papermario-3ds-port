# plan_tex_invalidate: keep texture slots across source rewrites, so only LRU eviction removes a slot
(Opus plan, saved by the orchestrator.)

## 1. How the cache key works today
- The key is address and shape, not content: `img` pointer, `tlut` pointer, `phash`, `fmt`, `siz`, `w`, `h`, `stride`, `dkey` (pm_tex.c:916, 930, 941).
- Only the palette is content-checked: `phash` is an FNV-1a of the TLUT, computed on every load before the lookup (pm_tex.c:889-910). Image bytes are never hashed.
- A slot goes wrong when new bytes are written to the same RAM, so every RAM write that may hold a texture source calls `pm_tex_invalidate(ptr, size)`: pm_os.c:379 (after every nuPiReadRom), pm_map.c:375 (after every decode_yay0; the background raster and palette sit at one fixed address for every map), pm_main.c:200 and :263, pm_gbi.c:2267 (z-buffer, saved frame, cimg).
- `pm_tex_invalidate` (pm_tex.c:760-786) drops every live slot whose `img` or `tlut` start lies in the written range. If drawn this frame, it hides the slot and frees it at the next frame start (`stale`). It also bumps `pm_dlc_gen`, `pm_pdl_heap_gen`, `pm_pdl_touch_range`; those bumps must stay.
- Result: a revisited map re-DMAs/re-decodes its files into the same addresses with probably the same bytes, and every slot built from them is dropped. That is the 595 pass-2 misses.

## 2. Content-aware design
- At load (miss path only): add `shash` (32-bit FNV-1a over the image bytes the expander reads) and `slen` to `Slot`. `slen = (rows-1)*stride_bytes + row_bytes`, real rows (mask rows when `mk_on`). Glyph atlas slot: hash the whole sheet (`g->size`). Shaded slot: hash the CI4 source (lut is in `dkey`). Use the word loop of OPT-PH (pm_tex.c:895-905), byte loop if not word-aligned.
- In `pm_tex_invalidate`: do not drop. Set `s->dirty = 1`, `s->sgen++` (DLC replay records through `pm_tex_bind_slot` rebuild), clear `cur`/`aux`/`have` as now, `addr_sub(img)` and `addr_sub(tlut)` so the 4 KB page filter still skips the slot. All gen bumps stay. Exception: slots with `src_native` (frame buffer, z-buffer, saved frame) still drop (150 KB hash is about 1.5 ms).
- At lookup: helper `slot_valid(s)` at the three hit sites of `load_inner` (lidx, recent, scan) and in `pm_tex_glyph_same`. Clean slot: one flag test. Dirty slot: hash `slen` bytes from `img`. Equal: clear `dirty`, `addr_add` again, count a hit. Different: replace (stale path if `s->epoch == epoch`, else `drop`), continue as a miss. `pm_tex_load_rgba8`: key names the content, so a dirty slot is valid without a hash.
- Check at lookup, not inside invalidate: invalidate runs about 6946 times in one step, and a texture can span several DMA chunks (half-written bytes would be hashed many times). The lazy check runs once per slot per rewrite, at the first lookup after a map load, off the per-op path.
- A per-region generation counter alone fails: the write notice fires for identical bytes too (the 595 case). The counter can only mark dirty; the hash decides.
- Cost on O3DS [estimate]: word FNV about 6 cycles per word, about 1.5k cycles per KB plus about 32 FCRAM line fills of about 40 cycles: about 10 us per KB (byte loop about 25 us/KB). 64x64 CI4 (2 KB) about 20 us. Worst case per map load: about 300-400 KB of source in cache = 3-4 ms once, against a full re-expand, tile and upload today. The miss path gains about 10 us/KB for the hash at load.

## 3. Rows
1. tex_inv1 (about 35 lines, measure only, no behaviour change): add `shash`/`slen` computed at load. In `pm_tex_invalidate`, before each drop, hash the new bytes and count `same` and `diff`. Add both to the texmap line (pm_mem.c:184). Azahar: one tour start. Quantity: pass-2 `inval same N diff M`. Pass-2 misses and texbyte unchanged; the 15 shots (5 noisy excluded) pixel-identical. Gate: if `same` is below half of pass-2 inval, stop the series. Console: texstat `tx1` line, `us` per miss rises by 5% or less. (`same` is a lower bound: a partial DMA chunk counts as diff.)
2. tex_inv2 (about 55 lines, the change): invalidate marks dirty; `slot_valid` at the 4 hit sites; keep the `src_native` drop. Azahar: one tour start. Quantity: pass-2 misses (597 should fall to about row-1 `diff`); pass-2 texbyte falls in proportion; the 15 shots identical to row 1; evict stays 0. Console: push with texstat phase; `spike` lines on revisited maps show lower `texmiss`/KB and no `tex cache full`.
3. tex_inv3 (about 30 lines, cost and fill check): measure ticks in `pm_tex_invalidate` and in the `slot_valid` hashes per map; add both to the texmap line; measure `used` peak. Azahar: one tour start. Quantity: pass-2 ms in the hash; no pixel change. If the peak reaches the cache size, evictions go to the lowest stamp only. Console: per-map hash ms at or below 5 ms on O3DS.

## 4. Risks
- Hash collision with changed bytes: old texture shown. 32-bit FNV: about 2^-32 per check, about 1e-7 per tour. If ever seen, fold `slen` and a second word sum into 64 bits.
- TMEM reload of the same address with new content and no invalidate (CPU writes texture RAM directly): not seen today either, so nothing gets worse. Palette edits caught by `phash` on every lookup.
- Animated textures: UV panning does not change bytes. Frames DMA'd into one buffer: each change is a mismatch and a reload (as today).
- Ring reuse: a different texture at the same address gets a new key, so a new slot; the old dirty slot stays until LRU, so the peak rises (row 3 checks it). The same key with new content is caught by the hash. The off-screen slot ring (offt, OFF_N=4) returns before the cache.
- GPU still sampling a slot: never overwrite a slab in place on a mismatch; use the stale path when `epoch` matches.
- DLC replay: `pm_tex_bind_slot` skips the lookup, so `sgen++` at dirty is required.
- Invalidate scan cost: more live slots means a longer scan; `addr_sub` at dirty keeps the page filter as selective as today. Row 3 measures it.

Files: pm_tex.c (Slot :24-46, invalidate :741-787, hit sites :911-951, load store :1179-1208, glyph_same :227-251), pm_mem.c (texmap :183-184), pm_os.c (:379), pm_map.c (:375), docs/handoff/route_test.md (tour command).
