# Diag: CPU/GPU crossings (row XING)

Scope: static audit of 3ds/src at HEAD ae3497177, plus O3DS log build 1227 (build3ds/ftp_170/hw_o3ds_log.txt). No Azahar run. No source edit.
Tags: [M] measured in log, [C] code read, [E] estimate, [U] not measured.
Scenes (log lines): A mac_01 16308-34716; B map3 34717-43053; C map9 43054-45129; D map10 45130-53296.

## (a) CPU reads of GPU memory
Yes. Two paths read render-target memory with the CPU.
- pm_gpu.c:1489 pm_gpu_read_frame reads frameBuf.colorBuf (VRAM) per pixel, 320 x 240, Morton index. 17.4 ms per call [M, "D03" lines #48588-#51759]. Callers: pm_main.c:263 (fallback), pm_main.c:272 (pause fill, cpu=1).
- pm_gpu.c:2445 pm_gpu_depth_at reads depthBuf (VRAM) per pixel. Caller pm_depth.c:16. Queries in log: 0 [M].
- Swirl or pause capture: 20 calls per 30 frames at f5309 and f5339 [M]. Sums 21.25 ms and 4.09 ms per 30 frames. Max 17.6 ms [M].
- Frame buffer texture reads through pm_tex (pm_fb_native): [C], count [U].

## (b) Per-draw wait, flush or submit
None found [C]. Per frame: closes 53 [M], flush calls 5.8 [M, plan_draw_cost], FrameSplit 1.0 [M, kick], FrameEnd 1 [C pm_gpu.c:1836].
One GPU wait per frame: C3D_FrameBegin(SYNCDRAW) at pm_gpu.c:1780 [C].

## (c) CPU blocked on GPU (PH_FBEGIN) versus work
| scene | frame ms | fbegin wait ms | share | gbi walk ms | idle ms |
|---|---|---|---|---|---|
| A mac_01 | 60.6 | 4.30 | 7.1 % | 33.7 (55.6 %) | 0.00 |
| B map3 | 35.0 | 2.10 | 6.0 % | 20.3 | 1.75 |
| C map9 | 33.7 | 0.97 | 2.9 % | 16.5 | 4.00 |
| D map10 | 35.3 | 0.72 | 2.0 % | 13.5 | 8.28 |
| E maps 6,5,6 | 34.6 | 2.10 | 6.1 % | 15.2 | 3.15 |
Source: phase lines, 4 windows per scene [M]. Share = fbegin / frame ms [C].

## (d) CPU writes into GPU-read linear memory, per frame
- Texture upload: CPU expand and memcpy into vram_stage (pm_tex.c:1352-1363), then C3D_TexLoadImage (pm_tex.c:536-539). 1.1 KB per frame in A, 2.6 KB per frame in D [M, texstat sums / frames]. Misses 1.6 per frame [M, diag_cpu_gpu_work].
- Palette matrix uniforms: C3D_FVUnifMtx4x4 (pm_gpu.c:914, 938), 64 B, plus 48 B when pal_ntab is set. 31.5 uploads per frame [M, gv1pal delta f1500-f3600]. Up to 3.5 KB per frame [C size].
- Vertex ring: memcpy 16 B per vertex (pm_gpu.c:2112). 279 vertices per frame [M, gv1 load delta]. 4.5 KB.
- Index ring: about 2270 u16 per frame. 4.5 KB [M, plan_draw_cost].
- Vertex pool: 88 slots, 3.5 KB [M, plan_draw_cost].
- Flush ranges per frame, total: about 12 KB [E].
## (e) Off-screen passes and transfers
- T2 screen grab (pm_gpu.c:1657 grab_impl, GX_TextureCopy at pm_gpu.c:1643): 0 grabs in 189 b7 lines [M]. Not used in the logged scenes.
- Previous-frame copy (pm_gpu.c:1781-1782, queued at pm_gpu_begin): once per frame when prev_want is set [C]. Count [U].
- Off-screen slot pass (pm_gbi.c:226 calls pm_gpu_off_begin): per frame count [U]. No counter exists.
- FX2a snapshot (pm_gpu.c:398, 865): GX_TextureCopy of 512 KB per snapshot [C]. Count [U].
- Display output: C3D_RenderTargetSetOutput and the GX_TRANSFER flags (pm_gpu.c:1229), set once at boot [C]. Present is done by citro3d at FrameEnd.
- Fog: GPU (pm5) unless GVR_FOG (pm_gbi.c:1363) [C]. Blur and shadow: [U].
## (f) CPU per-vertex and per-triangle work in the draw path
| item | code | per frame | ms per frame |
|---|---|---|---|
| GP1 cross load: 16 mul-add and clip bits per vertex | pm_gbi.c:3441-3457 | 21 vertices [M, gp1 delta] | under 0.02 [E] |
| Lazy transform for clipping (gv_xf) | pm_gbi.c:1270-1294 | 28 vertices [M, gv1 xf delta] | under 0.02 [E] |
| Box scan, 3 compares per vertex | pm_gbi.c:3412-3420 | 279 vertices [M] | under 0.01 [E] |
| Ring copy, 16 B per vertex | pm_gpu.c:2112 | 279 vertices [M] | under 0.01 [E] |
| Full CPU vertex path (fallback loads, vert_color) | pm_gbi.c:3461-3476 | [U] | [U] |
| CPU triangle path (GVR reasons) | pm_gbi.c:1362-1364, gv_fb | 14.9 of 788 triangles [M] | 0.55 at 37 us per op [M, opsplit] |
| Palette matrix product (Mtx_Multiply) | pm_gpu.c:910-913 | 23 [M, pmul] | about 0.05 [E] |
| G_MTX product chains and divides | pm_gbi.c:400, 1926 | [U] | 5 to 10 us each [E, diag_hw_log1] |
Vertex load op: 44.6 us per op [M, opsplit f2482]. CPU triangles: 1.9 % [M].
Finding: no GPU work runs on the CPU by mistake. CPU transform runs only for clip-crossing loads and fallbacks.

## (g) Top 3 crossings by ms, mac_01
1. Tex-op flush closes, 7.2 ms [M, diag_frame_budget]. CPU writes GPU command words per close (not a wait). Fix: DS2, -Os and cold split in pm_gpu.c flush_draw (line 838). Expected 0 to 2 ms [E].
2. GPU submit phase, 4.45 ms [M], includes gcache 1.67 ms [M]. gcache is 3 svc flush calls per split (pm_gpu.c:263-275, pm_fl_dc at 2044). Fix: one linear block for pool, ring and index, so one flush call covers them (alloc at pm_gpu.c:1253-1297). Expected 1.1 ms [E].
3. FrameBegin GPU wait, 4.30 ms [M]. The wait is the GPU tail after FrameEnd (pm_main.c:443, pm_gpu.c:1780). Fix: second kick split at 85 % of draws, in kick_check (pm_gpu.c:541-560, kick_at at pm_gpu.c:1816). Expected 1 to 2 ms [E]. Cost: one extra split, [M] about 0.01 ms.
Next step: DS0 counters (plan_draw_cost) before the three fixes. Readback (a) is rare. Fix it only if the 17.4 ms call shows on console.
