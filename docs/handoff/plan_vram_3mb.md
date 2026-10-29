# Plan: 3 MB VRAM texture cache with a fixed bank layout (plan_vram_3mb)
(Opus plan, saved by the orchestrator. Read: pm_tex.c, pm_gpu.c, pm_mem.c, pm_tex.h, sweep_texmax.md, review_tex_cache.md, agent_texmax.md, libctru vram.h from the build image (`docker run pm3ds-build:1`).)

## Corrections to the stated facts
- The 8-bit slots are 64x64, 16 KB each, not 512 KB (pm_gpu.c pm_gpu_off_begin, branch `siz==1 && stride<=64` gives w = 64). They keep pixels frame to frame (FX2a), so nothing can share their memory.
- A grab uses 1024 KB, not 512 KB: `grab_tex` (512 KB) plus one full off-screen slot (`pm_gpu_off_begin(cfb,320,2)` in grab_impl).
- Uncounted user: snaps. `off_snapshot` makes one copy per slot it is called for; a full slot's copy is 512 KB.
- Real worst case with no sharing: 8,622 KB (750 + 3072 + 4x512 + 512 + 4x512 + 12x16).
- The two-slab failure from map 31 was LINEAR memory, not VRAM. agent_texmax.md shows linear peak 14094 of 14208 KB. Each live slot keeps two linear copies of its texels: `vram_stage` is freed only in `drop()`; `tex_lin_heap` is built in `slot_tex_lin_build` and never read (`tex_lin` has no reader). A 3 MB cache therefore costs about 6 MB of linear memory. Row 1 fixes this first.
- The 3072 KB block failure fits the bank split: 5394 free = 6144 - 750. Colour buffer is in one bank and depth in the other, so each bank had 2697 KB free.

## API (vram.h in the image)
- `vramAllocAt(size, pos)`, `vramMemAlignAt(size, align, pos)`, `vramFree`, `vramGetSize`, `vramSpaceFree`. `pos` = `VRAM_ALLOC_A`, `VRAM_ALLOC_B` or `VRAM_ALLOC_ANY`.
- `C3D_TexInitVRAM` uses `vramAlloc` (ANY), no bank argument. Bank-placed texture: `C3D_TexInit` (linear), then `p = vramMemAlignAt(size, 0x80, bank)`, `linearFree(old data)`, `tex.data = p` (pattern already in pm_tex.c, slab path).
- Verify in the row 2 boot log: citro3d puts depth in the bank opposite colour (libcitro3d.a has `addrGetVRAMBank` and `vramBank`); `C3D_TexDelete` calls `vramFree` for a VRAM address (`addrIsVRAM`). Bank A is 0x1F000000 to 0x1F2FFFFF; bank B starts at 0x1F300000.

## (1) Bank layout (KB, each bank 3072)
| order | bank A | bank B |
|---|---|---|
| 1 boot, citro3d | colour 375 | depth 375 |
| 2 boot, small arena (all 64x64 users made once, never freed) | 3 small slots + 3 snaps = 96 | 3 small slots + 3 snaps = 96 |
| 3 boot, `vramMemAlignAt(1536K, 0x80, bank)` | cache half A 1536 | cache half B 1536 |
| free after boot | 1065 | 1065 |
| lazy full buffers (512 each), A first then B | 2 = 1024, margin 41 | 2 = 1024, margin 41 |
| after a shrink (half B released) | unchanged | 2601 free: 5 full, margin 41 |
- The small arena covers the 4 small slots and the 2 8-bit slots with their snaps. Made at boot so no 16 KB block lands in a 512 KB hole later.
- The cache has two slab halves. Each extent records its half. `slab_free` never merges across halves. `slab_alloc` skips a half that is draining.
- Remove the 768 KB fallback and the TEXCACHE OFF path.

## (2) Lazy users
- Full slots 0..3 hold data for one frame only: `pm_gpu_begin` sets `base=NULL` for `k<OFF_P`. Any full slot can be freed when idle.
- Copy buffer: one 512 KB block shared by `grab_tex` (256x512) and every full-slot snap (512x256). Both are 524,288 B, each with its own header on the same data. Never live together: each copy is queued after a split (`split_frame` in `off_snapshot`, `C3D_FrameSplit` in grab_impl); each reader is flushed before the next split (grab quad flushes at once, the snap reader is the batch right after the copy); queue order serialises them. One exception: the PT12 previous-frame grab copies at `pm_gpu_begin` and reads later. A snapshot in between sets `prev_pre=0`, and `pm_gpu_grab_prev` returns 0 (the CPU read path that already exists).
- The 8-bit slots cannot share memory with the grab copy (they keep pixels across frames, 16 KB). They stay in the small arena.
- Free rule: at `pm_gpu_begin`, after `C3D_FrameBegin` (previous frame done), free each full slot (`C3D_RenderTargetDelete` and `C3D_TexDelete`) and the copy buffer when idle for 60 frames. Covers scene end, pause close, battle end, file select end.
- Peak without a shrink: 4 full buffers (for example 3 boxes, or a grab slot plus the copy buffer). OFF_N stays 4; the "used again in turn" path stays.

