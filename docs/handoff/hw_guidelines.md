# Hardware guidelines for source changes (3DS topology)

Applies to every code row, in 3ds/src and in the guarded src/ edits. State in each commit message how the row follows these rules.

## The machine (O3DS first; the N3DS is faster, so a change that is good on the O3DS is good there)
- ARM11 MPCore, ARMv6K, 268 MHz. App code runs on one core. The second core is the system core with a small time slice. Plan for one core.
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
7. Memory: no per-frame malloc/free. Reuse buffers. Keep big arrays out of hot loops' cache footprint (the 4 KB page filter style is fine; a 16 KB scan is not).
8. Cache coherency: write GPU-visible data, then flush that span once (GSPGPU_FlushDataCache) per split; do not flush per draw; never leave CPU-read data in write-only (uncached) memory.
9. Measure: every row reports the Azahar quantity and, when it touches the walk, the sampled opsplit/prod lines. Azahar does not model caches, so a row for cache behaviour is confirmed on the console through the counters in the final build; put the counter in the build.
10. Prefer a smaller code path over an inlined big one. `static inline` only for tiny functions; no inlining of a handler into the loop that would push the loop past the I-cache.
