# audit_tex_upload: texture upload ordering and cache coherency (branch native)

Scope: read-only code audit of 3ds/src/pm_tex.c and 3ds/src/pm_gpu.c. No code change. No Azahar start (see Count).
Labels: PROVEN = read in the code at the line given. INFERRED = depends on citro3d or GSP timing that is not in this tree.

## Short result
- PROVEN: CPU to GPU flush is present on every GPU-read path of the texture code (table, rows 1, 2, 7, 8). The missing piece is ordering, not the flush.
- PROVEN: the default run has no GPU wait at frame begin. `fb_flags` is C3D_FRAME_SYNCDRAW only when `pm_interp_on > 0` (pm_gpu.c:1949). `pm_interp_on` is 0 by default (pm_options.c:14 `fps60 = 0`, pm_interp.c:83). The comment at pm_tex.c:928 ("previous frame has finished on the GPU (C3D_FRAME_SYNCDRAW)") is false in the default run.
- PROVEN: the two-frame stage free rule (pm_tex.c:935-936, `tc_frame >= up_frame + 2`) counts CPU frames, not GPU completion. Without the wait in row 10, the GPU can still read the staging copy when the CPU frees it.
- PROVEN: LRU eviction cannot reuse a slot that this frame or the previous frame sampled (pm_tex.c:783). The in-frame race in the request is not possible on the eviction path.
- GAP (PROVEN by order in code, effect INFERRED): invalidation drops a slot at once when the slot was last drawn in the previous frame (pm_tex.c:1012-1025, 1053-1066). Its extent and staging copy go back to the pool while the GPU may still sample them.
- GAP (INFERRED): the grab copy buffer `copy_blk` is one block for every grab and snapshot of a frame (pm_gpu.c:354, 380, 414, 1771, 1798, 427). A second copy overwrites data that an earlier part may still sample. This is the best candidate for the star transition (strips of the main target).
- Recommended test: one console run with the option fps60 on (SYNCDRAW on). If the garble rate falls, rows 1 to 4 are the cause. If it does not, rows 6 and 9 remain.

## Paths (texture and GPU-read data)

| # | Path | Code | Flush before GPU read | Ordering | Status |
|---|---|---|---|---|---|
| 1 | Miss: convert into linear stage, then VRAM upload | expand and tiling into `linearAlloc` stage pm_tex.c:1350-1351, memcpy 1357-1358, `pend_add` 1362. Upload: `upload_vram_slot` pm_tex.c:546-557 (flush 550, `C3D_TexLoadImage` 553, `up_frame` 556) | Yes: `pm_fl_dc` at 550, before the GX request | Upload runs in `pm_tex_upload_vram_before_frame` (pm_gpu.c:1944), before `C3D_FrameBegin` (pm_gpu.c:1951). The copy runs at the first submit of the frame. Draws of that frame come after it. | OK for the copy itself. GAP for the free (row 2). |
| 2 | Staged draw: PICA samples `vram_stage` in the miss frame | `tex_for_draw` pm_tex.c:697-704, flush 699 at each bind | Yes, per bind | Frame M draws use the stage. | GAP: stage freed at frame M+2 begin by pm_tex.c:935-936 (no GPU wait). |
| 3 | Stale slot with a copy still queued | Copy queued pm_gpu.c:1944, `drop` at begin pm_tex.c:933, `linearFree` in `drop` pm_tex.c:645-646 | n/a | The stage is freed before the GX copy is submitted (submit is at the end of the frame, pm_gpu.c:2031). | PROVEN order error. Effect: the copy reads freed linear memory. The target extent is already dead, so the visible harm is small (INFERRED). |
| 4 | Invalidate or hash change of a slot drawn in the previous frame | `tex_invalidate_body` pm_tex.c:1012 (stale only if `epoch == epoch`), else `drop` at 1021. `slot_valid` pm_tex.c:1053-1066. | n/a | No protection for `epoch + 1`. The eviction rule (783) has it, this path does not. | GAP. Extent and stage return to the pool at once. Reuse is safe only if the GPU finished frame M-1. Not guaranteed in the default run (row 10). |
| 5 | LRU eviction of a victim slot | `evict_oldest_done` pm_tex.c:780-795, skip test at 783 (`epoch == epoch` or `epoch + 1 == epoch`) | n/a | Blocks reuse within this frame and the previous one. | OK (PROVEN by code). Measured: no evictions on kzn_19 and mac_01 to frame 600 (catalog rows kzn19_texstat and mac01_texstat, single run each, unverified). |
| 6 | Grab and snapshot copy buffer | `copy_blk` pm_gpu.c:354, `copy_get` 380, snapshot `dst = copy_get()` 414, `grab_tex_init` 1771, `grab_queue_copy` 1798 (`GX_TextureCopy`), FX2a copy 427 | Not needed (GPU writes it, GPU reads it) | `split_frame` (pm_gpu.c:314-327 region) orders the copy after the earlier part in the GX queue. It does not wait for the PICA to finish that part. pm_gpu.c:418 says the copy overwrites the earlier grab (PT12). | GAP (INFERRED). A second grab in one frame overwrites the buffer that an earlier part samples. |
| 7 | Vertex, ring and index data | `gpu_flush_written` pm_gpu.c:314-325 (before every split and at end) | Yes | Flush before submit | OK. Index arena pm_gpu.c:2492. Noise texture pm_gpu.c:91 (once at boot). |
| 8 | Palette (TLUT) | CI textures are expanded on the CPU in `expand` (pm_tex.c:340-395). No palette goes to the GPU. | Not needed | n/a | OK. |
| 9 | CPU read of the target (frame-buffer readback) | pm_gpu.c:1618-1630 reads `colorBuf` with the CPU. It calls `pm_gpu_begin_wait` (1618), which does not wait for the GPU in the default run. | No invalidate in this path | The CPU can read while the PICA still writes the target. | GAP (INFERRED). Candidate for the pause screen. |
| 10 | Frame sync (affects rows 2, 3, 4) | `fb_flags` pm_gpu.c:1949, `C3D_FrameBegin` 1951, `pm_gpu_begin_wait` pm_gpu.c:1919-1921 and 1981 (`pm_interp_on > 0` only). Default `fps60 = 0` (pm_options.c:14). | n/a | No GPU wait at frame begin in the default run. | PROVEN in flags. The effect is INFERRED: citro3d without SYNCDRAW may still wait on its own. The citro3d source is not on this machine. |

