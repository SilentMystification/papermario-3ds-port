# diag_hang1a_svc: gxCmdQueueAdd svcBreak on the nok11_splash run

Read-only. No source edit, no Azahar start, no tree change. Worktree: papermario-3ds-port-main (branch native).
Tags: CONFIRMED (file:line read, or log/map value read) or INFERRED (reasoning, not measured).

## 1. The check and the constant

- CONFIRMED: `gxCmdQueueAdd` is libctru `gxqueue.o`, at 0x3E35CC, size 0x108, end 0x3E36D4. Source: build3ds/lane-hang1a/pm_3ds.map:58767-58768.
- CONFIRMED: the break PC 0x3ECE88 is `svcBreak` (svc.o). Source: pm_3ds.map:59366.
- CONFIRMED: both LR values are inside `gxCmdQueueAdd`. Hang1a LR 0x3E36B4 is at +0xE8. Baseline LR 0x3E35E4 is at +0x18. So `gxCmdQueueAdd` calls `svcBreak` from two different call sites.
- INFERRED: the check before each `svcBreak` is a queue-full test (entry count against a maximum). The two sites are probably two different checks. The exact condition is not read.
- INFERRED: R0 = 0 in the register dump (last_exception.txt) means `USERBREAK_PANIC`. This value comes from libctru headers that are not on disk.
- Constant: NOT FOUND. The libctru source and headers are not on this PC. `C:\devkitPro` does not exist. `docker/Dockerfile.build:2` uses `devkitpro/devkitarm:latest` (not pinned). The map path `/opt/devkitpro/libctru/lib/libctru.a` is inside that image.
- Queue size: NOT FOUND. It is set at init by the caller (citro3d). Not read.
- Citro3d also holds the GX queue until `C3D_FrameEnd` (docs/plans/topology.md:96, pm_gpu.c:2036). So entries of one frame add up before they run.

## 2. Queue adds per splash frame (code reading)

Splash path (src/effects/water_splash.c:190-221): 40 particle loads (`gDPLoadMultiTile` at :207) of the frame buffer. Each load reaches `grab_cur` through pm_gbi.c:2575 (and pm_gbi.c:1205 for unit 1).

HANG1a guard (in the tree now):
- CONFIRMED: `grab_cur` does one `pm_gpu_grab` per frame while `splash_on` is set. Guard: pm_gbi.c:272-277. Marks: pm_gbi.c:4047-4048.
- CONFIRMED: `grab_impl` (pm_gpu.c:1826-1882) on a non-reuse grab adds these queue entries:
  - `pm_gpu_off_begin(cfb,320,2)` at pm_gpu.c:1843 calls `C3D_RenderTargetClear` at pm_gpu.c:1734 (+1). Line 1734 runs on every begin that passes the early return at :1677.
  - `pm_gpu_off_end` at pm_gpu.c:1844: flush and `C3D_FrameDrawOn`, no clear (+0). See pm_gpu.c:1755-1763.
  - `C3D_RenderTargetClear` of the main target at pm_gpu.c:1847, only on the first split with `want_clear` (+0 or +1).
  - `C3D_FrameSplit` at pm_gpu.c:1851 (+1).
  - `GX_TextureCopy` via `grab_queue_copy` at pm_gpu.c:1803-1807 (+1).
  - `pm_gpu_off_begin(cfb,320,2)` at pm_gpu.c:1854 calls the clear again at :1734 (+1).
  - Two `pm_gpu_tri` and `flush` at pm_gpu.c:1865-1867 (+0 unless the flush splits).
  - `pm_gpu_off_end` at pm_gpu.c:1874 (+0).
  - Total per grab: 4 adds, or 5 with `want_clear`.
- CONFIRMED: `split_frame` (pm_gpu.c:326-334) is used by snapshots only (pm_gpu.c:446, 964). The splash path does not call it.
- CONFIRMED: `kick_check` (pm_gpu.c:627-640) adds at most 2 splits per frame, plus one clear at :631. It runs only with `off_active` 0.
- CONFIRMED: the particle loads do not call `pm_gpu_off_begin` (pm_gbi.c:290 takes only ROLE_SLOT or ROLE_DEPTHQ). Their flush per tile (pm_gbi.c:2573) adds no GX entry unless a kick split runs.
- INFERRED: `C3D_TexLoadImage` (pm_tex.c:547) adds a GX entry on the VRAM upload path. The splash windows read the grab slot, so no upload is expected. Not verified (citro3d not on disk).

Baseline (no HANG1a guard): each particle load can call `grab_impl` when its rows are dirty (pm_gpu.c:1836). So up to 40 grabs and about 160 adds in one frame. This is INFERRED from docs/handoff/investigate_nok11_splash.md section 2.

Contradiction to note: both runs break at the same frame, one frame after shot 12 frame 403 (log line #139). The HANG1a run has one grab per frame, so its count is about 4 adds. The break still occurs. So the grab count alone does not explain the break.
- CONFIRMED: no `grab` line is printed in either run (grablog is on). `grab_log` runs only at pm_gpu.c:1824-1825 (reuse at :1836, success at :1875). So the break came inside `grab_impl` before :1875 or before any grab, or the grab returned early at :1833, :1842 or :1843.

## 3. Smallest change that stops the overflow (no patch)

- File and function: `3ds/src/pm_gpu.c`, `grab_impl`, and `pm_gpu_off_begin` line 1734.
- Change: the grab slot needs no clear, because the quad at pm_gpu.c:1865-1866 covers the whole slot. Skip the clear at :1734 for the calls from `grab_impl` (a flag argument, or move the clear out of `pm_gpu_off_begin`). This removes 2 adds per grab (4 to 2).
- Caveat (INFERRED): other callers of `pm_gpu_off_begin` may need the clear. A change to line 1734 changes every off-screen pass. Test needed under the scenario catalog before commit.
- If the break is a per-frame total from the map base (not the grab), this change does not stop it. Section 4 item 3 must be read first.

## 4. Open items

1. Read `gxCmdQueueAdd` disassembly from the devkitarm image (libctru.a, gxqueue.o). This gives the compare and the constant. Not done (no source on disk, bash blocked in this run).
2. Find which of the two `svcBreak` sites (+0x18, +0xE8) fires in each run.
3. Count `gxCmdQueueAdd` calls per frame in the nok_11 scene, base and splash. Needs a counter. None exists in the tree.
4. Add an entry line in `grab_impl` before pm_gpu.c:1826 work (diagnostic row) to see whether the grab starts. Not done.
5. Confirm R0 = `USERBREAK_PANIC` against the libctru header.
