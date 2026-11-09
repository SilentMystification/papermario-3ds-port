# Hardware guidelines for source changes (3DS topology)

Applies to every code row, in 3ds/src and in the guarded src/ edits. State in each commit message how the row follows these rules.

## The machine (O3DS first; the N3DS is faster, so a change that is good on the O3DS is good there)
- ARM11 MPCore, ARMv6K, 268 MHz. The game thread (frame loop and walker) runs on core 0. The game logic runs on its own thread (pm_logic.c, CORE2C): core 1 on the O3DS (the system core, at most 80% for the app, shared with audio, GSP and DSP work), core 2 on the N3DS. Each core has its own L1. Data the walker reads and the logic writes goes through the walker queue (nd_wq.c) or the display context double buffer; never share a table between the two threads without the queue.
- L1: 16 KB instruction + 16 KB data, 4-way, 32-byte lines. The O3DS has no L2. A miss goes to FCRAM: roughly 30 to 60+ cycles.
- No hardware integer divide. No NEON. VFPv2 single precision is fine; double precision is slow. Unaligned access is slow or faulting.
- GPU commands and vertex data live in linear FCRAM. The CPU writes them through the cache; flush the written span once per split, not per write. The CPU must not read VRAM.

## Rules
1. Hot working set under 16 KB of code and 16 KB of data per hot loop. The op loop plus its hottest handlers must fit the I-cache. Move cold code (logging, pipelog/po3a/mtxlog blocks, error paths, rare ops) out of the hot function into `__attribute__((cold, noinline))` functions or other files; compile the cold files `-Os`.
2. Data layout: structure-of-arrays or tight structs of 16/32/64 bytes aligned to 32 so one record is one or two lines. No table scanned linearly if it is larger than a few lines (gm_tab 16 KB fills the whole L1D); use a small hash or an index. Keep per-op state in a few lines.
3. Access patterns: sequential reads and writes; prefetch ahead with `__builtin_prefetch` (PLD) in long walks; write buffers in order, in whole words (use aligned 32-bit/64-bit stores, `memcpy` on aligned pointers compiles to ldm/stm).
4. No division or modulo in hot code: shifts, masks, power-of-two sizes, reciprocal multiply. Use `float`, never `double`; avoid int/float conversion round trips and `sqrt`/`fmod` in hot paths.
5. Branches: dispatch by table, keep the common case first, avoid long if-else chains per op. Do not put a function-pointer call on a per-vertex path.
6. Do less per op, not the same work faster: skip an op when the shadow state already holds the same value, before the dispatch prologue; hoist loop-invariant state to the producer; batch.
7. Memory: no per-frame malloc/free. Reuse buffers. Keep big arrays out of hot loops' cache footprint (the 4 KB page filter style is fine; a 16 KB scan is not). A new large buffer (tens of KB and up) is allocated on the heap when its feature runs, not declared static: static size comes off the O3DS ROM margin (section below).
8. Cache coherency: write GPU-visible data, then flush that span once (GSPGPU_FlushDataCache) per split; do not flush per draw; never leave CPU-read data in write-only (uncached) memory.
9. Measure: every row reports the Azahar quantity and, when it touches the walk, the sampled opsplit/prod lines. Azahar does not model caches, so a row for cache behaviour is confirmed on the console through the counters in the final build; put the counter in the build.
10. Prefer a smaller code path over an inlined big one. `static inline` only for tiny functions; no inlining of a handler into the loop that would push the loop past the I-cache.

## Memory budget (O3DS XL, Homebrew Launcher in applet mode)
The game takes the whole process commit at boot (pm_mem.c __system_allocateHeaps): heap = commit - image - linear pool (14208 KB) - socket block (128 KB), capped at 64 MB, image = static (text, rodata, data, bss) + loader cost. pm_rom_init keeps 3458 KB of heap for the game and needs PM_ROM_PACKED (25748 KB) for the ROM: margin = heap - 3458 KB - 25748 KB. Below 0 the game stops at boot with "Not enough memory for the ROM". On the O3DS XL the commit is 65536 KB, so every KB of static size is a KB off the margin; the N3DS and Azahar hit the 64 MB cap first and do not show it.
- Heap held from boot: the pause picture (nuGfxZBuffer, 320 x 240 x 2 = 150 KB, malloc) is reserved once after the ROM (pm_main.c pm_zbuf_reserve). It is heap, not static, so memcheck does not count it; on the O3DS XL the commit is full after the map loads (crash log), so the reservation is made while the heap is whole.
- Goal (user): fit the applet-mode budget by default. When that is not feasible, an OVER build is a deliberate choice, not a failure.
- 3ds/tools/memcheck.py computes the margin from the map and 3ds/tools/mem_budget.json (device facts, which values are measured and which derived) and prints `memcheck: static <KB> margin <KB> device o3ds_xl OK|OVER` plus the 10 largest .data/.bss sections. OK needs margin >= 256 KB (safety, reason in the budget file). build.sh prints it after every build; push_3ds.sh stops an OVER push unless `--mem-ok` (go.ps1 -MemOk), which pushes and logs the override.
- Static moves in 4 KB steps, not by its byte growth: static = page_up(__end__) - __start__, and the .mapstat section (3ds/ld/map_restore.ld, ALIGN(0x1000)) starts on a page. When the rodata group (ends after .ARM.exidx) passes a page boundary, .mapstat and all later sections move one page, so about 1 KB of new code and strings can cost 4 KB of margin (CHEATSAVE: 258 -> 254 KB). Check the rodata group end in the map before a row adds rodata.
- The boot line `mem boot:` (every mode) prints region, commit, image, static, heap, cap and margin. A console boot of a new build replaces the derived values in mem_budget.json.