## Answers to the three questions

1. Flush and request order. Every CPU write read by the GPU has a flush before the request (rows 1, 2, 7, 8). The request order is correct for the copy. The gaps are the free rules (rows 2, 3, 4, 6), and they depend on GPU completion that the default run does not wait for (row 10).
2. Eviction. No in-frame reuse on the LRU path (row 5). The protected set is `epoch == epoch` or `epoch + 1 == epoch`. Invalidation (row 4) does not have that protection, so it is the in-frame and previous-frame gap.
3. Count. Not measured in this audit. See Count.

## Count (evictions per frame)

No Azahar start in this audit. Reasons:
- The texstat report prints at frame 300 (`tc_frame % 300`, pm_tex.c:922-925). The hos_01 texword run did not reach frame 204 within its 25 s cap (test_catalog.md, hos01_texword row). A 25 s cap cannot reach the report.
- The per-map counter `texmap` (pm_mem.c:234-237, pm_tex.c:867-871) prints at each map load. The scenarios that use it are tours with cap 90 to 1800 s (azt_scenarios.json, lines 1839, 1871, 2457, 2801).
- A short scenario needs a new entry in azt_scenarios.json and a catalog row in the same commit. This task writes only this file.
- Existing evidence: kzn19_texstat and mac01_texstat (CACHE1, frame 600): 0 evictions, 459 and 104 live slots. hos_01 has no count.

Proposed scenario for the orchestrator: `hos01_texmax` with debug `warp=hos_01:0 texmax loglevel=debug`, until `texmap since last map`, cap 25. The line comes at the warp load, so it fits the cap. Status: unverified until one run.

## Fix proposal (for a code agent, after the test above)
- Ordering of frees (rows 2, 3): free a stage or an extent only after the GPU has finished the frame that sampled it. Use a pending-free list tagged with the frame number, released after a GPU wait. Cost: less CPU and GPU overlap. KICK1 (pm_gpu.c:599-620) was built to keep that overlap, so measure it.
- Invalidation (row 4): send drops of slots drawn in frame M-1 to the same pending list. Do not drop at once. This is the same rule as the eviction protection (pm_tex.c:783), extended to invalidation.
- Grab copy (row 6): give each grab or snapshot in a frame its own block (a small pool), or wait for the GPU before the second copy into `copy_blk`.
- Stage drop (row 3): do not free `vram_stage` while `vram_up` copy is queued. Move the free after the copy submit.
- Rule to keep: never reuse a slot or block that a command list of this frame or the previous frame can sample. The eviction path already does this (pm_tex.c:783). The other paths do not.

## Test proposal (console, user judges)
- Run one console session with option fps60 on (SYNCDRAW on, pm_gpu.c:1949). Compare the garble rate on the wall (Shooting Star Summit), sewer coins and the star transition with the default run. The fps60 run has lower fps. Use it only as an A/B for the cause.

## Unverified
- citro3d `C3D_FrameBegin` behaviour without SYNCDRAW and whether `C3D_TexLoadImage` copies at submit or at call time. The citro3d and libctru sources are not on this machine (no devkitPro tree found under /c/devkitPro).
- GSP and PICA ordering of GX copies and command lists in one submit. The repo comments (pm_gpu.c:599-600, 1765-1767, 1997-1998) state the order as the call order. Not checked against hardware.
- Lag of the GPU behind the CPU in a heavy scene on console. Not measured.
