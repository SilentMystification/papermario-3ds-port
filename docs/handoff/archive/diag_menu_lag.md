# Diag: partner menu and file select lag (build 1297-ram, O3DS)

Source: build3ds/hw_live_log_170.txt (mtime 2026-10-29 02:56, build 1297-ram). Stale: build3ds/hw_pull/hw_o3ds_log.txt is build 1227-dc (romstall up to 13920 ms), not used.

## Numbers (live log, 709 spike lines, frames f2553 to f3999)
- Frame wall avg 66.1 ms, max 80.3 ms (f2611). 303 of 709 frames at or above 66 ms. Rate about 15 fps.
- gbi phase avg about 42 ms per frame (about 64 percent of wall). gpu about 4.5 to 5 ms. dl about 3.3 ms. logic about 2.8 ms. fbegin about 5.6 ms.
- GBI ops per frame 5531 to 6017. Triangles per frame 1032 to 1057. Draws per frame 106 to 116.
- taskpl main avg 6.9 ms per frame (ops avg 5916).
- gpusplit gproc avg 58603 of cpu avg 78184 (unit not labelled in log).
- gbisplit tex avg 17501, gv avg 2247, sync avg 2475 (unit not labelled in log).
- dcsplit kick avg 1864, draw avg 2833.
- Texture misses: 0 in 674 of 709 spike frames. Max 5 in one frame. texstat f3000 miss 164 (732 us). f3300 miss 14 (48 us). f3600 miss 8 (32 us).
- romstall lines: 0. rom reads: 0 in 652 spike frames, 1 to 2 reads of 896 to 1344 B in 62 frames.
- V05 off-screen warnings: 0. No off-screen counter line in the log.
- No partner-menu or file-select marker in the log. Only "game mode 4 frame 2765" (line 1121). The menu lines at lines 12 and 187 are the debug menu (Logging On and close).

## Code and docs
- File select flip, PT86 (3ds/SLICES.md line 124): step 146 to 160 gb 9.1 to 9.4 ms vs 2.8 ms before/after. Frame 156: 96 mesh chunks, 197 texrects, 16 fills.
- Rotating box: draw_box DRAW_FLAG_ROTSCALE, bug B (docs/handoff/agent_tex.md). Not started. No ROTSCALE symbol in 3ds/src.
- Off-screen slots: pm_gpu.c OFF_N = 4 (line 231). Used by pm_gbi.c line 226.
- Partner menu: no named function found in 3ds/src. Its draw path is not confirmed.

## Ranked candidates
1. GBI op volume per frame (about 6000 ops, about 1050 tris). Evidence: gbi phase 42 of 66 ms; ops and tris stable across the window. Settle: log the phase count line (gbiops, tri, draw) at partner menu open and at menu close, same build. Confirmed if open shows the rise in gbiops and gbi above 40 ms.
2. Per-piece flip draws (PT86: 96 mesh chunks, 197 texrects per flip frame). Evidence: PT86 measured 9.1 to 9.4 ms per step; flip frames are the only cost recorded for file select. Settle: draw count (dcsplit draw, phase draw) in file select flip frames 146 to 160 against an idle file select frame.
3. Off-screen VRAM render of the rotating box (OFF_N = 4). Evidence against: 0 V05 warnings, no off counter in log. Unproven. Settle: one per-frame off-begin count in the file select flip.
- Texture upload cost ruled out for steady frames: 0 texture misses in 674 of 709 spike frames.

## Next step
Run one until-line on partner menu open with phase count lines in the log. Do not use 1227-dc logs.