## (3) Failure rule (no latch disables anything)
- Remove the permanent `off_fail=1` and `grab_ready=-1` on a VRAM failure. A failure lasts one frame and uses the per-frame fallbacks: off-screen slot reuses a full slot in turn if one exists, else draws on screen; snap: no snap; grab: return 0 (CPU read).
- A full-buffer failure calls `pm_tex_shrink()`: half B starts draining, its slots get `stale=1` (existing deferred drop), `slab_alloc` skips B; at the next `pm_tex_begin_frame` with no live slot in B, `vramFree(B)` and `tex_cap=1536`. The request succeeds next frame.
- Regrow: when shrunk and bank B has held no full buffer for 120 frames, `vramMemAlignAt(1536K, B)`; on success `tex_cap=3072`; on failure retry after 120 frames.
- Textures are never disabled. If a half fails at boot, the cache runs on the other half and the regrow loop tries again.

## (4) Rows (in order; PM_ROW per row; budget 8 starts)
Test runs:
- Map run (stress, one start): `warp=arn_02:0 story=ch0_toad_town tour=@tour_all.txt texmax`, `-Until "tour: done" -Timeout 1800`. (Per the user: short tours for quick tests; the full run only for this completion test. Exclude tst_* and debug maps.)
- Effect script, 4 starts: fileselect padscript (golden.py SCENES) until the box turn at frames 282 to 296; `preset=toad padscript=<pause4>`; `preset=toad battle`; the flame map (find with `git log --grep=FX2a`, then verify).
- Shots equal: same frame window on base and new binary, pixel difference 0 in the effect region.

1. Linear memory (about 35 lines, pm_tex.c): free `vram_stage` at the first `pm_tex_begin_frame` after its upload (record the epoch); delete `tex_lin`, `tex_lin_heap`, `slot_tex_lin_build`; on the linearAlloc failure path call `slab_free` (review finding 5); count bytes lost when the extent table is full. Test: map run at 1536: linear peak at least 1500 KB below the HEAD run, tex fail cum 0, lost 0. Console: HUD linear row lower in Toad Town.
2. Per-bank accounting (about 40 lines, pm_mem.c): `pm_vram_alloc(size, bank, what)` and `pm_vram_free(p)`, tracking used bytes and min free per bank. At boot count the target colour and depth buffers and log their banks. HUD: `VRAM A %uK B %uK minA minB fail %u`; per-bank min added to the texmap line. Test: boot log shows colour in A and depth in B; A and B free are 2697 each; values match `vramSpaceFree`. Console: the HUD row is shown.
3. Boot layout and 3 MB cache (about 55 lines, pm_tex.c, pm_gpu.c): small arena at boot, `BUDGET` 3072, two halves through `vramMemAlignAt`, half tag on each extent, remove the fallback. Test: map run: boot free A 1065 and B 1065, tex fail cum 0, evictions per map below the HEAD run. Console: HUD fail 0 and min A/B 1065 on a map walk.
4. Full buffers (about 55 lines, pm_gpu.c): bank-placed helper (A then B), copy-buffer alias with the snapshot clearing `prev_pre`, idle-60 free, remove the latches. Test: effect script: min free per bank at least 41 KB, tex fail 0, shots equal (box turn, pause pages, battle start, flame). Console: HUD min A/B at least 41 through file select, pause, a battle and a water map.
5. Shrink and regrow (about 45 lines, pm_tex.c, pm_gpu.c): `pm_tex_shrink`, drain through stale, free at `begin_frame`, regrow after 120 idle frames. Debug switch `vramfill=N` makes N dummy 512 KB blocks in bank B at boot to force the shrink path (debug tool, may stay). Test: pause script with `vramfill=2`: log shows one "texcache shrink B" and one "regrow B", tex fail 0, shots equal after the regrow, min free per bank at least 41 KB. Console: pause on a water map, close it, wait 2 s: HUD tex returns to a 3072 cap and fail 0.

Note: the working tree had an uncommitted pm_tex.c hunk (`pm_tex_map_state`, sweep420) from another agent. Commit it or keep it out of row 1.
